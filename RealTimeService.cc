#include "utils.h"
#include "RealTimeService.h"

#define NOTIF_CHAN 	"notification_channel"

bool ShutDown = false; //		���� ��������� ������
bool DEBUG = true; //			���������� �������������� ���������

int tik = 2; // 				***������ ���

const int CODE_TIMER = 1;// 	���������-������� �� �������
const int ND = 0; //			id ����

//-----------------------------------------------------------------
//*	���� main
//-----------------------------------------------------------------
int main() {

	int notif_coid = 0;
	//������ �������
	if ((pthread_create(NULL, NULL, &server, NULL)) != EOK) {
		perror("Main: 	thread create ERROR\n");
		exit(EXIT_FAILURE);
	};
	if (DEBUG) {
		cout << "Main debug: 	server thread starting complete" << endl;
	}

	//������ ������ �����������
	if ((pthread_create(NULL, NULL, &notification, NULL)) != EOK) {
		perror("Main: 	thread create ERROR\n");
		exit(EXIT_FAILURE);
	}
	sleep(1);

	//������������ � ������ �����������
	if ((notif_coid = name_open(NOTIF_CHAN, 0)) == -1) {
		cout << "Main: 	NOTIF_CHAN connection ERROR" << endl;
		exit(EXIT_FAILURE);
	}

	//	�������� �������
	struct sigevent event; //				���������� ��� ������� ��� �����������
	// 	������� ��� ������ �����������
	SIGEV_PULSE_INIT(&event, notif_coid, SIGEV_PULSE_PRIO_INHERIT, CODE_TIMER,
			0);
	timer_t timerid; //						ID �������
	if (timer_create(CLOCK_REALTIME, &event, &timerid) == -1) {
		perror("Main: 	timer create ERROR\n");
		exit(EXIT_FAILURE);
	}

	//	������ �������������� �������
	struct itimerspec timer; // 			��������� ������ �������
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_nsec = 500000000; //	������ ������� ����� 0.5���
	timer.it_interval.tv_sec = tik; //		����������� ������ 2���
	timer.it_interval.tv_nsec = 0;
	timer_settime(timerid, 0, &timer, NULL); // ������ �������������� �������������� �������!
	while (!ShutDown) {

	}
	name_close(notif_coid);
}
