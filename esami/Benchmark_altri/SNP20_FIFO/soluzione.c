/*Quattro thread di un processo devono realizzare una catena di montaggio che svolga le
seguenti funzioni:
la thread 1 deve generare dei numeri interi positivi casuali e
posizionarli in una FIFO di dimensione massima fornita come parametro dalla linea di
comando;
la thread 2 deve scoprire, leggendoli dall'uscita della FIFO, se tali interi sono
primi; 
la thread 3 deve spostarli in un'altra FIFO se sono primi e rimuoverli se non lo
sono; 
la thread 4 deve dare allo standard output i fattori di ciascun numero non primo. Al
momento in cui il programma viene interrotto con ^C, deve stampare la lista dei numeri
primi trovati fino a quel momento. Le quattro thread devono operare  e sincronizzandosi in
modo da non creare interferenze nelle aree di memoria alle quali accedono e realizzando il
massimo livello di parallelismo possibile.*/




#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>  

#define FIFO_NAME "/tmp/my_fifo"
#define BUFFER_SIZE 256

/*===============VARIABILI GLOBALI==========*/
pthread_t thread1;
pthread_t thread2;
pthread_t thread3;
pthread_t thread4;
volatile int running =1;

/*=================UTILS===========================*/
int random_value(){    
    int random_number;
    random_number = rand() % 10000;
    return random_number;
}
/*================FIFO=============*/
int create_fifo() {
    int res;
    
    if (access(FIFO_NAME, F_OK) != -1) {
        printf("FIFO %s esiste gia'\n", FIFO_NAME);
        return 1;
    }
    
    res = mkfifo(FIFO_NAME, 0777);
    if (res != 0) {
        fprintf(stderr, "Errore: impossibile creare FIFO %s\n", FIFO_NAME);
        return -1;
    }
    
    printf("FIFO creata con successo: %s\n", FIFO_NAME);
    printf("Permessi: (0777)\n");
    return 0;
}

int read_fifo(int pipe_fd,char* buffer){
    int res;
     /* Verifica esistenza FIFO */
    if (access(FIFO_NAME, F_OK) == -1) {
        fprintf(stderr, "Errore: FIFO %s non esiste\n", FIFO_NAME);
        fprintf(stderr, "Esegui prima il writer per crearla\n");
        exit(EXIT_FAILURE);
    }

    printf("Reader %d: apertura FIFO in modalita' lettura...\n", getpid());
    pipe_fd = open(FIFO_NAME, open_mode);
    
    if (pipe_fd == -1) {
        fprintf(stderr, "Errore: impossibile aprire FIFO per lettura\n");
        exit(EXIT_FAILURE);
    }
    
    printf("Reader %d: FIFO aperta con successo (fd=%d)\n", getpid(), pipe_fd);
    printf("Reader: in attesa di messaggi...\n\n");

    /* Lettura continua dalla FIFO */
    while (1) {
        res = read(pipe_fd, buffer, BUFFER_SIZE);
        
        if (res == -1) {
            fprintf(stderr, "Errore nella lettura dalla FIFO\n");
            break;
        }
        
        if (res > 0) {
            buffer[res] = '\0'; /* Assicura terminazione stringa */
            
            /* Controlla messaggio di fine */
            if (strcmp(buffer, "FINE") == 0) {
                printf("Reader: ricevuto messaggio di terminazione\n");
                break;
            }
            
            printf("Reader: ricevuto -> %s\n", buffer);
        }
    }

    close(pipe_fd);
    printf("Reader %d: operazioni completate\n", getpid());
    
    return 0;
}

int write_fifo(int pipe_fd,char* string){

    int res;
    int open_mode = O_WRONLY;
    
    int counter = 1;
    
    /* Creazione della FIFO se non esistente */
    if (access(FIFO_NAME, F_OK) == -1) {
        res = mkfifo(FIFO_NAME, 0666);
        if (res != 0) {
            fprintf(stderr, "Errore: impossibile creare FIFO %s\n", FIFO_NAME);
            exit(EXIT_FAILURE);
        }
        printf("FIFO creata: %s\n", FIFO_NAME);
    }

    printf("Writer %d: apertura FIFO in modalita' scrittura...\n", getpid());
    pipe_fd = open(FIFO_NAME, open_mode);
    
    if (pipe_fd == -1) {
        fprintf(stderr, "Errore: impossibile aprire FIFO per scrittura\n");
        exit(EXIT_FAILURE);
    }
    
    printf("Writer %d: FIFO aperta con successo (fd=%d)\n", getpid(), pipe_fd);


    printf("Writer: invio -> %s\n", string);
    res = write(pipe_fd, string, strlen(string) + 1);
        
    if (res == -1) {
        fprintf(stderr, "Errore nella scrittura sulla FIFO\n");            break;
    }
        
        
    
    close(pipe_fd);
    printf("Writer %d: operazioni completate\n", getpid());
    
    return 0;
}

int remove_fifo() {
    if (access(FIFO_NAME, F_OK) == -1) {
        printf("FIFO %s non esiste\n", FIFO_NAME);
        return 1;
    }
    
    if (unlink(FIFO_NAME) == -1) {
        fprintf(stderr, "Errore: impossibile rimuovere FIFO %s\n", FIFO_NAME);
        return -1;
    }
    
    printf("FIFO rimossa con successo: %s\n", FIFO_NAME);
    return 0;
}

/*===================THREAD==============*/

void* generate_number(){
/*la thread 1 deve generare dei numeri interi positivi casuali e
posizionarli in una FIFO di dimensione massima fornita come parametro dalla linea di
comando; */


}

/*==============SIGNAL HANDLER===============*/
void signal_handler(int sig) {
    printf("\n[Processo A] Ricevuto segnale %d. Terminazione...\n", sig);
    running = 0;
    
   
}




/*===========MAIN===================*/
int main(){

    int pipe_fd;



    create_fifo();
    



    /*Seed random con PID*/
    srand(getpid());
    /*Equivale a cntrl+c*/
    signal(SIGINT,signal_handler);

    pthread_create(&thread1,NULL,generate_number,NULL);



    /*Aspetta terminazione thread*/
    pthread_join(thread1,NULL);


    return 0;

}
