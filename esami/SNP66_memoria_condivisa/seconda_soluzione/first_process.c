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

#define FILE_SIZE 4112
#define MAX_RECORDS 50
#define RECORD_SIZE 128
#define SEM_NAME "/sync_sem"

typedef struct {
    int next_position;
    int record_count;
    char data[FILE_SIZE];
    volatile int processes_finished;
} SharedMemory;

char* get_timestamp() {
    time_t rawtime;
    struct tm * timeinfo;
    static char buffer[30];

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    return buffer;
}

void write_record(SharedMemory* shared_mem, int process_id, sem_t* semaphore) {
    if(sem_wait(semaphore) == -1) {
        perror("sem_wait");
        return;
    }

    if(shared_mem->record_count >= MAX_RECORDS) {
        printf("Processo %d: File pieno, termino\n", process_id);
        sem_post(semaphore);
        return;
    }

    int random_number = rand() % 10000;
    char message[RECORD_SIZE];
    snprintf(message, sizeof(message), " | %d | %d | %s | %d |\n", 
             process_id, random_number, get_timestamp(), process_id);
    
    int write_pos = shared_mem->next_position;
    int record_len = strlen(message);

    if(write_pos + record_len >= FILE_SIZE - 1) {
        printf("Processo %d: Buffer pieno, resetto posizione\n", process_id);
        write_pos = 0;
        shared_mem->next_position = 0;
    }

    memcpy(shared_mem->data + write_pos, message, record_len);
    shared_mem->next_position = write_pos + record_len;
    shared_mem->record_count++;

    printf("Processo %d: Scritto record %d alla posizione %d\n", 
           process_id, shared_mem->record_count, write_pos);
    printf("  Contenuto: %s", message);

    if(sem_post(semaphore) == -1) {
        perror("sem_post");
    }
}

void process_worker(int process_id, SharedMemory* shared_mem, sem_t* semaphore) {
    printf("Processo %d (PID %d) avviato\n", process_id, getpid());
    
    srand(time(NULL) + getpid());

    for (int i = 0; i < 10; i++) {
        int sleep_time = (rand() % 3) + 1;
        printf("Processo %d: iterazione %d, pausa %d secondi\n", process_id, i+1, sleep_time);
        sleep(sleep_time);

        write_record(shared_mem, process_id, semaphore);

        if(shared_mem->record_count >= MAX_RECORDS) {
            printf("Processo %d: Limite record raggiunto, termino\n", getpid());
            break;
        }
    }

    printf("Processo %d terminato\n", process_id);
}

int main(void) {
    const char* filename = "shared_file.dat";
    int fd;
    SharedMemory* shared_mem;
    sem_t* semaphore;

    printf("PRIMO PROCESSO\n");
    printf("===============================================\n\n");

    fd = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open file");
        exit(EXIT_FAILURE);
    }

    if(ftruncate(fd, FILE_SIZE) == -1) {
        perror("ftruncate");
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("File %s creato con dimensione %d bytes\n", filename, FILE_SIZE);

    shared_mem = mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(shared_mem == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("Memory mapping completato\n");

    memset(shared_mem, 0, FILE_SIZE);
    shared_mem->next_position = 0;
    shared_mem->record_count = 0;
    shared_mem->processes_finished = 0;

    sem_unlink(SEM_NAME);
    semaphore = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0644, 1);
    if(semaphore == SEM_FAILED) {
        perror("sem_open");
        munmap(shared_mem, sizeof(shared_mem));
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("Semaforo POSIX creato\n\n");

    close(fd);
    process_worker(getpid(), shared_mem, semaphore);

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

    if (finished_count == 2) {
        printf("\n=============\n");
        printf("RISULTATI FINALI DI ENTRAMBI I PROCESSI\n");
        printf("=============\n");
        printf("Record totali scritti: %d\n", shared_mem->record_count);
        printf("Posizione finale: %d\n", shared_mem->next_position);
        
        printf("\n=== CONTENUTO COMPLETO FILE ===\n");
        printf("%.*s\n", shared_mem->next_position, shared_mem->data);
        
        if (msync(shared_mem, FILE_SIZE, MS_SYNC) == -1) {
            perror("msync");
        } else {
            printf("Dati sincronizzati su disco\n");
        }
        
        printf("\nEntrambi i processi completati con successo!\n");
        printf("File risultato salvato in: shared_file.txt\n");
        printf("=============\n");
    } else {
        printf("Aspetto che l'altro processo finisca...\n");
        
        for(int i = 0; i < 30; i++) {
            sleep(1);
            
            if(sem_trywait(semaphore) == 0) {
                if(shared_mem->processes_finished >= 2) {
                    printf("\n=== CONTENUTO FINALE ===\n");
                    printf("%.*s\n", shared_mem->next_position, shared_mem->data);
                    sem_post(semaphore);
                    break;
                }
                sem_post(semaphore);
            }
        }
    }

    sem_close(semaphore);
    if (finished_count == 2) {
        sem_unlink(SEM_NAME);
    }
    munmap(shared_mem, FILE_SIZE);
    
    printf("\nProgramma completato con successo\n");
    printf("File risultato salvato in: %s\n", filename);
    
    return 0;
}