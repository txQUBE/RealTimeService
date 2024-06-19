/*
 * server.cc
 *
 *  Created on: 18.06.2024
 *      Author: qnx
 */
#include "server.h"
#include "utils.h"

#define REG_CHAN 	"RTS_registration_channel"

const int REG_TYPE = 1; //				тип сообщения для регистрации
const int SEND_TIME_TYPE = 2; //		тип сообщения для отправки времени

typedef struct _pulse msg_header_t; // абстрактный тип для заголовка сообщения как у импульса
typedef struct _TIME_hadnle {
	msg_header_t hdr;
	string name; //		имя СУБТД
	int pid; //			id процесса
	int tid; //			id нити
} TIME_handle_t;

//Поле для запоминания СУБТД
tdb_ms_t TDB;

void* server(void*) {
	cout << "Server: 	Server starting..." << endl;
	name_attach_t *attach;
	TIME_handle_t msg;
	int rcvid;

	//	Создание именованного канала
	if ((attach = name_attach(NULL, REG_CHAN, 0)) == NULL) {
		cout << "Server: 	Channel create ERROR" << endl;
		exit(EXIT_FAILURE);
	}

	//	Цикл ожидания поступления сообщений в именованный канал
	while (!ShutDown) {
		cout << "Server: 	Waiting for connection..." << endl;
		rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), NULL);
		if (rcvid == -1) { /* Ошибка, завершение */
			if (DEBUG) {
				cout << "Server: 	rcvid = "<< rcvid << "\nServer: 	break;\n";
				cout << "msg.hdr.scoid " << msg.hdr.scoid << endl;
				cout << "msg.hdr.code " << msg.hdr.code << endl;
			}
			break;
		}
		//	Обработка системных msg
		if (rcvid == 0) { // 				Получен импульс
			switch (msg.hdr.code) {
			case _PULSE_CODE_DISCONNECT:
				//			Клиент разорвал все ранее созданные связи (вызвал name_close() для каждого вызова name_open()
				//			с именем канала) или терминировался

				if (DEBUG) {
					cout << "Server debug: 	receive PULSE CODE DISCONNECT"
							<< endl;
				}
				ConnectDetach(msg.hdr.scoid); // 	Уничтожить соединение ядра с каналом сервера
				name_detach(attach, 0); // 			Удалить имя процесса
				if (DEBUG) {
					cout
							<< "Server debug: 	receive _PULSE_CODE_DISCONNECT and terminated"
							<< endl;
				}

				break;
			case _PULSE_CODE_UNBLOCK:
				//				Клиент хочет деблокироваться (получен сигнал или истёк таймаут).
				//				Серверу необходимо принять решение о посылке ответа
				cout << "Server debug: 	receive PULSE CODE UNBLOCK" << endl;
				break;
			default:
				//				Пришёл импульс от какого-то процесса или от ядра
				//				(PULSE_CODE_COIDDEATH или _PULSE_CODE_THREADDEATH)
				cout
						<< "Server debug: 	receive PULSE CODE COIDDEATH or PULSE CODE THREADDEATH"
						<< endl;
				break;
			}
			continue; // Завершить текущую итерацию цикла
		}
		if (msg.hdr.type == _IO_CONNECT) { // Получено сообщение типа _IO_CONNECT - клиент выполнил name_open(),
			// нужен ответ EOK
			cout << "receive IO CONNECT" << endl;
			MsgReply(rcvid, EOK, NULL, 0);
			continue;
		}
		if (msg.hdr.type == REG_TYPE) { //										Получены данные для регистрации
			cout << endl << "Server    : Registration request received" << endl;
			regTDB(msg.name, msg.pid, msg.tid); // 						Зарегистрировать СУБТД
			MsgReply(rcvid, EOK, 0, 0);
		}
	}
	printf("Server: 	Server is shutting down \n");
	cout << "SERVER INFO: 	ShutDown = " << ShutDown << endl;
	return NULL;

}

//-----------------------------------------------------------------
//*	Функция регистрации
//*	Сохраняет данные о СУБТД
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
