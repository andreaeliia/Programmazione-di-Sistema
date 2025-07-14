

/*Questo file entra nella memoria mappata , legge una riga random e la toglie (Entra nel semaforo di prod.c)*/
/*Scrive su un file l'elemento levato(magari su memoria mappata)/altro semaforo simile al snp66*/

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


#define SEM_NAME_SHARED_MEMORY "/semaphore_shared_memory"
#define SEM_NAME_FILE "/semaphore_file"
#define FILENAME "shared_file.txt"
#define FILE_SIZE 4096
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


typedef struct {
    int fd;
    char* mapped_memory;
    size_t file_size;
    int* current_offset; /*Offset correnmte nel file*/
}shared_file_t;

/*========VARIABILI GLOBALI============*/
sem_t *semaphore_sh_memory;
sem_t *semaphore_file;
int continua_esecuzione =1;
shared_memory_t* shared_memory;
shared_file_t shared_file;


/* ========== GESTIONE SEGNALI ========== */
void signal_handler(int sig) {
    printf("\n[Processo A] Ricevuto segnale %d. Terminazione...\n", sig);
    continua_esecuzione = 0;
}

/*=============SEMAFORO=========*/
int init_semaphore_file(){
    /*Verifichiamo se il semaforo esiste gia*/
    semaphore_file= sem_open(SEM_NAME_FILE,0);
    
    if(semaphore_file == SEM_FAILED){
        /*se non esiste lo creo*/
        semaphore_file = sem_open(SEM_NAME_FILE, O_CREAT | O_EXCL,0644,1);

        if(semaphore_file ==  SEM_FAILED){

            if(errno == EEXIST){
                /*Qualcunaltro lo ha creato nel frattempo RACE CONDITIOn*/
                printf("[PROCESSO A] Race condition, riprovo apertura\n");
                semaphore_file = sem_open (SEM_NAME_FILE,0);
                if(semaphore_file == SEM_FAILED){
                    perror("sem_open final attempt\n");
                    return -1;
                }
            }else{

                perror("sem_open O_EXCL");
                return -1;
            }

        }else{
            /*semaforo creato */
        }

    }else{
        /*il semaforo esiste gia*/
    }



    printf("[PROCESSO A] Semaforo inizializzato\n");
    return 0;
}
int init_semaphore_shared_memory(){
    /*Il semaforo esiste gia*/
    semaphore_sh_memory= sem_open(SEM_NAME_SHARED_MEMORY,0);
    
    if(semaphore_sh_memory == SEM_FAILED){
        perror("sem_open");
        return -1;
    }



    printf("[PROCESSO A] Semaforo inizializzato\n");
    return 0;
}
void cleanup_semaphore(sem_t* semaphore,const char* SEM_NAME){
    /* Chiudiamo il semaforo */
    if(semaphore != NULL){
        sem_close(semaphore);
        
        /* Rimuovi solo se esiste */
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
    printf("[Processo A] Cleanup del semaforo completato...\n");
}
/*=========SHARED_FILE============*/
int init_shared_file(){
    struct stat file_info;
    

    /*Apre/Crea il file*/
    shared_file.fd = open (FILENAME,O_CREAT | O_RDWR,0644);
    if(shared_file.fd == -1){
        perror("open");
        return -1;
    }

    /*Verifichiamo la dimensione del file*/
    if(fstat(shared_file.fd,&file_info)== -1){
        perror("fstat");
        close(shared_file.fd);
        return -1;
    }

    /*Se il file e' vuoto, impostiamo la dimensione fissa*/
    if(file_info.st_size == 0){
        if(ftruncate(shared_file.fd,FILE_SIZE) == -1){
            perror("ftruncate");
            close(shared_file.fd);
            return -1;
        }
        shared_file.file_size = FILE_SIZE;
        printf("[PROCESSO A] FIle creato con dimensione %d bytes\n",FILE_SIZE);        
    }else{
        shared_file.file_size = file_info.st_size;
        printf("[PROCESSO A] File esistente con dimensione %ld bytes\n",file_info.st_size);
    }

    /*Mappiamo il file in memoria*/
    shared_file.mapped_memory = mmap(NULL,shared_file.file_size,
                                    PROT_READ | PROT_WRITE,
                                    MAP_SHARED,shared_file.fd,0);
    if(shared_file.mapped_memory == MAP_FAILED){
        perror("mmap");
        close(shared_file.fd);
        return -1;
    }

    /*Puntatore all'offset corrente*/
    shared_file.current_offset = (int*)shared_file.mapped_memory;
    
    printf("[PROCESSO A] File mappato in memoria\n");
    return 0;
}

void cleanup_shared_file(){
    printf("[Processo A] Cleanup risorse...\n");

    /*Unmap della memoria*/
    if(shared_file.mapped_memory != MAP_FAILED){
        munmap(shared_file.mapped_memory,shared_file.file_size);
    }

    /*chiudiamo file descriptor*/
    if(shared_file.fd != -1){
        close(shared_file.fd);
    }


    printf("[PROCESSO A] Cleanup della memoria mappata completato\n");
}

int write_to_shared_file(record record){
    int safe_offset;
    int record_len;
    char *timestamp;
    int number;
    int write_len;
    char *write;

    /*Allochiamo memoria per la stringa timestamp*/
    timestamp = malloc(strlen(record.time_stamp) + 1);
    if(timestamp == NULL){
        perror("malloc timestamp");
        return -1;
    }

    /*Allochiamo memoria per la stringa write*/
    write = malloc(64);  // spazio sufficiente per "|timestamp|numero|"
    if(write == NULL){
        perror("malloc write");
        free(timestamp);
        return -1;
    }

    /*Copiamo il timestamp dal record*/
    snprintf(timestamp, strlen(record.time_stamp) + 1, "%s", record.time_stamp);
    number = record.number;

    /*Prepariamo la stringa da scrivere nella memoria mappata*/
    sprintf(write, "|%s|%d|\n", timestamp, number);

    /*Calcoliamo la lunghezza del record*/
    write_len = strlen(write);
    record_len = write_len;

    /*acquisiamo il semaforo*/
    if(sem_wait(semaphore_file) == -1){
        perror("sem_wait");
        free(timestamp);
        free(write);
        return -1;
    }

    /*Leggiamo l'offset corrente*/
    safe_offset = sizeof(int) + *shared_file.current_offset;

    /*Verifichiamo lo spazio disponibile*/
    if(safe_offset + record_len >= shared_file.file_size){
        /*in questo caso quando l'offset sfora si perdono tutti i dati vecchi*/
        printf("[PROCESSO A] File pieno reset offset\n");
        *shared_file.current_offset = 0;
        safe_offset = sizeof(int);
    }

    /*Scrittura nella memoria mappata*/
    /*memcpy(DESTINAZIONE, SORGENTE, QUANTI_BYTES);*/
    memcpy(shared_file.mapped_memory + safe_offset, write, write_len);

    /*aggiorniamo l'offset*/
    *shared_file.current_offset += write_len;

    /*Forziamo la sincronizzazione su disco*/
    /*MS_SYNC Blocca fino a completamento*/
    if(msync(shared_file.mapped_memory, shared_file.file_size, MS_SYNC) == -1){
        perror("msync");
    }

    printf("[Processo A] Scritto: [%s] %d\n", timestamp, number);
    printf("[Processo A] Offset aggiornato: %d\n", *shared_file.current_offset);

    /*Rilascia semaforo*/
    if(sem_post(semaphore_file) == -1){
        perror("sem_post");
        free(timestamp);
        free(write);
        return -1;
    }

    /*Liberiamo la memoria allocata*/
    free(timestamp);
    free(write);

    return 0;
}



/*=====SHARED_MEMORY============*/
int init_shared_memory(){
    int shm_fd;

    /*Creiamo e apriamo la memoria condivisa POSIX*/
    shm_fd = shm_open("/shared_data", O_RDWR,0644);
    if(shm_fd == -1){
        perror("shm_open");
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
    printf(" Shared memory mappata in memoria\n");
    return 0;
}

record remove_record(){

    int random_index;
    int i;
    record removed_record;
    record empty;


    /*inizializzazione del record di error. Non so se si puo' fare */
    strcpy(empty.time_stamp, "");
    empty.number = -1;

    if (sem_wait(semaphore_sh_memory) == -1) {
        perror("sem_wait");
        return empty ;
    }   
    /* Controllo se array vuoto */
    if (shared_memory->items == 0) {
        printf("Errore: nessun record da rimuovere\n");
        return empty;
    }


    random_index =  rand() % shared_memory->items;
    

    removed_record = shared_memory->array[random_index];


    /*Shift degli elementi*/
    for (i = random_index ;i < shared_memory->items -1;i++)
    {
        shared_memory->array[i] = shared_memory->array[i+1];
    }

    /*Decrementiamo il contatore*/
    shared_memory->items--;


     /* Sincronizza su disco */
    if (msync(shared_memory, sizeof(shared_memory_t), MS_SYNC) == -1) {
        perror("msync");
    }
    
    /* Rilascia semaforo */
    if (sem_post(semaphore_sh_memory) == -1) {
        perror("sem_post");
        //return empty;
    }

    

    return removed_record;
}



int main(){

    record record;
    int sleep_time;

    printf("=== PROCESSO A - Avvio ===\n");

    /*Seed random con PId*/
    srand(getpid());


    /*Segnali*/
    signal(SIGINT,signal_handler);
    signal(SIGTERM,signal_handler);
     /*iniziamo il semaforo*/


    if(init_semaphore_shared_memory() == -1){
        exit(EXIT_FAILURE);
    }

    /*iniziamo il semaforo*/
    if(init_semaphore_file() == -1){
        exit(EXIT_FAILURE);
    }
    
    /*inizializziamo la shared_memory*/
    if(init_shared_memory() == -1){
        exit(EXIT_FAILURE);
    }

    /*Inizializziamo il file condiviso */
    if(init_shared_file() == -1){
        exit(EXIT_FAILURE);
    }

    /*loop principale*/
    while (continua_esecuzione)
    {
        /*Generiamo il record*/
        
        record = remove_record();

        
        if(record.number == -1){
            printf("[PROCESSO A] Errore nella rimozione\n");
            break;
        }

        if(write_to_shared_file(record)==-1){
            printf("[PROCESSO A] errore nella scrittura del file\n");
        }

        sleep_time = (rand()% 5) +1;
        printf("[PROCESSO A] Attesa di %d secondi...",sleep_time);
        sleep(sleep_time);
    }
    

    /*Non devo fare il cleanup di questo semaforo qui*/
    //cleanup_semaphore(semaphore_sh_memory,SEM_NAME_SHARED_MEMORY);
    cleanup_semaphore(semaphore_file,SEM_NAME_FILE);
    cleanup_shared_file();
    


    return 0;
    
}

