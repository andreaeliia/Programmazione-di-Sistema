#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/times.h>

#define NUMBERS 1000

int numbers[NUMBERS];

void dummy(int numero){
	volatile unsigned long sum = 0;
	int i;
	for (i = 0; i < numero; i++) {
        sum += i % 3;
    }

}


void unnamed_pipe(){

	pid_t pid;
	int file_pipes[2];
	int data_processed_parent, data_processed_child;
	int buffer[NUMBERS];
	int semid;
	int i, j;
	struct sembuf sem_b;
	
	struct timeval start_time, end_time;
    struct tms tms_buffer;
    long ticks_per_second = sysconf(_SC_CLK_TCK);
	
	/**Creazione chiave */
	key_t key_sem = ftok("sem_transfer", 1234);
    semid = semget(key_sem, 1, IPC_CREAT);
    if (semid < 0) {
        perror("Errore nell'apertura del semaforo");
        exit(EXIT_FAILURE);
    }

	 /** Inizializzazione del semaforo*/
    semctl(semid, 0, SETVAL, 1);
	

	if(pipe(file_pipes) == 0){
		/*Creazione del processo figlio*/
		pid = fork();
		if (pid < 0) {
		    fprintf(stderr, "Errore nella fork");
		    exit(EXIT_FAILURE);
		}
	}

	if(pid == 0){
		/*Processo figlio B*/
		close(file_pipes[1]); /*Chiusura della scrittura*/
			printf("Il figlio sta leggendo");
		for (j = 0; j < NUMBERS; j++){
			/*Il figlio aspetta se il padre sta scrivendo*/
			sem_b.sem_num = 0;
        	sem_b.sem_op = -1;
        	sem_b.sem_flg = 0;
        	semop(semid, &sem_b, 1);
		
		
			data_processed_child = read(file_pipes[0], &buffer[j], sizeof(buffer[j]));
			dummy(buffer[j]);
		
			/*Il padre può scrivere*/
			sem_b.sem_op = 1;
        	semop(semid, &sem_b, 1);
        	
        	sleep(1);
		}
		close(file_pipes[0]);
		printf("Letti %d bytes\n", data_processed_child);

	} else {
		printf("Il padre sta scrivendo...\n");
		/*Processo padre A*/
		close(file_pipes[0]); /*Chiusura della lettura*/
		/* Inizio misurazione del tempo di orologio */
        if (gettimeofday(&start_time, NULL) != 0) {
            printf("Errore durante il recupero del tempo di inizio\n");
            exit(EXIT_FAILURE);
        }
		
		for (i = 0; i < NUMBERS; i++){
			
			/*Il padre aspetta se il figlio sta leggendo*/
			sem_b.sem_num = 0;
        	sem_b.sem_op = -1;
        	sem_b.sem_flg = 0;
        	semop(semid, &sem_b, 1);
		
			data_processed_parent = write(file_pipes[1], &numbers[i], sizeof(numbers[i]));
			
			/*Il figlio può leggere*/
			sem_b.sem_op = 1;
        	semop(semid, &sem_b, 1);
		}
		
		close(file_pipes[1]);
		printf("Scritti %d bytes\n", data_processed_parent);
		wait(NULL); /*Aspetta che il figlio termini*/
		/* Fine misurazione tempo di orologio */
        if (gettimeofday(&end_time, NULL) != 0) {
            printf("Errore durante il recupero del tempo di fine\n");
            exit(EXIT_FAILURE);
        }
        
        /*Calcolo tempi di utente e di sistema*/
		 times(&tms_buffer);
	}

	 /*Calcolo dei tempi in microsecondi*/
    double clock_time = ((end_time.tv_sec - start_time.tv_sec) * 1e6) + (end_time.tv_usec - start_time.tv_usec);
    double user_time = ((double)tms_buffer.tms_utime) / ticks_per_second * 1e6;
    double sys_time = ((double)tms_buffer.tms_stime) / ticks_per_second * 1e6;
    
    fprintf(stderr, "Clock: %.6f µs, User: %.6f µs, System: %.6f µs\n", clock_time, user_time, sys_time);

}





int main (int argc, char *argv[]){
	
	
	/*generazione dei numeri casuali*/
	srand(time(NULL));
	int i;
	for (i = 0; i < NUMBERS; i++){
		numbers[i] = rand();
	}
	
	printf("Inizio della comunicazione tra i processi...\n");
	
	unnamed_pipe();
	
	return (EXIT_SUCCESS);

}
