#include "utils.h"
#include "RealTimeService.h"

#define NOTIF_CHAN 	"notification_channel"

bool ShutDown = false; //		флаг завершить работу
bool DEBUG = true; //			показывать дополнительные сообщения

int tik = 2; // 				***ЗАДАТЬ ТИК

const int CODE_TIMER = 1;// 	сообщение-импульс от таймера
const int ND = 0; //			id узла

//-----------------------------------------------------------------
//*	Нить main
//-----------------------------------------------------------------
int main() {

	int notif_coid = 0;
	//запуск сервера
	if ((pthread_create(NULL, NULL, &server, NULL)) != EOK) {
		perror("Main: 	thread create ERROR\n");
		exit(EXIT_FAILURE);
	};
	if (DEBUG) {
		cout << "Main debug: 	server thread starting complete" << endl;
	}

	//Запуск канала уведомлений
	if ((pthread_create(NULL, NULL, &notification, NULL)) != EOK) {
		perror("Main: 	thread create ERROR\n");
		exit(EXIT_FAILURE);
	}
	sleep(1);

	//подключиться к каналу уведомлений
	if ((notif_coid = name_open(NOTIF_CHAN, 0)) == -1) {
		cout << "Main: 	NOTIF_CHAN connection ERROR" << endl;
		exit(EXIT_FAILURE);
	}

	//	Создание таймера
	struct sigevent event; //				определить для таймера тип уведомления
	// 	импульс для канала уведомлений
	SIGEV_PULSE_INIT(&event, notif_coid, SIGEV_PULSE_PRIO_INHERIT, CODE_TIMER,
			0);
	timer_t timerid; //						ID таймера
	if (timer_create(CLOCK_REALTIME, &event, &timerid) == -1) {
		perror("Main: 	timer create ERROR\n");
		exit(EXIT_FAILURE);
	}

	//	Запуск периодического таймера
	struct itimerspec timer; // 			системный формат времени
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_nsec = 500000000; //	первый импульс через 0.5сек
	timer.it_interval.tv_sec = tik; //		последующие каждые 2сек
	timer.it_interval.tv_nsec = 0;
	timer_settime(timerid, 0, &timer, NULL); // запуск относительного периодического таймера!
	while (!ShutDown) {

	}
	name_close(notif_coid);
}
