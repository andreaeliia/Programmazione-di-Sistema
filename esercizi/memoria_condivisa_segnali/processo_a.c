#include "apue.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

#define SHM_NAME "/char_memory"
#define BUFFER_SIZE 20 //Variabile globale


//STUTTURA PER LA MEMORIA CONDIVISa


typedef struct
{
    char buffer[BUFFER_SIZE];
    int write_index;
    pid_t process_a_pid;
    pid_t process_b_pid;
    int ready_flag;  // AGGIUNTO: flag per sincronizzazione
}shared_data_t;

//VARIABILI GLOBALi
shared_data_t *shared_data;
sigset_t signal_set;
volatile int can_write =1;


//thread per la gestione dei segnali
void* signal_thread(void* arg) {
    int received_signal;
    
    printf("Processo A - Signal thread avviato\n");
    
    while (1) {
        sigwait(&signal_set, &received_signal);
        
        switch (received_signal) {
            case SIGUSR2:
                printf("Processo A - Ricevuto USR2: posso scrivere\n");
                can_write = 1;
                break;
            case SIGTERM:
                printf("Processo A - Signal thread termina\n");
                return NULL;
        }
    }
}
//thread per scrivere i caratteri
void* writer_thread(void* arg) {
    srand(time(NULL));

        // AGGIUNTO: Aspetta che B sia pronto
    printf("Processo A - Aspetto che B sia pronto...\n");
    while (shared_data->process_b_pid == 0) {
        usleep(100000); // 100ms
    }
    
    for (int i = 0; i < 5; i++) {
        // Aspetta permesso
        while (!can_write) {
            usleep(10000);
        }
        
        if (shared_data->write_index >= BUFFER_SIZE) {
            printf("Buffer pieno\n");
            break;
        }
        
        // Genera carattere casuale
        char ch = 'A' + (rand() % 26);
        
        // Scrivi in memoria condivisa
        shared_data->buffer[shared_data->write_index] = ch;
        printf("Processo A: scritto '%c' in pos %d\n", 
               ch, shared_data->write_index);
        shared_data->write_index++;
        
        can_write = 0;  // Resetta permesso
        
        // Invia segnale a B
        if (shared_data->process_b_pid > 0) {
            kill(shared_data->process_b_pid, SIGUSR1);
        }
        
        // Attesa casuale
        sleep(rand() % 3 + 1);
    }
    
    return NULL;
}

int main(void) {

    pthread_t  signal_tid,writer_tid; //variabili per i thread
    int shm_fd; //variabile per la memoria condivisa
    
    printf("Processo A avviato (PID: %d)\n", getpid());

     // Inizializza memoria condivisa
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(shared_data_t));
    shared_data = mmap(NULL, sizeof(shared_data_t), 
                      PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);


    // ===== INZIALIZZAZIONE DELLA STRUCT =====//
    

    shared_data ->write_index =0;
    shared_data -> process_a_pid = getpid();
    shared_data -> process_b_pid =0;
    
    close(shm_fd);


    //===SEGNALI===/
    sigemptyset(&signal_set);
    sigaddset(&signal_set, SIGUSR1);
    sigaddset(&signal_set, SIGUSR2);  
    sigaddset(&signal_set, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &signal_set, NULL);


    //== THREAD ==/
    pthread_create(&signal_tid, NULL, signal_thread, NULL);
    pthread_create(&writer_tid, NULL, writer_thread, NULL);
    pthread_join(writer_tid, NULL);// Aspetta writer
    pthread_cancel(signal_tid);// Termina


    //===BUFFER FINALE ====/
     printf("Processo A - Buffer: ");
    for (int i = 0; i < shared_data->write_index; i++) {
        printf("%c", shared_data->buffer[i]);
    }
    printf("\n");
    
    // Pulizia
    munmap(shared_data, sizeof(shared_data_t));
    shm_unlink(SHM_NAME);
    
    printf("Processo A terminato\n");
    return 0;


    return 0;
}