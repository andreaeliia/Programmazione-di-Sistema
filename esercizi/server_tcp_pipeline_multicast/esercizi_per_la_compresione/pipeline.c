#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>



int main(){
    int pipefd[2];// pipefd[0] = lettura, pipefd[1] = scrittura
    pid_t pid;
    char message[] = "Ciao dal padre";
    char buffer[100];


    //creazione della pipe prima del fork
    if(pipe(pipefd)== -1){
        perror("pipe");
        exit(0);
    }

     printf("Pipe creata: lettura=%d, scrittura=%d\n", pipefd[0], pipefd[1]);

     pid = fork();

      if (pid == -1) {
        perror("fork");
        exit(1);
    }else if (pid==0){
        // processo figlio che legge della pipe
        printf("Figlio: chiudo lato scrittura\n");
        sleep(3);
        close(pipefd[1]);


        printf("Figlio: aspetta il messaggio...\n");
        sleep(3);
        int bytes_read = read(pipefd[0],buffer,sizeof(buffer)-1);

        printf("Figlio: ricevuto '%s' (%d bytes)\n", buffer, bytes_read);
        sleep(3);
    }else{
        printf("Padre: chiudo la lettura...\n");
        sleep(3);
        close(pipefd[0]);


        printf("Padre: invio messaggio...\n");
        sleep(3);
        write(pipefd[1],message,strlen(message));

        printf("Padre: chiudo pipe e aspetto il figlio...\n");
        sleep(3);
        close(pipefd[1]);

        wait(NULL);
        sleep(3);
        printf("Padre : terminato...\n");
    }
    return 0;
}

/*
COMPILA: gcc -o esercizio2 esercizio2.c
ESEGUI: ./esercizio2

OUTPUT ATTESO:
Pipe creata: lettura=3, scrittura=4
Padre: chiudo lato lettura
Figlio: chiudo lato scrittura
Padre: invio messaggio...
Figlio: aspetto messaggio...
Figlio: ricevuto 'Ciao dal padre!' (15 bytes)
Padre: chiudo pipe e aspetto figlio
Padre: finito!

CONCETTI CHIAVE:
1. pipe() PRIMA di fork()
2. Ogni processo chiude il lato che non usa
3. Chiudere pipe sblocca read() dall'altra parte
*/