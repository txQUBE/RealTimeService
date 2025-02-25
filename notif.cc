/*
 * notif.cc
 *
 *  Created on: 18.06.2024
 *      Author: qnx
 */

#include "notif.h"
#include "utils.h"

#define NOTIF_CHAN 	"notification_channel"

extern pthread_barrier_t notif_barrier;

//��� �������� ������� ������� ������� ����
time_t now;

//��� ������ �������� �� �������
union TimerSig {
	_pulse M_pulse;
} tsig;

//-----------------------------------------------------------------
//*	���� �����������
//* ��������� �������� �� �������
//*	���������� ������ �� ���������� ���� � ����������������� ����
//----------------------------------------------------------------
void* notification(void*) {

	cout << "- Notif: ������ ������ �����������" << endl;
	name_attach_t *attach;
	TimerSig tsig;

	//	�������� ������������ ������
	if ((attach = name_attach(NULL, NOTIF_CHAN, 0)) == NULL) {
		cout << "- Notif: ������ �������� ������ �����������" << endl;
		exit(EXIT_FAILURE);
	}

	pthread_barrier_wait(&notif_barrier); //����� ������������ � ������

	//	���� ��������� �������
	for (;;) {
		cout << endl << "- Notif: �������� ������� �������" << endl;
		int rcvid = MsgReceive(attach->chid, &tsig, sizeof(tsig), NULL);
		if (rcvid == 0) { // ������� ������� �� �������
			gotAPulse();
			if (registered_tdb.pid == 0) {
				cout << "- Notif: ��� ������������������ �����" << endl;
			} else {
				cout << "- Notif: �������� SIGUSR1" << endl << endl;
				//	������� ������ � ��������� ���� � �����
				if (SignalKill(ND, registered_tdb.pid, registered_tdb.tid, SIGUSR1, SI_USER, 0) == -1) {
					cout << "- Notif: ������ SignalKill " << endl;
				}
			}
		}
	}
	return NULL;
}

//-----------------------------------------------------------------
//* ������� ��������� ������� �������
//-----------------------------------------------------------------
void gotAPulse(void) {

	time(&now);//�������� ������� ������ �������
	//������ ������ �������
	cout << endl << "������� ������ ������� " << ctime(&now);
	cout << "CODE_TIMER = " << (int) tsig.M_pulse.code << endl;
	cout << "Data: " << tsig.M_pulse.value.sival_int << endl;
}

