#include <sys/shm.h>
#include <sys/file.h>
#include <unistd.h>
#include "pppd.h"
#include "chap-sync.h"

#define LOCK_FILE_1 "/tmp/chaplockfile1"
#define LOCK_FILE_2 "/tmp/chaplockfile2"
#define KEY_FILE "/tmp/chapkeyfile"

int chap_sync_count = 1;

void chap_sync(){
	int sync_count = chap_sync_count;
	int fd_lock_1, fd_lock_2, id_shm;
	key_t key;
	int * shm;
	
	if(sync_count <= 1){
		return;
	}
	
	creat(KEY_FILE, 0755);
	key = ftok(KEY_FILE, 4);
	if (key < 0) {
		error("Sync fail: Can't get key.");
		return;
	}
	
	id_shm = shmget(key, sizeof(int), 0666);
	if(id_shm == -1){
		id_shm = shmget(key, sizeof(int), 0666 | IPC_CREAT);
		if(id_shm == -1){
			error("Sync fail: Can't get shared memory.");
			return;
		}
	}
	
	shm = (int*)shmat(id_shm, 0, 0);
	if(shm == NULL){
		error("Sync fail: Can't attach shared memory.");
		return;
	}

	fd_lock_1 = open(LOCK_FILE_1, O_RDONLY | O_CREAT);
	fd_lock_2 = open(LOCK_FILE_2, O_RDONLY | O_CREAT);
	
	if(flock(fd_lock_1, LOCK_EX | LOCK_NB) == -1){/*从*/
		int i;
		notice("Sync info: Slave process.");
		
		for(i = 0; i < 100; i++){
			if(flock(fd_lock_2, LOCK_SH | LOCK_NB) != -1){
				flock(fd_lock_2, LOCK_UN);
				usleep(10);
			} else {
				break;
			}
		}
		notice("Sync info: Slave process - wait for Main.");
		
		if(flock(fd_lock_2, LOCK_SH) == -1){
			goto err;
		}
		
		if(flock(fd_lock_2, LOCK_UN) == -1){
			goto err;
		}
		
		notice("Sync info: Slave process - try to get counter lock.");
		
		if(flock(fd_lock_2, LOCK_EX) == -1){
			goto err;
		}
		
		notice("Sync info: Slave process - get counter lock. Counter value %d", (*shm));
		
		(*shm)--;
		
		if(flock(fd_lock_2, LOCK_UN) == -1){
			goto err;
		}
		
		if(flock(fd_lock_1, LOCK_SH) == -1){
			goto err;
		}
		
		if(flock(fd_lock_1, LOCK_UN) == -1){
			goto err;
		}
	} else {/*主*/
		notice("Sync info: Main process.");
		
		if(flock(fd_lock_2, LOCK_EX) == -1){
			/*Oops!*/
			goto err;
		}
		notice("Sync info: Main process - get counter lock.");
		
		/*初始化计数器*/
		*shm = sync_count - 1;
		
		if(flock(fd_lock_2, LOCK_UN) == -1){
			goto err;
		}
		notice("Sync info: Main process - release counter lock.");
		
		while(*shm > 0){
			usleep(1);
		}
		
		if(flock(fd_lock_1, LOCK_UN) == -1){
			goto err;
		}
	}
	
	err:
	shmdt(shm);
}
