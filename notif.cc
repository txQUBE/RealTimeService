/*
 * notif.cc
 *
 *  Created on: 18.06.2024
 *      Author: Грац Д.Д.
 *
 *  Нить канала уведомлений об истечении тика таймера
 */

#include "notif.h"
#include "utils.h"
#include <fcntl.h>
#include <sys/mman.h>

#define NOTIF_CHAN 	"notification_channel"
#define TICKPAS_SIG SIGRTMIN // сигнал о прошедшем тике
#define RTS_SHM_TIME_NAME "rts_shm_time"
#define NOTIF_MARK "- - Notif: " // метка для вывода в консоль
// Структура данных, хранящаяся в именованной памяти RTS_SHM_TIME
// Структура для буфера локальных часов
typedef struct {
	int count; // 	Номер текущего тика
	long tick_nsec; // 	Длительность одного тика в наносекундах
	int tick_sec;// 	Длительность одного тика в секундах
} local_time;

// Структура именованной памяти параметров таймера
typedef struct {
	pthread_rwlockattr_t attr; //атрибутная запись блокировки чтения/записи
	pthread_rwlock_t rwlock; //блокировка чтения/записи
	local_time count; // 	Номер текущего тика
} shm_time;

shm_time* shmTimePtr; // указатель именованнной памяти

extern Clock timer; // Буфер таймера
extern pthread_barrier_t notif_barrier;
extern bool tick_changed; // флаг-статус изменения тика таймера

//Для приема испульса от таймера
union TimerSig {
	_pulse M_pulse;
} tsig;

//Для хранения момента времени каждого тика
time_t now;

//Прототипы функций
shm_time* createNamedMemory(const char* name);
void sendNotifTickPassed(); // функция отправки сигнала о прошедшем тике
void sendTickParam(int coid); // функция отправки актуального значения тика таймера
void sendTickUpdate();// функция отправки обновлений параметров таймера
void gotAPulse(void);//		Функиця обработки сигнала таймера

/*
 * Нить уведомлений
 * Принимает импульсы от таймера
 * Отправляет сигнал по проществии тика в зарегестрирование СУБД
 */
void* notification(void*) {
	// создание именованной памяти
	shmTimePtr = createNamedMemory(RTS_SHM_TIME_NAME);

	// начальные значения
	pthread_rwlock_wrlock(&shmTimePtr->rwlock);
	shmTimePtr->count.tick_nsec = timer.tick_nsec;
	shmTimePtr->count.tick_sec = timer.tick_sec;
	pthread_rwlock_unlock(&shmTimePtr->rwlock);

	cout << NOTIF_MARK << "запуск канала уведомлений" << endl << endl;
	name_attach_t *attach;
	TimerSig tsig;

	//	Создание именованного канала
	if ((attach = name_attach(NULL, NOTIF_CHAN, 0)) == NULL) {
		cout << NOTIF_MARK << "ошибка создания канала уведомлений" << endl;
		exit(EXIT_FAILURE);
	}

	pthread_barrier_wait(&notif_barrier); // ожидание подключения таймера к каналу

	//	Приём сообщений таймера
	while (!shutDown) {
		cout << NOTIF_MARK << "ожидание сигнала таймера" << endl;
		int rcvid = MsgReceive(attach->chid, &tsig, sizeof(tsig), NULL);
		if (rcvid == 0) { // получен импульс от таймера
			gotAPulse();
			sendNotifTickPassed();
		}
	}

	//Освобождение ресурсов
	pthread_rwlockattr_destroy(&shmTimePtr->attr);
	pthread_rwlock_destroy(&shmTimePtr->rwlock);
	cout << NOTIF_MARK << "EXIT_SUCCESS" << endl;
	return EXIT_SUCCESS;
}

/*
 * Функиця обработки сигнала таймера
 */
void gotAPulse(void) {
	timer.count++;
	time(&now);//Получаем текущий момент времени
	//Печать строки времени
	cout << NOTIF_MARK << " GOT A TIMER PULSE, count: " << timer.count << endl;
	cout << NOTIF_MARK << "TIME: " << ctime(&now);

	//захват блокировки для записи
	pthread_rwlock_wrlock(&shmTimePtr->rwlock);
	shmTimePtr->count.count = timer.count;
	shmTimePtr->count.tick_nsec = timer.tick_nsec;
	shmTimePtr->count.tick_sec = timer.tick_sec;
	pthread_rwlock_unlock(&shmTimePtr->rwlock);
}

/*
 * функция отправки сигнала о прошедшем тике
 */
void sendNotifTickPassed() {
	// захват буфера с зарегистрированными СУБТД
	pthread_mutex_lock(&tdb_map.mtx);

	if (tdb_map.buf.empty()) {
		cout << NOTIF_MARK << "no registered TDB_MS" << endl;
	} else {
		cout << NOTIF_MARK << "send SIGUSR1" << endl;
		// итерация буфера с зарегистрированными СУБТД
		for (map<string, tdb_ms_t>::iterator it = tdb_map.buf.begin(); it
				!= tdb_map.buf.end(); ++it) {
			// Послать сигнал о прошедшем тике В СУБТД
			if (SignalKill(0, it->second.pid, it->second.tid, TICKPAS_SIG,
					SI_USER, 0) == -1) {
				if (errno == ESRCH) {
					// Удалить из буфера неактивный процесс
					cout << NOTIF_MARK << "erase " << it->first << endl;
					tdb_map.buf.erase(it->first);
				} else
					cerr << NOTIF_MARK << "error SignalKill errno: "
							<< strerror(errno) << endl;
			}
		}

	}
	pthread_mutex_unlock(&tdb_map.mtx);
	cout << endl;
}

/*
 * Функция подключения к именованной памяти
 */
shm_time* createNamedMemory(const char* name) {
	shm_time *namedMemoryPtr;
	int fd; //дескриптор именованной памяти

	// Создание именованной памяти
	if ((fd = shm_open(name, O_RDWR | O_CREAT, 0777)) == -1) {
		cerr << NOTIF_MARK << "error shm_open, errno: " << errno << endl;
		exit(0);
	}
	// Установка размера именованной памяти
	if (ftruncate(fd, 0) == -1 || ftruncate(fd, sizeof(shm_time)) == -1) {
		cerr << NOTIF_MARK << "error ftruncate, errno: " << errno << endl;
		exit(0);
	}
	//Отображение разделяемой именованной памяти в адресное пространство процесса
	if ((namedMemoryPtr = (shm_time*) mmap(NULL, sizeof(shm_time), PROT_READ
			| PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) {
		cerr << NOTIF_MARK << "error mmap, errno: " << errno << endl;
		exit(0);
	}
	cout << NOTIF_MARK << "shm_open - именованная память построена" << endl;


	//Инициализация блокировки чтения/записи
	if (pthread_rwlockattr_init(&namedMemoryPtr->attr) != EOK) {
		cerr << NOTIF_MARK << "error rwlockattr_init, errno: " << errno << endl;
		exit(0);
	}

	if (pthread_rwlockattr_setpshared(&namedMemoryPtr->attr,
			PTHREAD_PROCESS_SHARED) != EOK) {
		cerr << NOTIF_MARK << "error rwlockattr_setpshared, errno: " << errno
				<< endl;
		exit(0);
	}

	if (pthread_rwlock_init(&namedMemoryPtr->rwlock, &shmTimePtr->attr) != EOK) {
		cerr << NOTIF_MARK << "error rwlock_init, errno: " << errno << endl;
		exit(0);
	}

	return shmTimePtr;
}

