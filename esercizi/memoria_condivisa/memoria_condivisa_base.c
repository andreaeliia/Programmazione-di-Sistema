/*
 * ESERCIZIO 2: Memoria Condivisa Base
 * 
 * Obiettivo: Capire come creare e usare memoria condivisa tra processi
 * 
 * Compila con: gcc -o ex2 exercise2.c -lrt
 * Esegui con: ./ex2
 * 
 * NOTA: -lrt è necessario per le funzioni POSIX shared memory
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>    // per mmap, munmap
#include <sys/stat.h>    // per le costanti di modo
#include <fcntl.h>       // per le costanti O_*
#include <string.h>


typedef struct 
{
    int counter;
    char message[100];
    int proccess_count;
}SharedData;


#define SHN_NAME "/my_shared_memory"  //nome della cartella condivisa


int main(){
    printf("======ESERCIZIO MEMORIA CONDIVISA======");

    //rimozione della memoria condivisa precedente nel caso esiste
    shm_unlink(SHN_NAME);


    //Creazione di un nuovo segmento di memoria condivisa

    int shm_fd = shm_open(SHN_NAME, O_CREAT | O_RDWR,0666);
    if(shm_fd == -1){
        perror("Errore shm_open");
        exit(1);
    }



    //Impostiamo la dimensione della memoria condivisa
    if(ftruncate(shm_fd,sizeof(SharedData))==-1){
        perror("ftruncate failed");
        exit(1);
    }


    //Mappiamo la memoria condivisa
    SharedData *shared_data = mmap(NULL,sizeof(SharedData),PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd,0);

    if(shared_data == MAP_FAILED){
        perror("mmap failed");
        exit(1);
    }


    //Inizializziamo i dati condivisi
    shared_data-> counter =0;
    strcpy(shared_data->message,"Messaggio iniziale");
    shared_data-> proccess_count = 0;

    
    printf("Memoria condivisa creata e inizializziata\n");
    printf("Dati iniziali : counter =%d  message='%s'\n",shared_data->counter,shared_data->message);


    //Fork per creare un processo figlio

    pid_t pid = fork();

    if(pid == -1){
        //Failed
        perror("fork failed");
        exit(1);
    } else if (pid == 0) {
        //processo figlio
        printf("FIGLIO : PID : %d \n", getpid());

        //Incrementiamo il process_count

        shared_data->proccess_count++;

        //modifica dei dati da parte del figlio
        for (int i = 0; i < 5; i++)
        {
            shared_data->counter++;
            printf("Figlio: counter = %d\n", shared_data->counter);
            sleep(1);

        }
        //modifichiamo il messaggio da parte del figlio

        strcpy(shared_data->message, "Modificato dal figlio \n");
        printf("figlio: messaggio cambiato\n");

        //terminiamo il figlio

        printf("FIGLIO: Terminato");
        exit(0);
        

    }else{
        
        printf("PADRE : PID %d",getpid());
        

        //Incrementiamo il process count
        shared_data->proccess_count++;

           printf("Padre: process_count incrementato a %d\n", shared_data->proccess_count);
        
        // Il padre legge i dati mentre il figlio li modifica
        for (int i = 0; i < 7; i++) {
            printf("Padre: counter = %d, message = '%s'\n", 
                   shared_data->counter, shared_data->message);
            sleep(1);
        }


        //Aspettiamo che il figlio termini
        wait(NULL);

        printf("\n--- RISULTATI FINALI ---\n");
        printf("Counter finale: %d\n", shared_data->counter);
        printf("Messaggio finale: '%s'\n", shared_data->message);
        printf("Process count finale: %d\n", shared_data->proccess_count);

    }


    //CleanUp della memoria
    if(munmap(shared_data,sizeof(SharedData))==-1){
        perror("munmap failed");

    }

    //Il padre rimuove la memoria condivisa
    if(pid !=0){
        if(shm_unlink(SHN_NAME)==-1){
            perror("shm_unlink failed");
        }
        printf("Memoria condivisa rimossa");

    }

    return 0;


}
