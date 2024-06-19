/*
 * server.cc
 *
 *  Created on: 18.06.2024
 *      Author: qnx
 */
#include "server.h"
#include "utils.h"

#define REG_CHAN 	"RTS_registration_channel"

const int REG_TYPE = 1; //				��� ��������� ��� �����������
const int SEND_TIME_TYPE = 2; //		��� ��������� ��� �������� �������

typedef struct _pulse msg_header_t; // ����������� ��� ��� ��������� ��������� ��� � ��������
typedef struct _TIME_hadnle {
	msg_header_t hdr;
	string name; //		��� �����
	int pid; //			id ��������
	int tid; //			id ����
} TIME_handle_t;

//���� ��� ����������� �����
tdb_ms_t TDB;

void* server(void*) {
	cout << "Server: 	Server starting..." << endl;
	name_attach_t *attach;
	TIME_handle_t msg;
	int rcvid;

	//	�������� ������������ ������
	if ((attach = name_attach(NULL, REG_CHAN, 0)) == NULL) {
		cout << "Server: 	Channel create ERROR" << endl;
		exit(EXIT_FAILURE);
	}

	//	���� �������� ����������� ��������� � ����������� �����
	while (!ShutDown) {
		cout << "Server: 	Waiting for connection..." << endl;
		rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), NULL);
		if (rcvid == -1) { /* ������, ���������� */
			if (DEBUG) {
				cout << "Server: 	rcvid = "<< rcvid << "\nServer: 	break;\n";
				cout << "msg.hdr.scoid " << msg.hdr.scoid << endl;
				cout << "msg.hdr.code " << msg.hdr.code << endl;
			}
			break;
		}
		//	��������� ��������� msg
		if (rcvid == 0) { // 				������� �������
			switch (msg.hdr.code) {
			case _PULSE_CODE_DISCONNECT:
				//			������ �������� ��� ����� ��������� ����� (������ name_close() ��� ������� ������ name_open()
				//			� ������ ������) ��� ��������������

				if (DEBUG) {
					cout << "Server debug: 	receive PULSE CODE DISCONNECT"
							<< endl;
				}
				ConnectDetach(msg.hdr.scoid); // 	���������� ���������� ���� � ������� �������
				name_detach(attach, 0); // 			������� ��� ��������
				if (DEBUG) {
					cout
							<< "Server debug: 	receive _PULSE_CODE_DISCONNECT and terminated"
							<< endl;
				}

				break;
			case _PULSE_CODE_UNBLOCK:
				//				������ ����� ��������������� (������� ������ ��� ���� �������).
				//				������� ���������� ������� ������� � ������� ������
				cout << "Server debug: 	receive PULSE CODE UNBLOCK" << endl;
				break;
			default:
				//				������ ������� �� ������-�� �������� ��� �� ����
				//				(PULSE_CODE_COIDDEATH ��� _PULSE_CODE_THREADDEATH)
				cout
						<< "Server debug: 	receive PULSE CODE COIDDEATH or PULSE CODE THREADDEATH"
						<< endl;
				break;
			}
			continue; // ��������� ������� �������� �����
		}
		if (msg.hdr.type == _IO_CONNECT) { // �������� ��������� ���� _IO_CONNECT - ������ �������� name_open(),
			// ����� ����� EOK
			cout << "receive IO CONNECT" << endl;
			MsgReply(rcvid, EOK, NULL, 0);
			continue;
		}
		if (msg.hdr.type == REG_TYPE) { //										�������� ������ ��� �����������
			cout << endl << "Server    : Registration request received" << endl;
			regTDB(msg.name, msg.pid, msg.tid); // 						���������������� �����
			MsgReply(rcvid, EOK, 0, 0);
		}
	}
	printf("Server: 	Server is shutting down \n");
	cout << "SERVER INFO: 	ShutDown = " << ShutDown << endl;
	return NULL;

}

//-----------------------------------------------------------------
//*	������� �����������
//*	��������� ������ � �����
//-----------------------------------------------------------------
void regTDB(string name, int pid, int tid) {
	TDB.name = name;
	TDB.pid = pid;
	TDB.tid = tid;

	cout << "Server: " << endl;
	cout << "----------TDB_MS REGISTRATION INFO----------" << endl;
	cout << "	Name = " << TDB.name << endl;
	cout << "	PID = " << TDB.pid << endl;
	cout << "	TID = " << TDB.tid << endl;
	cout << "--------------------------------------------" << endl << endl;
}
