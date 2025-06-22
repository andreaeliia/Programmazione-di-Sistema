#include "apue.h"
#include <pthread.h>





//Variabili globali
sigset_t signal_set;



void* signal_thread(void* arg){
    int received_signal;

    printf("Thread per i segnali : in attesa di segnali...\n");
    

    //Loop per asepttare i vari segnali
    while(1){
        if(sigwait(&signal_set,&received_signal)!=0){
            err_sys("sigwait error");
        }

        printf("Segnale ricevuto %d\n",received_signal);
        
        if(received_signal == SIGTERM){
            printf("Segnale di terminazione \n");
            break;
        }
    }
    return NULL;
}









int main(void){

    pthread_t signal_tid;

    //Preparazione dei segnali
    sigemptyset(&signal_set);
    sigaddset(&signal_set, SIGUSR1);
    sigaddset(&signal_set, SIGUSR2);  
    sigaddset(&signal_set, SIGTERM);


    //blocchiamo i segnali per tutti i thread
    pthread_sigmask(SIG_BLOCK,&signal_set,NULL);



    printf("Processo PID : %d\n",getpid());



    pthread_create(&signal_tid,NULL,signal_thread,NULL);

      for (int i = 0; i < 10; i++) {
        printf("Main thread: lavoro %d\n", i);
        sleep(2);
    }
    
    // Termina signal thread
    kill(getpid(), SIGTERM);
    pthread_join(signal_tid, NULL);

    return 0;


}