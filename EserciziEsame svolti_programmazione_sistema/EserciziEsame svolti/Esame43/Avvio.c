#include "apue.h"
#include <fcntl.h>
#include <sys/shm.h>
#include <time.h>
#include <signal.h>


int fd;
int pfd[2]; 
char *sharedMem = "/Users/patryk/Desktop/universita/Magistrale/PrimoAnno/Sistemi/Esempi/apue.3e/Esempi/EserciziEsame/Esame43/sharedMem.txt";
char *lockingMem ="/Users/patryk/Desktop/universita/Magistrale/PrimoAnno/Sistemi/Esempi/apue.3e/Esempi/EserciziEsame/Esame43/lockingMem.txt";
  
void attachMem();
void createMem();
void detachMem();
void lockMem();
void unlockMem();

char* result;
key_t key;
int sharedMemId;
int fdLockFile;
pid_t parentPid;


static void
sig_int(int signo)
{
	if(getpid()==parentPid)
	printf("\n%s\n",result);
	
	exit(0);
}

      
int main(void)
{
	
	
	pid_t pid;
	createMem();
	attachMem();
	
	parentPid = getpid();
	if (signal(SIGINT, sig_int) == SIG_ERR)
		err_sys("signal(SIGALRM) error");
		
		
	if((pid = fork())<0){
		printf("ERRORE FORK 1");
		exit(1);
	} else if(pid == 0){
		if((fdLockFile = open(lockingMem, O_RDWR) < 0)){
			err_sys("open1 error");
		} 
			
		do{	
			lockMem();
			char str[1024];
			
			int myRandom = random()%10;
			time_t myTime = time(NULL);
			sprintf(str,"PID: %d NUMBER: %d TIME: %d",getpid(),myRandom,myTime);
			strcpy(result,str);
			unlockMem();
			sleep(3);
			
			
		} while(1);
		}
	else{
			if((pid = fork())<0){
		printf("ERRORE FORK 1");
		exit(1);
	} else if(pid == 0){
		if((fdLockFile = open(lockingMem, O_RDWR) < 0)){
			err_sys("open1 error");
		} 
			
		do{	
			lockMem();
			char str[1024];
			
			int myRandom = random()%10;
			time_t myTime = time(NULL);
			sprintf(str,"PID: %d NUMBER: %d TIME: %d",getpid(),myRandom,myTime);
			strcpy(result,str);
			unlockMem();
			sleep(3);
			
			
		} while(1);
		}
	else{
			if((pid = fork())<0){
		printf("ERRORE FORK 1");
		exit(1);
	} else if(pid == 0){
		if((fdLockFile = open(lockingMem, O_RDWR) < 0)){
			err_sys("open1 error");
		} 
			
		do{	
			lockMem();
			char str[1024];
			
			int myRandom = random()%10;
			time_t myTime = time(NULL);
			sprintf(str,"PID: %d NUMBER: %d TIME: %d",getpid(),myRandom,myTime);
			strcpy(result,str);
			unlockMem();
			sleep(3);
			
			
		} while(1);
		}
	else{
			WAIT_CHILD();
		
			}
		
		}	
	}
	exit(0);
}



void createMem(){
key = ftok(sharedMem,0);
	if(key == -1){
		printf("Error creation Key");
	}

	sharedMemId = shmget(key,1024,0644|IPC_CREAT);
	}

void attachMem(){
	

	result = shmat(sharedMemId,NULL,0);
	if(result == (char*)(-1)){
		printf("Error attach Memory");
	}
}

void detachMem(){
	shmdt(result);

}

void lockMem(){
	struct flock region_to_lock;
    int res;
	
	
    
    region_to_lock.l_type = F_WRLCK;
    region_to_lock.l_whence = SEEK_SET;
    region_to_lock.l_start = 0;
    region_to_lock.l_len = 10;
    
    res = fcntl(fdLockFile, F_SETLKW, &region_to_lock);
     if (res == -1) {
        printf("Process %d - failed to unlock region\n", getpid());
     }
    
}

void unlockMem(){
	
	struct flock region_to_lock;
    int res;
    
	region_to_lock.l_type = F_UNLCK;
    region_to_lock.l_whence = SEEK_SET;
    region_to_lock.l_start = 0;
    region_to_lock.l_len = 10;
    
    res = fcntl(fdLockFile, F_SETLKW, &region_to_lock);
    if (res == -1) {
        printf("Process %d - failed to unlock region\n", getpid());
    } 
}


