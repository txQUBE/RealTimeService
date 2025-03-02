/*
 * server.cc
 *
 *  Created on: 18.06.2024
 *      Author: Грац Д.Д.
 *
 *  Модуль СРВ
 *  Состоит из:
 *  	RealTimeService
 *  	server
 *  	notification
 *  Имеет меню для управленияК
 */
#include "utils.h"
#include "RealTimeService.h"
#include <pthread.h>
#include <sys/mman.h>

#define NOTIF_CHAN 	"notification_channel"
#define RTS_SHM_TIME_NAME "rts_shm_time"
#define MAIN_MARK "#main" // метка для вывода в консоль
bool shutDown = false; //	флаг завершить работу
bool tick_changed = false; // флаг-статус изменения тика таймера

// Структура для хранения параметров таймера
typedef struct {
	long tick_nsec; // 	Длительность одного тика в наносекундах
	int tick_sec;// 	Длительность одного тика в секундах
	timer_t periodicTimer;// дескриптор таймера
	struct itimerspec periodicTick;// интервал срабатывания относительного таймера в 1 тик
	int count; // 		Номер текущего тика часов ПРВ
} Clock;

Clock timer; // Буфер параметров таймера
timer_t timer_exist = -10;// таймер, "-10" означает об отсутствии таймера

const int CODE_TIMER = 1;//код сообщения-импульса от таймера

pthread_barrier_t notif_barrier; // барьер синхронизации подключения к каналу для уведомлений таймера

void* server(void*);
void* notification(void*);

//функция подготовки к запуску периодического таймера уведомления импульсом об истечении тика часов ПРВ
void setPeriodicTimer(timer_t* periodicTimer,
		struct itimerspec* periodicTimerStruct, int notif_coid);
//Функция установки параметров таймера
void setTimerProps(struct itimerspec* periodicTimerStruct);
//Функция установки приоритета для нити
void set_thread_priority(pthread_t thread, int priority);

//Функции меню управления СРВ
void menu_showList(); // 		отобразить список команд
void menu_changeTick(); //		изменить тик таймера
void menu_showTdbMsList();// 	отобрразить список зарегистрированных СУБТД

/*
 * Нить main
 * Запускает нить сервера регистрации
 * Запускает нить уведомлений об истечении тика таймера
 * Запускает таймер
 * Обрабатывает ввод пользователя в качестве опций меню
 */
int main() {
	pthread_t server_thread, notification_thread;

	timer.tick_nsec = 0; // начальные значения тика
	timer.tick_sec = 5;// 	начальные значения тика
	//инициализация мутекса буфера зарегестрированных СУБТД

	//запуск сервера регистрации
	if ((pthread_create(&server_thread, NULL, server, NULL)) != EOK) {
		cerr << MAIN_MARK << "error server thread launch init, errno" << errno
				<< endl;
		exit(EXIT_FAILURE);
	};

	// установить приоритет ниже нити уведомлений,
	// для контроля доступа к буферу зарегистрированных СУБТД
	set_thread_priority(server_thread, 9);

	pthread_barrier_init(&notif_barrier, NULL, 2); // инициализация барьера подключения к каналу уведомлений для таймера

	//Запуск канала уведомлений
	if ((pthread_create(&notification_thread, NULL, notification, NULL)) != EOK) {
		cerr << MAIN_MARK << "error notification thread init, errno" << errno
				<< endl;
		exit(EXIT_FAILURE);
	}
	//Установить высокий приоритет для нити уведомлений
	set_thread_priority(notification_thread, 10);

	pthread_barrier_wait(&notif_barrier);// ожидание создания канала уведомлений
	pthread_barrier_destroy(&notif_barrier);// освобождение ресурсов

	int notif_coid = 0;
	//подключиться к каналу уведомлений
	if ((notif_coid = name_open(NOTIF_CHAN, 0)) == -1) {
		cerr << MAIN_MARK << "error name_open(NOTIF_CHAN), errno " << errno
				<< endl;
		exit(EXIT_FAILURE);
	}

	//	подготовить таймер
	setPeriodicTimer(&timer.periodicTimer, &timer.periodicTick, notif_coid);
	// 	запуск относительного периодического таймера!
	timer_settime(timer.periodicTimer, 0, &timer.periodicTick, NULL);
	timer_exist = timer.periodicTimer;

	cout << MAIN_MARK << "Enter command:" << endl;
	cout << MAIN_MARK << "Enter 0 to show menu list" << endl;
	// обработка ввода пользователя
	while (!shutDown) {
		int userInput;

		cin >> userInput;

		switch (userInput) {
		case 0:
			menu_showList();
			break;
		case 1:
			menu_changeTick();
			break;
		case 2:
			menu_showTdbMsList();
			break;
		case 9:
			shutDown = true;
			break;
		}
	}

	// дождаться завершения нитей
	pthread_join(server_thread, NULL);
	pthread_join(notification_thread, NULL);
	// очистка ресурсов
	shm_unlink(RTS_SHM_TIME_NAME);
	timer_delete(timer.periodicTimer);
	cout << MAIN_MARK << "EXIT_SUCCESS" << endl;
	return EXIT_SUCCESS;
}

/*
 * функция меню изменяющая значение тика таймера
 */
void menu_changeTick() {
	if (timer_exist == -10) {
		cout << "timer doesn't exist\n";
	} else {
		cout << "Enter new Tick value nsec:\n";
		int newTick_nsec;
		cin >> newTick_nsec;

		cout << "Enter new Tick value sec:\n";
		int newTick_sec;
		cin >> newTick_sec;

		timer.tick_nsec = newTick_nsec;
		timer.tick_sec = newTick_sec;

		setTimerProps(&timer.periodicTick);
		timer_settime(timer.periodicTimer, 0, &timer.periodicTick, NULL);

		// Сброс счетчика тиков
		timer.count = 0;

		tick_changed = true; // статус об изменении тика
		cout << "New timer tick: " << timer.tick_sec << " sec "
				<< timer.tick_nsec << " nsec.";
	}

}

/*
 * Функция установки параметров таймера
 */
void setTimerProps(struct itimerspec* periodicTimerStruct) {
	periodicTimerStruct->it_value.tv_sec = timer.tick_sec;
	periodicTimerStruct->it_value.tv_nsec = timer.tick_nsec;
	periodicTimerStruct->it_interval.tv_sec = timer.tick_sec;
	periodicTimerStruct->it_interval.tv_nsec = timer.tick_nsec;
}

/*
 * Функция подготовки к запуску периодического таймера уведомления импульсом об истечении тика часов ПРВ
 */
void setPeriodicTimer(timer_t* periodicTimer,
		struct itimerspec* periodicTimerStruct, int notif_coid) {
	struct sigevent event;

	// 	импульс для канала уведомлений
	SIGEV_PULSE_INIT(&event, notif_coid, SIGEV_PULSE_PRIO_INHERIT, CODE_TIMER,0);

	if (timer_create(CLOCK_REALTIME, &event, periodicTimer) == -1) {
		cerr << MAIN_MARK << "error timer_create, errno: " << errno << endl;
		exit(EXIT_FAILURE);
	}

	// установить интервал срабатывания периодического таймера тика в системном времени
	setTimerProps(periodicTimerStruct);
}

/*
 * Функция установки приоритета для нити
 */
void set_thread_priority(pthread_t thread, int priority) {
	struct sched_param param;
	param.sched_priority = priority;
	pthread_setschedparam(thread, SCHED_RR, &param); // Используем политику Round Robin
}

/*
 * Функция отображения меню
 */
void menu_showList() {
	cout << "-----MENU-----" << endl;
	cout << "1. Change timer tick" << endl;
	cout << "2. Show registered tdb list" << endl;
	cout << "-----МЕНЮ-----" << endl;
}

void menu_showTdbMsList() {
	pthread_mutex_lock(&tdb_map.mtx);
	cout << MAIN_MARK << "REGISTERED TDBMS LIST:" << endl;
	if (tdb_map.buf.empty()) {
		cout << MAIN_MARK << "Buffer is empy..." << endl;
	} else {
		for (map<string, tdb_ms_t>::iterator it = tdb_map.buf.begin(); it
				!= tdb_map.buf.end(); ++it) {
			cout << it->first << endl;
		}
	}
	pthread_mutex_lock(&tdb_map.mtx);
}

