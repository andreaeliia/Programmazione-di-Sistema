#include "apue.h"
#include <signal.h>
#include <pthread.h>

void* signal_thread(void* arg){
    sigset_t *signal_set = (sigset_t*)arg; 
    int received_signal;

    printf("Thread segnali avviato \n");

    while(1){
        //Aspettare SIGUSR1 SIGUSR2
         if (sigwait(signal_set, &received_signal) != 0) {
            err_sys("sigwait failed");
        }

        switch (received_signal) {
            case SIGUSR1:
                printf("Thread segnali: ricevuto SIGUSR1\n");
                break;
            case SIGUSR2:
                printf("Thread segnali: ricevuto SIGUSR2\n");
                break;
            case SIGTERM:
                printf("Thread segnali: terminazione\n");
                return NULL;
        }
    }
}

void* worker_thread(void* arg){
    int id = *((int*)arg);
    for (int i = 0; i < 10; i++)
    {
        printf("Worker %d: interazione %d\n",id,i);
        sleep(4);
    }
    return NULL;
    
}

int main(void){
    pthread_t signal_tid, worker_tid;
    sigset_t set;
    int worker_id = 1;


    //Preprazione set per i segnali
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    sigaddset(&set, SIGTERM);

    //Blocco dei segnali per tutti i thread
    pthread_sigmask(SIG_BLOCK,&set,NULL);
    
    printf("Processo PID : %d\n",getpid());

    //creazione thread per gestione dei segnali
    pthread_create(&signal_tid,NULL,signal_thread,&set);


    //Creazione thread worker
    pthread_create(&worker_tid,NULL, worker_thread,&worker_id);


    //Aspetto il thread
    pthread_join(worker_tid,NULL);


    //termino thread segnali
    printf("Invio SIGTERM per terminare signal thread\n");
    kill(getpid(),SIGTERM);
    pthread_join(signal_tid,NULL);

    return 0;

}