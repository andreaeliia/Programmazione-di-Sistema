

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <semaphore.h>
#include <signal.h>


#define SEM_NAME "/semaphore"


//====VARIABILI GLOBALI=====//
int continua_esecuzione = 1;
sem_t *semaphore;

void signal_handler(int sig) {

    printf("Ricevuto segnale %d. Iniziando terminazione...", sig);
    sem_close(semaphore);
    sem_unlink(SEM_NAME);
    continua_esecuzione = 0;
}


char* record_generate(){
    char* record;
    int random_number;


    

    /* | A | X | timestamp | A | */

    time_t ora = time(NULL);
    char* time_str = ctime(&ora);
    random_number = rand() % 10000;
    
    time_str[strlen(time_str)-1] = '\0'; /* rimuove newline */
    
    sprintf(record,"| A | %d | %s | A |\n",random_number,time_str);
    






    return  record; 
}

int main() {

    

    const char* filename = "test_file.txt";
    int fd;
    char* mapped_memory;
    struct stat file_info;
    char* record;
    int random_time;


    srand(getpid());


    /*Randomizziamo il tempo di scrittura del file*/
    random_time = (rand()+1) % 3;
    // Crea/apre file
    fd = open(filename, O_CREAT | O_RDWR, 0644);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }


     // Rimuove semaforo esistente (se presente)
    sem_unlink(SEM_NAME);
    
    // Crea semaforo con valore iniziale 1 (mutex)
    semaphore = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0644, 1);
    if (semaphore == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }


    signal(SIGINT,signal_handler);
    signal(SIGTERM,signal_handler);


    
    
    // Ottiene informazioni sul file
    if (fstat(fd, &file_info) == -1) {
        perror("fstat");
        close(fd);
        exit(EXIT_FAILURE);
    }
    
    // Mappa il file in memoria
    mapped_memory = mmap(NULL, file_info.st_size, 
                        PROT_READ | PROT_WRITE, 
                        MAP_SHARED, fd, 0);
    if (mapped_memory == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }
    
    while(continua_esecuzione){

        /*Scriviamo sul file*/
        if (sem_wait(semaphore) == -1) {
                perror("sem_wait");
                exit(EXIT_FAILURE);
            }

        // Scrive dati nel file


        record = record_generate();
        write(fd,record , strlen(record));






        if (sem_post(semaphore) == -1) {
            perror("sem_post");
            exit(EXIT_FAILURE);
        }





        
        // Legge da memoria mappata
        printf("Contenuto: %.*s\n", (int)file_info.st_size, mapped_memory);
        
        // Modifica in memoria (riflessa sul file)
        mapped_memory[0] = 'h';
        
        // Sincronizza con disco
        if (msync(mapped_memory, file_info.st_size, MS_SYNC) == -1) {
            perror("msync");
        }

    
        sleep(random_time);
    }
    
    // Cleanup
    if (munmap(mapped_memory, file_info.st_size) == -1) {
        perror("munmap");
    }
    
    return 0;
}