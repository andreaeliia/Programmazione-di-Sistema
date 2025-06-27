#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>

#define FILE_SIZE 4096
#define MAX_RECORDS 50
#define RECORD_SIZE 128
#define SEM_NAME "/sync_sem"

//Struttura per gestire la memoria condivisa
typedef struct {
    int next_position; // Prossima posizione di scrittura
    int record_count; //Numero di record scritti
    char data[FILE_SIZE];
    volatile int start;   //flag per aspettare il primo processo
    volatile int processes_finished;
}SharedMemory;

//Otteniamo il timestamp formattato
char* get_timestamp() {
    time_t rawtime;
    struct tm * timeinfo;
    static char buffer[30]; // buffer statico per contenere la stringa

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return buffer;
}

//Scriviamo i record nel file mappato
void write_record(SharedMemory* shared_mem, int process_id, sem_t* semaphore){
    //ACQuisiamo il semaforo
    if(sem_wait(semaphore)== -1){
        perror("sem_wait");
        return;
    }

    //Controllo se c'e' ancora spazio
    if(shared_mem->record_count >= MAX_RECORDS){
        printf("Processo %d: File pieno, termino\n",process_id);
        sem_post(semaphore);
        return;
    }

    //Generazione dati per il record
    int random_number = rand()%10000;
    char message [RECORD_SIZE];
    snprintf(message,sizeof(message),"%d_%d_%s_%d'\n",process_id,random_number,get_timestamp(),process_id);
   // printf(message);
    
    //Calcoliamo la posizione per la scrittura
    int write_pos = shared_mem->next_position;
    int record_len = strlen(message);

    //Verificiamo overflow con il buffer
    if(write_pos + record_len >= FILE_SIZE -1){
        printf("Processo %d: Buffer pieno, resetto posizione\n", process_id);
        write_pos = 0;
        shared_mem->next_position = 0;
    }

    //Scriviamo nella memoria mappata
    memcpy(shared_mem->data + write_pos, message,record_len);
    shared_mem->next_position = write_pos + record_len;
    shared_mem ->record_count++;

    printf("Processo %d: Scritto record %d alla posizione %d\n", 
           process_id, shared_mem->record_count, write_pos);
    printf("  Contenuto: %s", message);

    //Rilasciamo semaoforo
    if(sem_post(semaphore)==-1){
        perror("sem_post");
    }
}

void process_worker(int process_id, SharedMemory* shared_mem, sem_t* semaphore){
    printf("Processo %d (PID %d) avviato\n",process_id,getpid());

    //seed random diverso per ogni processo 
    srand(time(NULL)+getpid());
    
    //ciclo di scrittura
    for (int  i = 0; i < 10; i++)
    {
        //Intervallo casuale tra 1 e 3 secondi
        int sleep_time = (rand()% 3)+1;

        printf("Processo %d: iterazione %d, pausa %d secondi\n",process_id,i+1,sleep_time);
        sleep(sleep_time);

        //Scrive record
        write_record(shared_mem,process_id,semaphore);

        //Controllo se il file e' pieno
        if(shared_mem->record_count >= MAX_RECORDS){
            printf("Processo %d: Limite record raggiunto, termino \n",getpid());
            break;
        }
    }

    printf("Processo %d terminato\n",process_id);
}

int main(void){
    
    const char* filename = "shared_file.txt";
    int fd;
    SharedMemory* shared_mem;
    sem_t* semaphore;

    printf("MEMORY MAPPING CON SINCRONIZZAZIONE INTER-PROCESS\n");
    printf("===============================================\n\n");

    //Creazione/Apertura file
    fd = open(filename, O_RDWR ,0644);
    if (fd == -1 )
    {
        perror("open file");
        exit(EXIT_FAILURE);
    }

    //Estende il file della memoria desiderata
    if(ftruncate(fd,FILE_SIZE) == -1){
        perror("ftruncate");
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("File %s aperto con dimensione %d bytes\n",filename,FILE_SIZE);

    //MEMORY MAPPING
    shared_mem = mmap(NULL,FILE_SIZE,PROT_READ | PROT_WRITE,MAP_SHARED,fd,0);
    if(shared_mem ==  MAP_FAILED){
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("Memory mapping completato\n");

    // NON fare memset - usa memoria già inizializzata dal primo processo
    shared_mem->start = 1;

    //CREAZIONE SEMAFORO POSIX
    semaphore = sem_open(SEM_NAME, 0);
    
    if(semaphore == SEM_FAILED){
        perror("sem_open");
        munmap(shared_mem,FILE_SIZE);
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("Semaforo POSIX aperto\n\n");
    
    // Sblocca il primo processo
    sem_post(semaphore);

    close(fd);  //Chiudiamo il file descriptor perche' non serve piu
    process_worker(getpid(),shared_mem,semaphore);

    int finished_count;
    if (sem_wait(semaphore) == -1) {
        perror("sem_wait");
    } else {
        shared_mem->processes_finished++;
        finished_count = shared_mem->processes_finished;
        if (sem_post(semaphore) == -1) {
            perror("sem_post");
        }
    }

    printf("Processo %d terminato (%d/2 processi finiti)\n", getpid(), finished_count);

    // Solo l'ultimo processo stampa il risultato finale
    if (finished_count == 2) {
        printf("\n =============  \n");
        printf("RISULTATI FINALI DI ENTRAMBI I PROCESSI\n");
        printf("\n =============  \n");
        printf("Record totali scritti: %d\n", shared_mem->record_count);
        printf("Posizione finale: %d\n", shared_mem->next_position);
        
        printf("\n=== CONTENUTO COMPLETO FILE ===\n");
        printf("%.*s\n", shared_mem->next_position, shared_mem->data);
        
        // Sincronizza su disco
        if (msync(shared_mem, FILE_SIZE, MS_SYNC) == -1) {
            perror("msync");
        } else {
            printf("Dati sincronizzati su disco\n");
        }
        
        printf("\nEntrambi i processi completati con successo!\n");
        printf("File risultato salvato in: shared_file.txt\n");
        printf("\n =============  \n");
    } else {
        printf("Aspetto che l'altro processo finisca...\n");
        sleep(1);
    }

    // Cleanup
    sem_close(semaphore);
    if (finished_count == 2) {
        sem_unlink(SEM_NAME);  // Solo l'ultimo pulisce il semaforo
    }
    munmap(shared_mem, FILE_SIZE);
    
    printf("\nProgramma completato con successo\n");
    printf("File risultato salvato in: %s\n", filename);
    
    return 0;
}