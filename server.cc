/*
 * server.cc
 *
 *  Created on: 18.06.2024
 *      Author: qnx
 */
#include "server.h"
#include "utils.h"

#define REG_CHAN 	"RTS_registration_channel"

const int REG_TYPE = 101; //	��� ��������� ��� �����������

typedef struct _pulse msg_header_t; // 		����������� ��� ��� ��������� ��������� ��� � ��������
typedef struct _registration_hadnle { // 	��������� ��������� � ������� �������������� �����
	msg_header_t hdr;
	string name; //	��� �����
	int pid; //		id ��������
	int tid; //		id ����
	int nd;//		���������� ����
} registration_handle_t;

tdb_map_buffer_t tdb_map; //	����� ��� ����������� �����

/*
 * ���� ������� ����������� �����
 */
void* server(void*) {
	cout << "- - - - Server: 	������ ������� �����������" << endl;
	name_attach_t* attach;
	registration_handle_t msg;
	int rcvid;

	//	�������� ������������ ������
	if ((attach = name_attach(NULL, REG_CHAN, 0)) == NULL) {
		cout << "- - - - Server: 	������ �������� ������������ ������" << endl;
		exit(EXIT_FAILURE);
	}

	//	���� �������� ����������� ��������� � ����������� �����
	while (!shutDown) {
		cout << "- - - - Server: 	�������� ������� �����������" << endl;
		rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), NULL);
		cout << "- - - - Server ������� RCVID = " << rcvid << endl;

		if (rcvid == -1) { /* ������ */
			cout << "- - - - Server: 	������� rcvid = " << rcvid << endl;
			continue;
		}

		if (rcvid == 0) {/* ������� ������� */
			// ��������� ��������� ��������
			switch (msg.hdr.code) {
			case _PULSE_CODE_DISCONNECT:
				ConnectDetach(msg.hdr.scoid); /* ���������� ���������� ���� � ������� �������*/
				name_detach(attach, 0); /* ������� ��� �������� */
				printf("Server receive _PULSE_CODE_DISCONNECT and terminated");
				break;
			case _PULSE_CODE_UNBLOCK:
				cout << "- - - - Server _PULSE_CODE_UNBLOCK\n";
				break;
			default:
				cout << "- - - - Server default\n";
				break;
			}
			continue;
		}
		/* ���������� ��������� �� ������� */
		// ��������� ��������� ���������
		if (msg.hdr.type == _IO_CONNECT) {
			cout << "- - - - Server: recieve name_open()\n";
			MsgReply(rcvid, EOK, NULL, 0);
			continue;
		}
		if (msg.hdr.type > _IO_BASE && msg.hdr.type <= _IO_MAX) { //�������� IO-��������� �� ����
			cout << "_IO_BASE < hdt.type << _IO_MAX\n";
			MsgError(rcvid, ENOSYS );
			continue;
		}
		// ��������� ��������� � �����������
		if (msg.hdr.code == REG_TYPE) {
			cout << "- - - - Server: ������� ��������� REG_TYPE \n";
			regTDB(msg.name, msg.pid, msg.tid, msg.nd);

			// TODO: ��������� ���������� ������

			MsgReply(rcvid, EOK, 0, 0);
			continue;
		}
	}

	printf("- - - - Server: Server is shutting down \n");
	cout << "- - - - Server: ShutDown = " << shutDown << endl;
	return NULL;
}

/*
 * ������� �����������
 */
bool regTDB(string name, int pid, int tid, int nd) {

	if (tdb_map.buf.count(name) > 0) {
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
		cout << "Server: " << endl;
		cout << "----------TDB_MS REGISTRATION INFO---------" << endl;
		cout << "				SUCCESS						" << endl;
		cout << "-------------------------------------------" << endl << endl;
		return true;
	}
	return false;
}
