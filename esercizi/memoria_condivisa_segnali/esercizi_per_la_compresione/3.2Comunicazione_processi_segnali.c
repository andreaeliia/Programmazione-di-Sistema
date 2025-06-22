#include "apue.h"
#include <signal.h>
#include <sys/wait.h>


void child_signal_handler(int sig) {
    printf("Figlio ricevuto segnale %d\n", sig);
}


int main(void){
    pid_t pid;

    pid = fork();

    if(pid == 0 ){
        signal(SIGUSR1,child_signal_handler);
        printf("Figlio (PID: %d) in attesa...\n", getpid());
        
        // Aspetta segnali
        while (1) {
            pause();  // Sospende fino al prossimo segnale
        }
    }else if (pid>0)
    {
        printf("Padre (PID : %d) inviera segnali...\n",getpid());

        sleep(2); //aspetto che il figlio sia pronto

        for (int i = 0; i < 3; i++) {
            printf("Padre invia SIGUSR1 a figlio\n");
            
            // TODO: Invia SIGUSR1 al figlio
            kill(pid, SIGUSR1);
            
            sleep(2);
        }

        // Termina figlio
        kill(pid, SIGTERM);
        wait(NULL);
        printf("Figlio terminato\n");
    }else{
        err_sys("fork failed");
    }
    return 0;
    
}