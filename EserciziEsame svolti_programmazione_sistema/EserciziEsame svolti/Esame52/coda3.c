#include <stdio.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>


int fine = 0;

int loop = 1;


static int semaphore_p(void);
static int semaphore_v(void);

static int sem_id;

void *thread_function(void *arg);


int main(){
	
	/*Avvio il thread*/
	int res;
    pthread_t a_thread;
    res = pthread_create(&a_thread, NULL, thread_function,NULL);
    if (res != 0) {
        perror("Thread creation failed");
        exit(EXIT_FAILURE);
    }
    
    /*Avvio il semaforo*/
    
    
    sem_id = semget((key_t)1234, 1, 0666 | IPC_CREAT);
    union semun sem_union;
    sem_union.val = 1;
	if(semctl(sem_id, 0, SETVAL, sem_union)== -1){
	fprintf(stderr, "semaphore init failed\n");
	return 1;
	}
	
    sleep(1);
   
    
    /*Loop per bloccare semaforo e passare*/
    while(loop == 1){
    	if(fine > 0){
    		
    		semaphore_p();
    		printf("\nMacchina processo 3 entrata, ci sono %d macchine in coda \n", (fine ));
    		
    		sleep(3);
    		fine--;
    		
    		printf("Macchina processo 3 uscita ci sono %d macchine in coda \n", (fine ));
    		
    		semaphore_v();
    		
    		
    	}
    }
    
	   
    return 0;
}

void *thread_function(void *arg) {
    
    
    while(loop == 1){
    		srand(time(NULL)); 
    		int r = (rand()%6) + 2;
    		sleep(r);
    		fine++;
    
    }
}



static int semaphore_p(void)
{
    struct sembuf sem_b;
    
    sem_b.sem_num = 0;
    sem_b.sem_op = -1; /* P() */
    sem_b.sem_flg = SEM_UNDO;
    if (semop(sem_id, &sem_b, 1) == -1) {
        fprintf(stderr, "semaphore_p failed\n");
        return(0);
    }
    return(1);
}

/* semaphore_v is similar except for setting the sem_op part of the sembuf structure to 1,
 so that the semaphore becomes available. */

static int semaphore_v(void)
{
    struct sembuf sem_b;
    
    sem_b.sem_num = 0;
    sem_b.sem_op = 1; /* V() */
    sem_b.sem_flg = SEM_UNDO;
    if (semop(sem_id, &sem_b, 1) == -1) {
        fprintf(stderr, "semaphore_v failed\n");
        return(0);
    }
    return(1);
}