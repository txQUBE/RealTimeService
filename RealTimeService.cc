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
#include <pthread.h>

#define NOTIF_CHAN 	"notification_channel"

bool shutDown = false; //	���� ��������� ������
bool tick_changed = false; // ����-������ ��������� ���� �������

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

void* server(void*);
void* notification(void*);

//������� ���������� � ������� �������������� ������� ����������� ��������� �� ��������� ���� ����� ���
void setPeriodicTimer(timer_t* periodicTimer,
		struct itimerspec* periodicTimerStruct, int notif_coid);
//������� ��������� ���������� �������
void setTimerProps(struct itimerspec* periodicTimerStruct);
//������� ��������� ���������� ��� ����
void set_thread_priority(pthread_t thread, int priority);

//������� ���� ���������� ���
void menu_showList(); // 		���������� ������ ������
void menu_changeTick(); //		�������� ��� �������
void menu_showTdbMsList();// 	����������� ������ ������������������ �����

/*
 * ���� main
 * ��������� ���� ������� �����������
 * ��������� ���� ����������� �� ��������� ���� �������
 * ��������� ������
 * ������������ ���� ������������ � �������� ����� ����
 */
int main() {
	pthread_t server_thread, notification_thread;

	timer.tick_nsec = 0; // ��������� �������� ����
	timer.tick_sec = 5;// 	��������� �������� ����
	//������������� ������� ������ ������������������ �����

	if (pthread_mutexattr_init(&tdb_map.attr) != EOK) {
		cerr << "error pthread_mutexattr_init errno: " << errno << endl;
	}
	// ���������� �������� ��� ��������� ������������ �����
	if (pthread_mutexattr_setprotocol(&tdb_map.attr, PTHREAD_PRIO_INHERIT)
			!= EOK) {
		cerr << "error pthread_mutexattr_init errno: " << errno << endl;
	}
	if (pthread_mutex_init(&tdb_map.mutex, NULL) != EOK) {
		std::cout << "main: ������ pthread_mutex_init(): " << strerror(errno)
				<< std::endl;
		return EXIT_FAILURE;
	}

	//������ ������� �����������
	if ((pthread_create(&server_thread, NULL, server, NULL)) != EOK) {
		perror("main: error server thread launch\n");
	};

	pthread_barrier_init(&notif_barrier, NULL, 2); // ������������� ������� ����������� � ������ ����������� ��� �������

	//������ ������ �����������
	if ((pthread_create(&notification_thread, NULL, notification, NULL)) != EOK) {
		perror("main: error notification thread launch\n");
	}
	//���������� ������� ��������� ��� ���� �����������
	set_thread_priority(notification_thread, 10);

	pthread_barrier_wait(&notif_barrier);// �������� �������� ������ �����������
	pthread_barrier_destroy(&notif_barrier);// ������������ ��������

	int notif_coid = 0;
	//������������ � ������ �����������
	if ((notif_coid = name_open(NOTIF_CHAN, 0)) == -1) {
		cerr << "main: error name_open(NOTIF_CHAN). errno " << errno << endl;
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
			menu_showTdbMsList();
			break;
		}
	}

	pthread_mutex_destroy(&tdb_map.mutex);
	cout << "#main: EXIT_SUCCESS" << endl;
	return EXIT_SUCCESS;
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

		// ����� �������� �����
		timer.Time = 0;

		tick_changed = true; // ������ �� ��������� ����
		cout << "New timer tick: " << timer.tick_sec << " sec "
				<< timer.tick_nsec << " nsec.";
	}

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

/*
 * ������� ��������� ���������� ��� ����
 */
void set_thread_priority(pthread_t thread, int priority) {
	struct sched_param param;
	param.sched_priority = priority;
	pthread_setschedparam(thread, SCHED_RR, &param); // ���������� �������� Round Robin
}

/*
 * ������� ����������� ����
 */
void menu_showList() {
	cout << "-----MENU-----" << endl;
	cout << "1. Change timer tick" << endl;
	cout << "2. Show registered tdb list" << endl;
	cout << "-----����-----" << endl;
}

void menu_showTdbMsList(){
	pthread_mutex_lock(&tdb_map.mutex);
	cout << "#main: REGISTERED TDBMS LIST:" << endl;
	for (map<string, tdb_ms_t>::iterator it = tdb_map.buf.begin(); it
					!= tdb_map.buf.end(); ++it) {
		cout<< it->first << endl;
	}
	pthread_mutex_lock(&tdb_map.mutex);
}


