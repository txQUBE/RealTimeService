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
#include <sys/mman.h>

#define NOTIF_CHAN 	"notification_channel"
#define RTS_SHM_TIME_NAME "rts_shm_time"
#define MAIN_MARK "#main" // ����� ��� ������ � �������
bool shutDown = false; //	���� ��������� ������
bool tick_changed = false; // ����-������ ��������� ���� �������

// ��������� ��� �������� ���������� �������
typedef struct {
	long tick_nsec; // 	������������ ������ ���� � ������������
	int tick_sec;// 	������������ ������ ���� � ��������
	timer_t periodicTimer;// ���������� �������
	struct itimerspec periodicTick;// �������� ������������ �������������� ������� � 1 ���
	int count; // 		����� �������� ���� ����� ���
} Clock;

Clock timer; // ����� ���������� �������
timer_t timer_exist = -10;// ������, "-10" �������� �� ���������� �������

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

	//������ ������� �����������
	if ((pthread_create(&server_thread, NULL, server, NULL)) != EOK) {
		cerr << MAIN_MARK << "error server thread launch init, errno" << errno
				<< endl;
		exit(EXIT_FAILURE);
	};

	// ���������� ��������� ���� ���� �����������,
	// ��� �������� ������� � ������ ������������������ �����
	set_thread_priority(server_thread, 9);

	pthread_barrier_init(&notif_barrier, NULL, 2); // ������������� ������� ����������� � ������ ����������� ��� �������

	//������ ������ �����������
	if ((pthread_create(&notification_thread, NULL, notification, NULL)) != EOK) {
		cerr << MAIN_MARK << "error notification thread init, errno" << errno
				<< endl;
		exit(EXIT_FAILURE);
	}
	//���������� ������� ��������� ��� ���� �����������
	set_thread_priority(notification_thread, 10);

	pthread_barrier_wait(&notif_barrier);// �������� �������� ������ �����������
	pthread_barrier_destroy(&notif_barrier);// ������������ ��������

	int notif_coid = 0;
	//������������ � ������ �����������
	if ((notif_coid = name_open(NOTIF_CHAN, 0)) == -1) {
		cerr << MAIN_MARK << "error name_open(NOTIF_CHAN), errno " << errno
				<< endl;
		exit(EXIT_FAILURE);
	}

	//	����������� ������
	setPeriodicTimer(&timer.periodicTimer, &timer.periodicTick, notif_coid);
	// 	������ �������������� �������������� �������!
	timer_settime(timer.periodicTimer, 0, &timer.periodicTick, NULL);
	timer_exist = timer.periodicTimer;

	cout << MAIN_MARK << "Enter command:" << endl;
	cout << MAIN_MARK << "Enter 0 to show menu list" << endl;
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
		case 9:
			shutDown = true;
			break;
		}
	}

	// ��������� ���������� �����
	pthread_join(server_thread, NULL);
	pthread_join(notification_thread, NULL);
	// ������� ��������
	shm_unlink(RTS_SHM_TIME_NAME);
	timer_delete(timer.periodicTimer);
	cout << MAIN_MARK << "EXIT_SUCCESS" << endl;
	return EXIT_SUCCESS;
}

/*
 * ������� ���� ���������� �������� ���� �������
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

		// ����� �������� �����
		timer.count = 0;

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
		cerr << MAIN_MARK << "error timer_create, errno: " << errno << endl;
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

