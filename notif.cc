/*
 * notif.cc
 *
 *  Created on: 18.06.2024
 *      Author: ���� �.�.
 *
 *  ���� ������ ����������� �� ��������� ���� �������
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

//��������� �������
void sendNotifTickPassed(); // ������� �������� ������� � ��������� ����

/*
 * ���� �����������
 * ��������� �������� �� �������
 * ���������� ������ �� ���������� ���� � ����������������� ����
 */
void* notification(void*) {

	cout << "- Notif: ������ ������ �����������" << endl;
	name_attach_t *attach;
	TimerSig tsig;

	//	�������� ������������ ������
	if ((attach = name_attach(NULL, NOTIF_CHAN, 0)) == NULL) {
		cout << "- Notif: ������ �������� ������ �����������" << endl;
		exit(EXIT_FAILURE);
	}

	pthread_barrier_wait(&notif_barrier); // �������� ����������� ������� � ������

	//	���� ��������� �������
	for (;;) {
		cout << endl << "- Notif: �������� ������� �������" << endl;
		int rcvid = MsgReceive(attach->chid, &tsig, sizeof(tsig), NULL);
		if (rcvid == 0) { // ������� ������� �� �������
			gotAPulse();
			sendNotifTickPassed();
		}
	}
	return NULL;
}

/*
 * ������� ��������� ������� �������
 */
void gotAPulse(void) {

	time(&now);//�������� ������� ������ �������
	//������ ������ �������
	cout << endl << "Timer signal received " << ctime(&now);
	cout << "CODE_TIMER = " << (int) tsig.M_pulse.code << endl;
	cout << "Data: " << tsig.M_pulse.value.sival_int << endl;
}

/*
 * ������� �������� ������� � ��������� ����
 */
void sendNotifTickPassed() {
	// ������ ������ � ������������������� �����
	pthread_mutex_lock(&tdb_map.Mutex);

	if (tdb_map.buf.empty()) {
		cout << "- Notif: no registered TDB_MS" << endl;
	} else {
		cout << "- Notif: send SIGUSR1" << endl << endl;
		// �������� ������ � ������������������� �����
		for (map<string, tdb_ms_t>::iterator it = tdb_map.buf.begin(); it
				!= tdb_map.buf.end(); ++it) {
			// ������� ������ � ��������� ���� � �����
			if (SignalKill(0, it->second.pid, it->second.tid, SIGUSR1,
					SI_USER, 0) == -1) {
				if(errno == ESRCH){
					// ������� �� ������ ���������� �������
					cout << "- Notif: erase " << it->first << endl;
					tdb_map.buf.erase(it->first);

				} else
				cerr << "- Notif: error SignalKill errno: "<< strerror(errno) << endl;

			}
		}
	}

	pthread_mutex_unlock(&tdb_map.Mutex);
}

