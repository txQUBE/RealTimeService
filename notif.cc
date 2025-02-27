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
#define TICKPAS_SIG SIGRTMIN // ������ � ��������� ����
#define TICKUPDATE_SIG SIGRTMIN + 1 // ������ �� ���������� ���������� ����
#define TICKUPDATE_CODE 2 // ��� ��������� � ������� ���������� �������

// ��������� ��� �������� ���������� �������
struct Clock {
	long tick_nsec; // 	������������ ������ ���� � ������������
	int tick_sec;// 	������������ ������ ���� � ��������
	timer_t periodicTimer;// ���������� �������
	struct itimerspec periodicTick;// �������� ������������ �������������� ������� � 1 ���
	int Time; // 		����� �������� ���� ����� ���
};

extern Clock timer; // ����� �������
extern pthread_barrier_t notif_barrier;
extern bool tick_changed; // ����-������ ��������� ���� �������

typedef struct _pulse msg_header_t; // 		����������� ��� ��� ��������� ��������� ��� � ��������
//��������� ��� �������� ��������� � ����������� �������
struct local_time_msg_t {
	msg_header_t hdr;
	long tick_nsec; // 	������������ ������ ����, ������������ � ������������
	int tick_sec;// 	������������ ������ ����, ������������ � ��������
	int Time; // 		����� �������� ���� ����� ���
};

//��� ������ �������� �� �������
union TimerSig {
	_pulse M_pulse;
} tsig;

//��� �������� ������� ������� ������� ����
time_t now;

//��������� �������
void sendNotifTickPassed(); // ������� �������� ������� � ��������� ����
void sendTickParam(int coid); // ������� �������� ����������� �������� ���� �������
void sendTickUpdate();// ������� �������� ���������� ���������� �������
void gotAPulse(void);//		������� ��������� ������� �������

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

		//��������� ������ ��������� ���������� ���� �������
		if (tick_changed) {
			sendTickUpdate();
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
	pthread_mutex_lock(&tdb_map.mutex);

	if (tdb_map.buf.empty()) {
		cout << "- Notif: no registered TDB_MS" << endl;
	} else {
		cout << "- Notif: send SIGUSR1" << endl << endl;
		// �������� ������ � ������������������� �����
		for (map<string, tdb_ms_t>::iterator it = tdb_map.buf.begin(); it
				!= tdb_map.buf.end(); ++it) {
			// ������� ������ � ��������� ���� � �����
			if (SignalKill(0, it->second.pid, it->second.tid, TICKPAS_SIG,
					SI_USER, 0) == -1) {
				if (errno == ESRCH) {
					// ������� �� ������ ���������� �������
					cout << "- Notif: erase " << it->first << endl;
					tdb_map.buf.erase(it->first);
				} else
					cerr << "- Notif: error SignalKill errno: " << strerror(
							errno) << endl;
			}
		}
	}
	pthread_mutex_unlock(&tdb_map.mutex);
}

void sendTickUpdate() {
	pthread_mutex_lock(&tdb_map.mutex);

	for (map<string, tdb_ms_t>::iterator it = tdb_map.buf.begin(); it
			!= tdb_map.buf.end(); ++it) {
		// ������� ������ �� ��������� ���� ������� � ��������� ��������� � ���������� �����
		if (SignalKill(0, it->second.pid, it->second.tid, TICKUPDATE_SIG,
				SI_USER, 0) != -1) {
			int coid;
			//������������ � ������������ ������ ����� ��� �������� ���������
			if ((coid = name_open(it->first.c_str(), 0)) == -1) {
				cerr << "- Notif: error sendTickUpdate " << it->first<<" errno: "
						<< errno << endl;
			}
			//��������� ��������� �������
			sendTickParam(coid);
			cout << "- Notif: sent TICKUPDATE to "<< it->first.c_str() << endl;
			name_close(coid);
		}
	}
	pthread_mutex_unlock(&tdb_map.mutex);
}

void sendTickParam(int coid) {
	local_time_msg_t msg;
	msg.hdr.type = 0x00;
	msg.hdr.subtype = 0x00;
	msg.hdr.code = TICKUPDATE_CODE;
	msg.Time = timer.Time;
	msg.tick_nsec = timer.tick_nsec;
	msg.tick_sec = timer.tick_sec;

	int reply = MsgSend(coid, &msg, sizeof(msg), NULL, 0);
	if (reply != EOK){
		cerr << "- - - - Server: MsgSend error, errno: " << errno << endl;
	}
}

