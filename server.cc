/*
 * server.cc
 *
 *  Created on: 18.06.2024
 *      Author: ���� �.�.
 *
 *  ���� ������� ����������� ����� � ���
 */
#include "server.h"
#include "utils.h"

#define REG_CHAN "rts_registration_channel" // ��� ������ �����������
const int REG_TYPE = 101; //	��� ��������� ��� �����������

typedef struct _pulse msg_header_t; // 		����������� ��� ��� ��������� ��������� ��� � ��������
typedef struct _registration_hadnle { // 	��������� ��������� � ������� �������������� �����
	msg_header_t hdr;
	string name; //		��� �����
	int pid; //			id ��������
	pthread_t tid; //	id ����
	int nd;//			���������� ����
} registration_handle_t;

tdb_map_buffer_t tdb_map; //	����� ��� ����������� �����

// ��������� ��� �������� ���������� �������
struct clock {
	long tick_nsec; // 	������������ ������ ���� � ������������
	int tick_sec;// 	������������ ������ ���� � ��������
	timer_t periodicTimer;// ���������� �������
	struct itimerspec periodicTick;// �������� ������������ �������������� ������� � 1 ���
	int Time; // 		����� �������� ���� ����� ���
};

// ��������� �������
// ������� ����������� ����� � ���
bool reg_tdb(string name, int pid, pthread_t tid, int nd);
// ������� �������� ������������� ����� � ������ �������
bool isRegistered(string name);
void sendTickParam(int coid);

/*
 * ���� ������� ����������� �����
 * ��������� ����� ��� �����������
 * ������������ ��������� ���������, �������� � ��������� ��� ����������� �����
 */
void* server(void*) {
	cout << "- - - - Server: starting..." << endl;

	if (pthread_mutexattr_init(&tdb_map.attr) != EOK) {
		cerr << "error pthread_mutexattr_init errno: " << errno << endl;
		exit(0);
	}
	// ���������� �������� ��� ��������� ������������ �����
	if (pthread_mutexattr_setprotocol(&tdb_map.attr, PTHREAD_PRIO_INHERIT)
			!= EOK) {
		cerr << "error pthread_mutexattr_init errno: " << errno << endl;
		exit(0);
	}
	if (pthread_mutex_init(&tdb_map.mtx, NULL) != EOK) {
		std::cout << "error pthread_mutex_init(): " << strerror(errno)
				<< std::endl;
		exit(0);
	}

	name_attach_t* attach;
	registration_handle_t msg;
	int rcvid;

	//	�������� ������������ ������
	if ((attach = name_attach(NULL, REG_CHAN, 0)) == NULL) {
		cerr << "- - - - Server: 	error name_attach(). errno:" << errno << endl;
		return NULL;
	}

	//	���� �������� ����������� ��������� � ����������� �����
	while (!shutDown) {
		cout << "- - - - Server: wait for msg..." << endl;
		rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), NULL);

		if (rcvid == -1) { /* ������ */
			if (errno == ENOTCONN) {
				// ������ ����������
				printf("- - - - Server: Client disconected.\n");
				continue;
			} else {
				perror("MsgReceive");
				break;
			}
		}

		if (rcvid == 0) {/* ������� ������� */
			// ��������� ��������� ��������
			switch (msg.hdr.code) {
			case _PULSE_CODE_DISCONNECT:
				ConnectDetach(msg.hdr.scoid); /* ���������� ���������� ���� � ������� �������*/
				printf("- - - - Server:  _PULSE_CODE_DISCONNECT\n");
				break;
			case _PULSE_CODE_UNBLOCK:
				cout << "- - - - Server: _PULSE_CODE_UNBLOCK\n";
				break;
			default:
				cout << "- - - - Server: default\n";
				break;
			}
			continue;
		}
		/* ���������� ��������� �� ������� */
		// ��������� ��������� ���������
		if (msg.hdr.type == _IO_CONNECT) {
			MsgReply(rcvid, EOK, NULL, 0);
			continue;
		}
		if (msg.hdr.type > _IO_BASE && msg.hdr.type <= _IO_MAX) { //�������� IO-��������� �� ����
			MsgError(rcvid, ENOSYS );
			continue;
		}
		// ��������� ��������� � �����������
		if (msg.hdr.code == REG_TYPE) {
			// ��������� ������ ������
			pthread_mutex_lock(&tdb_map.mtx);
			// ������� �����������
			bool success = reg_tdb(msg.name, msg.pid, msg.tid, msg.nd);
			pthread_mutex_unlock(&tdb_map.mtx);
			// ����� �������
			if (success) {
			    MsgReply(rcvid, EOK, 0, 0); // �������� �����������
			} else {
			    MsgReply(rcvid, EINVAL, 0, 0); // ������ �����������
			}
			continue;
		}
	}

	//������� ��������
	name_detach(attach, 0);
	pthread_mutexattr_destroy(&tdb_map.attr);
	pthread_mutex_destroy(&tdb_map.mtx);
	cout << "- - - - Server: EXIT_SUCCESS" << endl;
	return EXIT_SUCCESS;
}

/*
 * ������� �����������
 * ��������� ��������� ��� �������� ������ ����� � ��������� � �����
 */
bool reg_tdb(string name, int pid, pthread_t tid, int nd) {

	if (isRegistered(name)) {
		return false; // ����������� �� ���������, ����� � ����� ������ ��� ����������������
	}

	// ��������� ��� �������� ������ �����
	tdb_ms_t tdb;
	tdb.pid = pid;
	tdb.tid = tid;
	tdb.nd = nd;
	tdb_map.buf[name] = tdb;

	bool isRegistered;
	isRegistered = tdb_map.buf.count(name) > 0;
	// �������� ������ �����������
	if (isRegistered) {
		cout << "- - - - Server: "<< name << " REGISTRATION SUCCESS" << endl;
		return true;
	}
	return false;
}

/*
 * ������� �������� �����������
 * ��������� ������������� ����� � ������ �� � �����
 */
bool isRegistered(string name) {
	return tdb_map.buf.count(name) > 0;
}
