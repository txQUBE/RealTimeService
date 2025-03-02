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
#include <fcntl.h>
#include <sys/mman.h>

#define NOTIF_CHAN 	"notification_channel"
#define TICKPAS_SIG SIGRTMIN // ������ � ��������� ����
#define RTS_SHM_TIME_NAME "rts_shm_time"
#define NOTIF_MARK "- - Notif: " // ����� ��� ������ � �������
// ��������� ������, ���������� � ����������� ������ RTS_SHM_TIME
// ��������� ��� ������ ��������� �����
typedef struct {
	int count; // 	����� �������� ����
	long tick_nsec; // 	������������ ������ ���� � ������������
	int tick_sec;// 	������������ ������ ���� � ��������
} local_time;

// ��������� ����������� ������ ���������� �������
typedef struct {
	pthread_rwlockattr_t attr; //���������� ������ ���������� ������/������
	pthread_rwlock_t rwlock; //���������� ������/������
	local_time count; // 	����� �������� ����
} shm_time;

shm_time* shmTimePtr; // ��������� ������������ ������

extern Clock timer; // ����� �������
extern pthread_barrier_t notif_barrier;
extern bool tick_changed; // ����-������ ��������� ���� �������

//��� ������ �������� �� �������
union TimerSig {
	_pulse M_pulse;
} tsig;

//��� �������� ������� ������� ������� ����
time_t now;

//��������� �������
shm_time* createNamedMemory(const char* name);
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
	// �������� ����������� ������
	shmTimePtr = createNamedMemory(RTS_SHM_TIME_NAME);

	// ��������� ��������
	pthread_rwlock_wrlock(&shmTimePtr->rwlock);
	shmTimePtr->count.tick_nsec = timer.tick_nsec;
	shmTimePtr->count.tick_sec = timer.tick_sec;
	pthread_rwlock_unlock(&shmTimePtr->rwlock);

	cout << NOTIF_MARK << "������ ������ �����������" << endl << endl;
	name_attach_t *attach;
	TimerSig tsig;

	//	�������� ������������ ������
	if ((attach = name_attach(NULL, NOTIF_CHAN, 0)) == NULL) {
		cout << NOTIF_MARK << "������ �������� ������ �����������" << endl;
		exit(EXIT_FAILURE);
	}

	pthread_barrier_wait(&notif_barrier); // �������� ����������� ������� � ������

	//	���� ��������� �������
	while (!shutDown) {
		cout << NOTIF_MARK << "�������� ������� �������" << endl;
		int rcvid = MsgReceive(attach->chid, &tsig, sizeof(tsig), NULL);
		if (rcvid == 0) { // ������� ������� �� �������
			gotAPulse();
			sendNotifTickPassed();
		}
	}

	//������������ ��������
	pthread_rwlockattr_destroy(&shmTimePtr->attr);
	pthread_rwlock_destroy(&shmTimePtr->rwlock);
	cout << NOTIF_MARK << "EXIT_SUCCESS" << endl;
	return EXIT_SUCCESS;
}

/*
 * ������� ��������� ������� �������
 */
void gotAPulse(void) {
	timer.count++;
	time(&now);//�������� ������� ������ �������
	//������ ������ �������
	cout << NOTIF_MARK << " GOT A TIMER PULSE, count: " << timer.count << endl;
	cout << NOTIF_MARK << "TIME: " << ctime(&now);

	//������ ���������� ��� ������
	pthread_rwlock_wrlock(&shmTimePtr->rwlock);
	shmTimePtr->count.count = timer.count;
	shmTimePtr->count.tick_nsec = timer.tick_nsec;
	shmTimePtr->count.tick_sec = timer.tick_sec;
	pthread_rwlock_unlock(&shmTimePtr->rwlock);
}

/*
 * ������� �������� ������� � ��������� ����
 */
void sendNotifTickPassed() {
	// ������ ������ � ������������������� �����
	pthread_mutex_lock(&tdb_map.mtx);

	if (tdb_map.buf.empty()) {
		cout << NOTIF_MARK << "no registered TDB_MS" << endl;
	} else {
		cout << NOTIF_MARK << "send SIGUSR1" << endl;
		// �������� ������ � ������������������� �����
		for (map<string, tdb_ms_t>::iterator it = tdb_map.buf.begin(); it
				!= tdb_map.buf.end(); ++it) {
			// ������� ������ � ��������� ���� � �����
			if (SignalKill(0, it->second.pid, it->second.tid, TICKPAS_SIG,
					SI_USER, 0) == -1) {
				if (errno == ESRCH) {
					// ������� �� ������ ���������� �������
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
 * ������� ����������� � ����������� ������
 */
shm_time* createNamedMemory(const char* name) {
	shm_time *namedMemoryPtr;
	int fd; //���������� ����������� ������

	// �������� ����������� ������
	if ((fd = shm_open(name, O_RDWR | O_CREAT, 0777)) == -1) {
		cerr << NOTIF_MARK << "error shm_open, errno: " << errno << endl;
		exit(0);
	}
	// ��������� ������� ����������� ������
	if (ftruncate(fd, 0) == -1 || ftruncate(fd, sizeof(shm_time)) == -1) {
		cerr << NOTIF_MARK << "error ftruncate, errno: " << errno << endl;
		exit(0);
	}
	//����������� ����������� ����������� ������ � �������� ������������ ��������
	if ((namedMemoryPtr = (shm_time*) mmap(NULL, sizeof(shm_time), PROT_READ
			| PROT_WRITE, MAP_SHARED, fd, 0)) == MAP_FAILED) {
		cerr << NOTIF_MARK << "error mmap, errno: " << errno << endl;
		exit(0);
	}
	cout << NOTIF_MARK << "shm_open - ����������� ������ ���������" << endl;


	//������������� ���������� ������/������
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

