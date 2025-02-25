/*
 * server.cc
 *
 *  Created on: 18.06.2024
 *      Author: qnx
 */
#include "server.h"
#include "utils.h"

#define REG_CHAN 	"RTS_registration_channel"

const int REG_TYPE = 201; //				��� ��������� ��� �����������
const int SEND_TIME_TYPE = 2; //		��� ��������� ��� �������� �������

typedef struct _pulse msg_header_t; // ����������� ��� ��� ��������� ��������� ��� � ��������
typedef struct _TIME_hadnle {
	msg_header_t hdr;
	string name; //		��� �����
	int pid; //			id ��������
	int tid; //			id ����
} registration_handle_t;

//����� ��� ����������� �����
tdb_ms_t registered_tdb;

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

		if (rcvid == -1) { /* ������, ���������� */
			cout << "- - - - Server: 	������� rcvid = " << rcvid << endl;
		}

		if (rcvid == 0) {/* ������� ������� */
			switch (msg.hdr.code) {
			case _PULSE_CODE_DISCONNECT:
				/* ������ �������� ��� ����� ��������� ����� (������ name_close() ��� ������� ������
				 name_open() � ������ ������) ��� �������������� */
				ConnectDetach(msg.hdr.scoid);
				/* ���������� ���������� ���� � ������� �������*/
				name_detach(attach, 0);
				/* ������� ��� �������� */
				printf("Server receive _PULSE_CODE_DISCONNECT and terminated");
				return EXIT_SUCCESS;
			case _PULSE_CODE_UNBLOCK:
				/* ������ ����� ��������������� (������� ������ ��� ���� �������). ������� ����������
				 ������� ������� � ������� ������ */
				break;
			default:
				/* ������ ������� �� ������-�� �������� ��� �� ����
				 (PULSE_CODE_COIDDEATH ��� _PULSE_CODE_THREADDEATH) */
				break;
			}
			continue;// ��������� ������� �������� �����
		}
		/* ���������� ��������� �� ������� */
		if (msg.hdr.type == _IO_CONNECT) {
			cout << "- - - - Server: recieve name_open()\n";
			MsgReply(rcvid, EOK, NULL, 0);
			continue; // ��������� ������� �������� �����
		}
	}
	printf("- - - - Server: Server is shutting down \n");
	cout << "- - - - Server: ShutDown = " << shutDown << endl;
}

//-----------------------------------------------------------------
//*	������� �����������
//*	��������� ������ � �����
//-----------------------------------------------------------------
void regTDB(string name, int pid, int tid) {
	registered_tdb.name = name;
	registered_tdb.pid = pid;
	registered_tdb.tid = tid;

	cout << "Server: " << endl;
	cout << "----------TDB_MS REGISTRATION INFO----------" << endl;
	cout << "	Name = " << registered_tdb.name << endl;
	cout << "	PID = " << registered_tdb.pid << endl;
	cout << "	TID = " << registered_tdb.tid << endl;
	cout << "--------------------------------------------" << endl << endl;
}
