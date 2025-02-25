#include "utils.h"
#include "RealTimeService.h"

#define NOTIF_CHAN 	"notification_channel"

bool shutDown = false; //		���� ��������� ������
bool DEBUG = true; //			���������� �������������� ���������

struct Clock {
	long tick_nsec; // ������������ ������ ���� � ������������
	int tick_sec;// � ��������
	int Time; // ����� �������� ���� ����� ���
	timer_t periodicTimer;//���������� �������
	struct itimerspec periodicTick;//�������� ������������ �������������� ������� � 1 ���
};

Clock timer; // ����� �������
timer_t timerId = -10;

const int CODE_TIMER = 1;// 	���������-������� �� �������
const int ND = 0; //			id ����

pthread_barrier_t notif_barrier; //������������� ����������� � ������ �����������

//���������� � ������� �������������� ������� ����������� ��������� �� ��������� ���� ����� ���
void setPeriodicTimer(timer_t* periodicTimer,
		struct itimerspec* periodicTimerStruct, int notif_coid);

//menu
void menu_showList();
void menu_changeTick();
void setPeriodicTimerSeconds(long nsec, int sec);

//-----------------------------------------------------------------
//*	���� main
//-----------------------------------------------------------------
int main() {

	timer.tick_nsec = 0; //
	timer.tick_sec = 10;
	//������������� ������� ������ ������������������ �����
	if (pthread_mutex_init(&registered_tdb.Mutex, NULL) != EOK) {
		std::cout << "main: ������ pthread_mutex_init(): " << strerror(errno)
				<< std::endl;
		return EXIT_FAILURE;
	}

	//������ ������� �����������
	if ((pthread_create(NULL, NULL, &server, NULL)) != EOK) {
		perror("main: ������ ������� �������\n");
	};

	pthread_barrier_init(&notif_barrier, NULL, 2);

	//������ ������ �����������
	if ((pthread_create(NULL, NULL, &notification, NULL)) != EOK) {
		perror("main: ������ ������� ���� �����������\n");
	}

	pthread_barrier_wait(&notif_barrier);// �������� �������� ������ �����������

	int notif_coid = 0;
	//������������ � ������ �����������
	if ((notif_coid = name_open(NOTIF_CHAN, 0)) == -1) {
		cout << "main: ������ ����������� � ������ �����������" << endl;
		exit(EXIT_FAILURE);
	}

	setPeriodicTimer(&timer.periodicTimer, &timer.periodicTick, notif_coid);
	timer_settime(timer.periodicTimer, 0, &timer.periodicTick, NULL); // ������ �������������� �������������� �������!
	timerId = timer.periodicTimer;

	cout << "������� �������:\n";
	cout << "������� 0 ����� ������� ������ ������\n";
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

void menu_changeTick() {
	if (timerId == -10) {
		cout << "������ �� �������\n";
	} else {
		cout << "������� ����� �������� ���� � ������������:\n";
		int newTick_nsec;
		cin >> newTick_nsec;

		cout << "������� ����� �������� ���� � ��������:\n";
		int newTick_sec;
		cin >> newTick_sec;

		setPeriodicTimerSeconds(newTick_nsec, newTick_sec);
	}
	cout << "����� ��� �������: " << timer.tick_sec << " ��� "
			<< timer.tick_nsec << " ��.";
}

void menu_showList() {
	cout << "-----����-----\n";
	cout << "1. �������� ��� �������\n";
	cout << "-----����-----\n";
}

// ������� �������� ������� ���� � ������������ ���������
void setPeriodicTimer(timer_t* periodicTimer,
		struct itimerspec* periodicTimerStruct, int notif_coid) {
	struct sigevent event;

	// 	������� ��� ������ �����������
	SIGEV_PULSE_INIT(&event, notif_coid, SIGEV_PULSE_PRIO_INHERIT, CODE_TIMER,0);

	if (timer_create(CLOCK_REALTIME, &event, periodicTimer) == -1) {
		perror("Main: 	������ timer_create\n");
		exit(EXIT_FAILURE);
	}

	// ���������� �������� ������������ �������������� ������� ���� � ��������� �������
	periodicTimerStruct->it_value.tv_sec = timer.tick_sec;
	periodicTimerStruct->it_value.tv_nsec = timer.tick_nsec; // �������� ������� ��������
	periodicTimerStruct->it_interval.tv_sec = timer.tick_sec;
	periodicTimerStruct->it_interval.tv_nsec = timer.tick_nsec;
}

void setPeriodicTimerSeconds(long nsec, int sec) {
	timer.tick_nsec = nsec;
	timer.tick_sec = sec;
	struct itimerspec* periodicTimerStruct = &timer.periodicTick;
	periodicTimerStruct->it_interval.tv_sec = timer.tick_sec;
	periodicTimerStruct->it_interval.tv_nsec = timer.tick_nsec;
	timer_settime(timer.periodicTimer, 0, &timer.periodicTick, NULL);
}
