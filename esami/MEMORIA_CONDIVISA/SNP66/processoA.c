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


#define SEM_NAME "/semaphore"
#define FILENAME "shared_file.txt"
#define FILE_SIZE 4096
#define MAX_RECORD_SIZE 200


/*===========STRUTTURA PER FILE CONDIVISO==========*/
typedef struct {
    int fd;
    char* mapped_memory;
    size_t file_size;
    int* current_offset; /*Offset correnmte nel file*/
}shared_file_t;

/*========VARIABILI GLOBALI============*/
sem_t *semaphore;
int continua_esecuzione =1;
shared_file_t shared_file;

/* ========== GESTIONE SEGNALI ========== */
void signal_handler(int sig) {
    printf("\n[Processo A] Ricevuto segnale %d. Terminazione...\n", sig);
    continua_esecuzione = 0;
}

/*=============SEMAFORO=========*/
int init_semaphore(){
    /*Verifichiamo se il semaforo esiste gia*/
    semaphore = sem_open(SEM_NAME,0);
    
    if(semaphore == SEM_FAILED){
        /*se non esiste lo creo*/
        semaphore = sem_open(SEM_NAME, O_CREAT | O_EXCL,0644,1);

        if(semaphore ==  SEM_FAILED){

            if(errno == EEXIST){
                /*Qualcunaltro lo ha creato nel frattempo RACE CONDITIOn*/
                printf("[PROCESSO A] Race condition, riprovo apertura\n");
                semaphore = sem_open (SEM_NAME,0);
                if(semaphore == SEM_FAILED){
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


    /*il processo vede se esiste gia il semaforo */
    semaphore = sem_open(SEM_NAME,O_CREAT,0644,1);
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

int write_to_shared_file(const char* record){
    int safe_offset;
    int record_len;



    record_len = strlen(record);

    /*acquisiamo il semaforo*/
    if(sem_wait(semaphore) == -1){
        perror("sem_wait");
        return -1;
    }

    /*Leggiamo l'offset corrente*/
    safe_offset = sizeof(int) + *shared_file.current_offset;

    /*Verifichiamo lo spazio disponibile*/
    if(safe_offset + record_len>= shared_file.file_size){
        /*in questo caso quando l'offset sfora si perdono tutti i dati vecchi*/
        printf("[PROCESSO A] File pieno reset offset\n");
        *shared_file.current_offset = 0;
        safe_offset = sizeof(int);
    }

    /*Scrittura nella memoria mappata*/
    /*memcpy(DESTINAZIONE, SORGENTE, QUANTI_BYTES);*/
    memcpy(shared_file.mapped_memory + safe_offset, record,record_len);

    /*aggiorniamo l'offset*/
    *shared_file.current_offset += record_len;

    /*Forziamo la sincronizzazione su disco*/
    /*MS_SYNC Blocca fino a completamento*/
    if(msync(shared_file.mapped_memory,shared_file.file_size,MS_SYNC) == -1){
        perror("msync");
    }
    printf("[Processo A] Scritto: %s", record);
    printf("[Processo A] Offset aggiornato: %d\n", *shared_file.current_offset);


    /*Rilascia semaforo*/
    if(sem_post(semaphore) == -1){
        perror("sem_post");
        return -1;
    }
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
/*=============UTILS=============*/
char* generate_record(){
    static char record[MAX_RECORD_SIZE];
    int random_number;
    time_t ora;
    struct tm *timeinfo;
    char timestamp[64];

    random_number = rand() % 10000;


        /*TIMESTAMP*/
    ora = time(NULL);
    timeinfo = localtime(&ora);
    strftime(timestamp,sizeof(timestamp),"%H:%M:%S",timeinfo); /*Verificare se funziona*/

     /* Crea record: | ProcessoA | numero | timestamp | ProcessoA | */
    snprintf(record, MAX_RECORD_SIZE, 
             "| ProcessoA | %d | %s | ProcessoA |\n", 
             random_number, timestamp);
    
    return record;


}


int main(){

    char* record;
    int sleep_time;

    printf("=== PROCESSO A - Avvio ===\n");

    /*Seed random con PId*/
    srand(getpid());


    /*Segnali*/
    signal(SIGINT,signal_handler);
    signal(SIGINT,signal_handler);
    

    /*iniziamo il semaforo*/
    if(init_semaphore() == -1){
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
        record = generate_record();
        

        /*Scriviamo sul file condiviso*/
        if(write_to_shared_file(record) == -1){
            printf("[PROCESSO A] Errore nella scrittura\n");
            break;
        }

        sleep_time = (rand()% 5) +1;
        printf("[PROCESSO A] Attesa di %d secondi...",sleep_time);
        sleep(sleep_time);
    }
    


    return 0;
    
}