#include "apue.h"
#include <signal.h>


void signal_handler(int sig){
    if(sig ==  SIGUSR1){
        printf("Ricevuto SIGUSR1! PID : %d\n",getpid());


    }
}

int main (void){
    //IMPOSTIAMO GESTORE PER SIGUSR1
    signal(SIGUSR1,signal_handler);

    printf("Processo PID: %d\n", getpid());
    printf("Invia segnale con: kill -USR1 %d\n", getpid());

    while(1){
        printf("In attesa di segnali...\n");
        sleep(2);
    }
    return 0;
}