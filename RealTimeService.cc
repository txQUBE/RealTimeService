#include "utils.h"
#include "RealTimeService.h"

#define NOTIF_CHAN 	"notification_channel"

bool shutDown = false; //	флаг завершить работу

struct Clock {
	long tick_nsec; // 	Длительность одного тика в наносекундах
	int tick_sec;// 	Длительность одного тика в секундах
	int Time; // 		Номер текущего тика часов ПРВ
	timer_t periodicTimer;// дескриптор таймера
	struct itimerspec periodicTick;// интервал срабатывания относительного таймера в 1 тик
};

Clock timer; // Буфер параметров таймера
timer_t timerId = -10;// таймер, "-10" означает об отсутствии таймера

const int CODE_TIMER = 1;//код сообщения-импульса от таймера

pthread_barrier_t notif_barrier; // барьер синхронизации подключения к каналу для уведомлений таймера

//функция подготовки к запуску периодического таймера уведомления импульсом об истечении тика часов ПРВ
void setPeriodicTimer(timer_t* periodicTimer,
		struct itimerspec* periodicTimerStruct, int notif_coid);

//Функции меню управления СРВ
void menu_showList(); // 		отобразить список команд
void menu_changeTick(); //		изменить тик таймера

void setPeriodicTimerTick(long nsec, int sec); //		установить тик таймера

/*
 * Нить main
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
		perror("main: ошибка запуска сервера\n");
	};

	pthread_barrier_init(&notif_barrier, NULL, 2); // инициализация барьера подключения к каналу уведомлений для таймера

	//Запуск канала уведомлений
	if ((pthread_create(NULL, NULL, &notification, NULL)) != EOK) {
		perror("main: ошибка запуска нити уведомлений\n");
	}

	pthread_barrier_wait(&notif_barrier);// ожидание создания канала уведомлений

	int notif_coid = 0;
	//подключиться к каналу уведомлений
	if ((notif_coid = name_open(NOTIF_CHAN, 0)) == -1) {
		cout << "main: ошибка подключения к каналу уведомления" << endl;
		exit(EXIT_FAILURE);
	}

	//	подготовить таймер
	setPeriodicTimer(&timer.periodicTimer, &timer.periodicTick, notif_coid);
	// 	запуск относительного периодического таймера!
	timer_settime(timer.periodicTimer, 0, &timer.periodicTick, NULL);
	timerId = timer.periodicTimer;

	cout << "Введите команду:\n";
	cout << "Введите 0 чтобы увидеть список команд\n";
	// обработка ввода пользователя
	while (!shutDown) {
		int userInput;

		cin >> userInput;

		switch (userInput) {
		case 0:
			menu_showList();
		case 1:
			menu_changeTick();
			break;
		case 2:
			break;
		}
	}
}

/*
 * функция меню для ввода нового значения тика
 */
void menu_changeTick() {
	if (timerId == -10) {
		cout << "таймер не запущен\n";
	} else {
		cout << "Введите новое значение тика в наносекундах:\n";
		int newTick_nsec;
		cin >> newTick_nsec;

		cout << "Введите новое значение тика в секундах:\n";
		int newTick_sec;
		cin >> newTick_sec;

		setPeriodicTimerTick(newTick_nsec, newTick_sec);
	}
	cout << "Новый тик таймера: " << timer.tick_sec << " сек "
			<< timer.tick_nsec << " нс.";
}

/*
 * Функция отображения меню
 */
void menu_showList() {
	cout << "-----МЕНЮ-----\n";
	cout << "1. Изменить тик таймера\n";
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
 * Функция создания таймера тика с уведомлением импульсом
 */
void setPeriodicTimer(timer_t* periodicTimer,
		struct itimerspec* periodicTimerStruct, int notif_coid) {
	struct sigevent event;

	// 	импульс для канала уведомлений
	SIGEV_PULSE_INIT(&event, notif_coid, SIGEV_PULSE_PRIO_INHERIT, CODE_TIMER,0);

	if (timer_create(CLOCK_REALTIME, &event, periodicTimer) == -1) {
		perror("Main: 	ошибка timer_create\n");
		exit(EXIT_FAILURE);
	}

	// установить интервал срабатывания периодического таймера тика в системном времени
	setTimerProps(periodicTimerStruct);
}

/*
 * функция установки параметров таймера
 */
void setPeriodicTimerTick(long nsec, int sec) {
	timer.tick_nsec = nsec;
	timer.tick_sec = sec;

	setTimerProps(&timer.periodicTick);
	timer_settime(timer.periodicTimer, 0, &timer.periodicTick, NULL);
}
