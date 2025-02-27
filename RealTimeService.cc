/*
 * server.cc
 *
 *  Created on: 18.06.2024
 *      Author: ���� �.�.
 *
 *  ������ ���
 *  ������� ��:
 *  	RealTimeService
 *  	server
 *  	notification
 *  ����� ���� ��� �����������
 */
#include "utils.h"
#include "RealTimeService.h"

#define NOTIF_CHAN 	"notification_channel"

bool shutDown = false; //	���� ��������� ������

// ��������� ��� �������� ���������� �������
struct Clock {
	long tick_nsec; // 	������������ ������ ���� � ������������
	int tick_sec;// 	������������ ������ ���� � ��������
	timer_t periodicTimer;// ���������� �������
	struct itimerspec periodicTick;// �������� ������������ �������������� ������� � 1 ���
	int Time; // 		����� �������� ���� ����� ���
};

Clock timer; // ����� ���������� �������
timer_t timerId = -10;// ������, "-10" �������� �� ���������� �������

const int CODE_TIMER = 1;//��� ���������-�������� �� �������

pthread_barrier_t notif_barrier; // ������ ������������� ����������� � ������ ��� ����������� �������

//������� ���������� � ������� �������������� ������� ����������� ��������� �� ��������� ���� ����� ���
void setPeriodicTimer(timer_t* periodicTimer,
		struct itimerspec* periodicTimerStruct, int notif_coid);
//������� ��������� ���������� �������
void setTimerProps(struct itimerspec* periodicTimerStruct);

//������� ���� ���������� ���
void menu_showList(); // 		���������� ������ ������
void menu_changeTick(); //		�������� ��� �������

/*
 * ���� main
 * ��������� ���� ������� �����������
 * ��������� ���� ����������� �� ��������� ���� �������
 * ��������� ������
 * ������������ ���� ������������ � �������� ����� ����
 */
int main() {

	timer.tick_nsec = 0; // ��������� �������� ����
	timer.tick_sec = 5;// 	��������� �������� ����
	//������������� ������� ������ ������������������ �����
	if (pthread_mutex_init(&tdb_map.Mutex, NULL) != EOK) {
		std::cout << "main: ������ pthread_mutex_init(): " << strerror(errno)
				<< std::endl;
		return EXIT_FAILURE;
	}

	//������ ������� �����������
	if ((pthread_create(NULL, NULL, &server, NULL)) != EOK) {
		perror("main: error server thread launch\n");
	};

	pthread_barrier_init(&notif_barrier, NULL, 2); // ������������� ������� ����������� � ������ ����������� ��� �������

	//������ ������ �����������
	if ((pthread_create(NULL, NULL, &notification, NULL)) != EOK) {
		perror("main: error notification thread launch\n");
	}

	pthread_barrier_wait(&notif_barrier);// �������� �������� ������ �����������
	pthread_barrier_destroy(&notif_barrier);// ������������ ��������

	int notif_coid = 0;
	//������������ � ������ �����������
	if ((notif_coid = name_open(NOTIF_CHAN, 0)) == -1) {
		cerr << "main: error name_open(NOTIF_CHAN). errno "<< errno << endl;
		exit(EXIT_FAILURE);
	}

	//	����������� ������
	setPeriodicTimer(&timer.periodicTimer, &timer.periodicTick, notif_coid);
	// 	������ �������������� �������������� �������!
	timer_settime(timer.periodicTimer, 0, &timer.periodicTick, NULL);
	timerId = timer.periodicTimer;

	cout << "#main: Enter command:\n";
	cout << "#main: Enter 0 to show menu list\n";
	// ��������� ����� ������������
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
 * ������� ���� ���������� �������� ���� �������
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
 * ������� ����������� ����
 */
void menu_showList() {
	cout << "-----MENU-----\n";
	cout << "1. Change timer tick\n";
	cout << "-----����-----\n";
}

/*
 * ������� ��������� ���������� �������
 */
void setTimerProps(struct itimerspec* periodicTimerStruct) {
	periodicTimerStruct->it_value.tv_sec = timer.tick_sec;
	periodicTimerStruct->it_value.tv_nsec = timer.tick_nsec;
	periodicTimerStruct->it_interval.tv_sec = timer.tick_sec;
	periodicTimerStruct->it_interval.tv_nsec = timer.tick_nsec;
}

/*
 * ������� ���������� � ������� �������������� ������� ����������� ��������� �� ��������� ���� ����� ���
 */
void setPeriodicTimer(timer_t* periodicTimer,
		struct itimerspec* periodicTimerStruct, int notif_coid) {
	struct sigevent event;

	// 	������� ��� ������ �����������
	SIGEV_PULSE_INIT(&event, notif_coid, SIGEV_PULSE_PRIO_INHERIT, CODE_TIMER,0);

	if (timer_create(CLOCK_REALTIME, &event, periodicTimer) == -1) {
		perror("Main: 	error timer_create\n");
		exit(EXIT_FAILURE);
	}

	// ���������� �������� ������������ �������������� ������� ���� � ��������� �������
	setTimerProps(periodicTimerStruct);
}

