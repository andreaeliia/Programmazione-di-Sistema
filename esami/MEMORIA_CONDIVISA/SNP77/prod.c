#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#define SEM_NAME "/semaphore_shared_memory"

#define MAX_RECORD 200

/*===========STRUTTURA PER FILE CONDIVISO==========*/
typedef struct {
    char time_stamp[64];
    int number;
}record;

typedef struct 
{
    record array[MAX_RECORD];
    int items;
}shared_memory_t;

/*========VARIABILI GLOBALI============*/
sem_t *semaphore;
int continua_esecuzione =1;
shared_memory_t* shared_memory;

/* ========== GESTIONE SEGNALI ========== */
void signal_handler(int sig) {
    printf("\n[Processo A] Ricevuto segnale %d. Terminazione...\n", sig);
    continua_esecuzione = 0;
}

/*=============SEMAFORO=========*/
int init_semaphore(){

    semaphore = sem_open(SEM_NAME, O_CREAT,0644,1);

    if(semaphore == SEM_FAILED){
        perror("sem_open");
        return -1;
    }

    printf("[PROCESSO A] Semaforo inizializzato\n");
    return 0;
}

void cleanup_semaphore(){
    /* Chiudiamo il semaforo */
    if(semaphore != NULL){
        sem_close(semaphore);

        /* Rimuovi solo se esiste*/
        if(sem_unlink(SEM_NAME) == -1) {
            if(errno != ENOENT) {
                /* In questo caso ignoriamo l'errore.
                Avendo fatto la logica del "qualsiasi processo puo' avviare il semaforo"
                per fare il cleanup ci sara' un errore se il semaforo e' stato gia chiuso. 
                In questo caso l'errore non appare */

                perror("sem_unlink");  
            }
        }
    }
    printf("Cleanup del semaforo completato...\n");
}
/*=========SHARED_MEMORY============*/
int init_shared_memory(){
    int shm_fd;

    /*Creiamo e apriamo la memoria condivisa POSIX*/
    shm_fd = shm_open("/shared_data", O_CREAT | O_RDWR,0644);
    if(shm_fd == -1){
        perror("shm_open");
        return -1;
    }

    /*Impostiamo la dimensione*/
    if(ftruncate(shm_fd , sizeof(shared_memory_t))== -1){
        perror("ftruncate");
        return -1;
    }
    /*Mappiamo il file in memoria*/
    shared_memory = mmap(NULL,sizeof(shared_memory_t),
                                    PROT_READ | PROT_WRITE,
                                    MAP_SHARED,shm_fd,0);

    if(shared_memory == MAP_FAILED){
        perror("mmap");
        close(shm_fd);
        return -1;
    }

    close(shm_fd);
    printf(" File mappato in memoria\n");
    return 0;
}

int add_record_to_array(record* record){
     /* Acquisisce semaforo per accesso esclusivo */
    if (sem_wait(semaphore) == -1) {
        perror("sem_wait");
        return -1;
    }

    /*COntrolliamo lo spazio disponibile*/
    if(shared_memory->items>=MAX_RECORD){
        printf("Array pieno,reset\n");
        shared_memory->items =0 ; /*Reset array fittizzio*/
    }

    /*copiamo il recordo nell'array*/
    strcpy(shared_memory->array[shared_memory->items].time_stamp,record->time_stamp);
    shared_memory->array[shared_memory->items].number = record->number;

    /*incrementiamo items*/
    shared_memory->items++;

    printf("Record aggiunto in posizione %d: %s | %d",shared_memory->items -1,
                        shared_memory->array[shared_memory->items-1].time_stamp,
                        shared_memory->array[shared_memory->items-1].number);

    /* Sincronizza su disco */
    if (msync(shared_memory, sizeof(shared_memory_t), MS_SYNC) == -1) {
        perror("msync");
        /*Non è fatale, continua */
    }

    /* Rilascia semaforo */
    if (sem_post(semaphore) == -1) {
        perror("sem_post");
        return -1;
    }

    return 0;  /* Successo */
}
void cleanup_shared_memory(){
    printf("Cleanup risorse...\n");

    /* Unmap della memoria */
    if(shared_memory != MAP_FAILED && shared_memory != NULL){
        if(munmap(shared_memory, sizeof(shared_memory_t)) == -1){
            perror("munmap");
        } else {
            printf("Memoria unmappata con successo\n");
        }
        shared_memory = NULL;  /*Evita double-free */
    }

    /*(Rimuovi memoria condivisa POSIX */
    /* ATTENZIONE: Solo UN processo dovrebbe fare questo! */
    if(shm_unlink("/shared_data") == -1){
        if(errno != ENOENT){  /*Ignora "non esiste" */
            perror("shm_unlink");
        }
    } else {
        printf(" Memoria condivisa rimossa\n");
    }

    printf("Cleanup della memoria mappata completato\n");
}
/*=============UTILS=============*/
record generate_record(){
    record record;
    int random_number;
    time_t ora;
    struct tm *timeinfo;
    char timestamp[64];

    random_number = rand() % 10000;

        /*TIMESTAMP*/
    ora = time(NULL);
    timeinfo = localtime(&ora);
    strftime(timestamp,sizeof(timestamp),"%H:%M:%S",timeinfo); 

    record.number = random_number;
    strcpy(record.time_stamp, timestamp);

    return record;

}

int init_shared_array(){
    /* Azzera il contatore */
    shared_memory->items = 0;

    /* Azzerra tutto l'array (non può fallire) */
    memset(shared_memory->array, 0, sizeof(shared_memory->array));

    /* Sincronizza TUTTA la struttura su disco */
    if(msync(shared_memory, sizeof(shared_memory_t), MS_SYNC) == -1){
        perror("msync");
        return -1;
    }

    printf("Array inizializzato\n");
    return 0;
}
int main(){

    record record;
    int sleep_time;


    sem_unlink(SEM_NAME);
    printf("=== PROCESSO A - Avvio ===\n");

    /*Seed random con PId*/
    srand(getpid());

    /*Segnali*/
    signal(SIGINT,signal_handler);
    signal(SIGTERM,signal_handler);

    /*iniziamo il semaforo*/
    if(init_semaphore() == -1){
        exit(EXIT_FAILURE);
    }

    if(init_shared_memory() == -1){
        exit(EXIT_FAILURE);
    }

    if(init_shared_array()==-1){
        exit(EXIT_FAILURE);
    }

    /*loop principale*/
    while (continua_esecuzione)
    {
        /*Generiamo il record*/
        record = generate_record();

        /*Scriviamo sul file condiviso*/
        if(add_record_to_array(&record) == -1){
            printf("Errore nella scrittura\n");
            break;
        }

        sleep_time = (rand()% 5) +1;
        printf("Attesa di %d secondi...\n",sleep_time);
        sleep(sleep_time);
    }

    cleanup_shared_memory();
    cleanup_semaphore();

    printf("================TERMINATO================\n");

    return 0;

}