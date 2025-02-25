/*
 * server.cc
 *
 *  Created on: 18.06.2024
 *      Author: qnx
 */
#include "server.h"
#include "utils.h"

#define REG_CHAN 	"RTS_registration_channel"

const int REG_TYPE = 201; //				тип сообщения для регистрации
const int SEND_TIME_TYPE = 2; //		тип сообщения для отправки времени

typedef struct _pulse msg_header_t; // абстрактный тип для заголовка сообщения как у импульса
typedef struct _TIME_hadnle {
	msg_header_t hdr;
	string name; //		имя СУБТД
	int pid; //			id процесса
	int tid; //			id нити
} registration_handle_t;

//Буфер для запоминания СУБТД
tdb_ms_t registered_tdb;

void* server(void*) {
	cout << "- - - - Server: 	запуск сервера регистрации" << endl;
	name_attach_t* attach;
	registration_handle_t msg;
	int rcvid;

	//	Создание именованного канала
	if ((attach = name_attach(NULL, REG_CHAN, 0)) == NULL) {
		cout << "- - - - Server: 	ошибка создания именованного канала" << endl;
		exit(EXIT_FAILURE);
	}

	//	Цикл ожидания поступления сообщений в именованный канал
	while (!shutDown) {
		cout << "- - - - Server: 	ожидание запроса подключения" << endl;
		rcvid = MsgReceive(attach->chid, &msg, sizeof(msg), NULL);
		cout << "- - - - Server ПОЛУЧЕН RCVID = " << rcvid << endl;

		if (rcvid == -1) { /* Ошибка, завершение */
			cout << "- - - - Server: 	поулчен rcvid = " << rcvid << endl;
		}

		if (rcvid == 0) {/* Получен импульс */
			switch (msg.hdr.code) {
			case _PULSE_CODE_DISCONNECT:
				/* Клиент разорвал все ранее созданные связи (вызвал name_close() для каждого вызова
				 name_open() с именем канала) или терминировался */
				ConnectDetach(msg.hdr.scoid);
				/* Уничтожить соединение ядра с каналом сервера*/
				name_detach(attach, 0);
				/* Удалить имя процесса */
				printf("Server receive _PULSE_CODE_DISCONNECT and terminated");
				return EXIT_SUCCESS;
			case _PULSE_CODE_UNBLOCK:
				/* Клиент хочет деблокироваться (получен сигнал или истёк таймаут). Серверу необходимо
				 принять решение о посылке ответа */
				break;
			default:
				/* Пришёл импульс от какого-то процесса или от ядра
				 (PULSE_CODE_COIDDEATH или _PULSE_CODE_THREADDEATH) */
				break;
			}
			continue;// Завершить текущую итерацию цикла
		}
		/* Полученное сообщение не импульс */
		if (msg.hdr.type == _IO_CONNECT) {
			cout << "- - - - Server: recieve name_open()\n";
			MsgReply(rcvid, EOK, NULL, 0);
			continue; // Завершить текущую итерацию цикла
		}
	}
	printf("- - - - Server: Server is shutting down \n");
	cout << "- - - - Server: ShutDown = " << shutDown << endl;
}

//-----------------------------------------------------------------
//*	Функция регистрации
//*	Сохраняет данные о СУБТД
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
