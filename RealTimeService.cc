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

#define NOTIF_CHAN 	"notification_channel"

bool shutDown = false; //	флаг завершить работу

// Структура для хранения параметров таймера
struct Clock {
	long tick_nsec; // 	Длительность одного тика в наносекундах
	int tick_sec;// 	Длительность одного тика в секундах
	timer_t periodicTimer;// дескриптор таймера
	struct itimerspec periodicTick;// интервал срабатывания относительного таймера в 1 тик
	int Time; // 		Номер текущего тика часов ПРВ
};

Clock timer; // Буфер параметров таймера
timer_t timerId = -10;// таймер, "-10" означает об отсутствии таймера

const int CODE_TIMER = 1;//код сообщения-импульса от таймера

pthread_barrier_t notif_barrier; // барьер синхронизации подключения к каналу для уведомлений таймера

//функция подготовки к запуску периодического таймера уведомления импульсом об истечении тика часов ПРВ
void setPeriodicTimer(timer_t* periodicTimer,
		struct itimerspec* periodicTimerStruct, int notif_coid);
//Функция установки параметров таймера
void setTimerProps(struct itimerspec* periodicTimerStruct);

//Функции меню управления СРВ
void menu_showList(); // 		отобразить список команд
void menu_changeTick(); //		изменить тик таймера

/*
 * Нить main
 * Запускает нить сервера регистрации
 * Запускает нить уведомлений об истечении тика таймера
 * Запускает таймер
 * Обрабатывает ввод пользователя в качестве опций меню
 */
int main() {

	timer.tick_nsec = 0; // начальные значения тика
	timer.tick_sec = 5;// 	начальные значения тика
	//инициализация мутекса буфера зарегестрированных СУБТД
	if (pthread_mutex_init(&tdb_map.Mutex, NULL) != EOK) {
		std::cout << "main: ошибка pthread_mutex_init(): " << strerror(errno)
				<< std::endl;
		return EXIT_FAILURE;
	}

	//запуск сервера регистрации
	if ((pthread_create(NULL, NULL, &server, NULL)) != EOK) {
		perror("main: error server thread launch\n");
	};

	pthread_barrier_init(&notif_barrier, NULL, 2); // инициализация барьера подключения к каналу уведомлений для таймера

	//Запуск канала уведомлений
	if ((pthread_create(NULL, NULL, &notification, NULL)) != EOK) {
		perror("main: error notification thread launch\n");
	}

	pthread_barrier_wait(&notif_barrier);// ожидание создания канала уведомлений
	pthread_barrier_destroy(&notif_barrier);// освобождение ресурсов

	int notif_coid = 0;
	//подключиться к каналу уведомлений
	if ((notif_coid = name_open(NOTIF_CHAN, 0)) == -1) {
		cerr << "main: error name_open(NOTIF_CHAN). errno "<< errno << endl;
		exit(EXIT_FAILURE);
	}

	//	подготовить таймер
	setPeriodicTimer(&timer.periodicTimer, &timer.periodicTick, notif_coid);
	// 	запуск относительного периодического таймера!
	timer_settime(timer.periodicTimer, 0, &timer.periodicTick, NULL);
	timerId = timer.periodicTimer;

	cout << "#main: Enter command:\n";
	cout << "#main: Enter 0 to show menu list\n";
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
			break;
		}
	}
}

/*
 * функция меню изменяющая значение тика таймера
 */
void menu_changeTick() {
	if (timerId == -10) {
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
	}
	cout << "New timer tick: " << timer.tick_sec << " sec "
			<< timer.tick_nsec << " nsec.";
}

/*
 * Функция отображения меню
 */
void menu_showList() {
	cout << "-----MENU-----\n";
	cout << "1. Change timer tick\n";
	cout << "-----МЕНЮ-----\n";
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
		perror("Main: 	error timer_create\n");
		exit(EXIT_FAILURE);
	}

	// установить интервал срабатывания периодического таймера тика в системном времени
	setTimerProps(periodicTimerStruct);
}

