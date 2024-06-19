/*
 * notif.cc
 *
 *  Created on: 18.06.2024
 *      Author: qnx
 */

#include "notif.h"
#include "utils.h"

#define NOTIF_CHAN 	"notification_channel"

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

	cout << "Notif: 	Channel starting..." << endl;
	name_attach_t *attach;
	TimerSig tsig;

	//	�������� ������������ ������
	if ((attach = name_attach(NULL, NOTIF_CHAN, 0)) == NULL) {
		cout << "Notif: 	Channel create ERROR" << endl;
		exit(EXIT_FAILURE);
	}

	if (DEBUG) {
		cout << "Notif debug: 	Channel is running... " << endl;
	}

	//	���� ��������� �������
	for (;;) {
		cout << endl << "Notif: 	waiting for timer pulse..." << endl;
		int rcvid = MsgReceive(attach->chid, &tsig, sizeof(tsig), NULL);
		if (rcvid == 0) { // ������� ������� �� �������
			gotAPulse();
			if (TDB.pid == 0) {
				cout << "Notif: 	No TDB_MS registred" << endl;
			} else {
				cout << "Notif: 	Sending SIGUSR1" << endl << endl;
				//	������� ������ � ��������� ���� � �����
				if (SignalKill(ND, TDB.pid, TDB.tid, SIGUSR1, SI_USER, 0) == -1) {
					cout << "Notif: 	SignalKill ERROR " << endl;
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
	cout << endl << "Timer pulse received " << ctime(&now);
	cout << "CODE_TIMER = " << (int) tsig.M_pulse.code << endl;
	cout << "Data: " << tsig.M_pulse.value.sival_int << endl;
}

