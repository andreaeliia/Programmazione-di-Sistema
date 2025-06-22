#include "apue.h"
#include <sys/wait.h>



//programma che crea un processo figlio
int main (void){
    pid_t pid;

    printf("Processo padre PID : %d\n",getpid());

    //creazione processo figlio
    pid = fork();

    if(pid == 0){
        //codice processo figlio
        printf("Sono il figlio,PID %d\n",getpid());
        printf("Mio padre ha il PID : %d\n",getppid());
        sleep(2);
        exit(0);
    }else if (pid > 0 ){
        //codice del processo padre
        printf("Sono il padre, ho creato figlio con PID %d\n",pid);
        

        //aspettare che il figlio finisca
        wait(NULL);
        printf("Figlio terminato \n");
    } else {
        err_sys("fork failed");
    }
  
    
    return 0;

}