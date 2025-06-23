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
#include <semaphore.h>  // Per i semafori POSIX

typedef struct 
{
    int counter;
    int operation_done;
    char last_process[20];
    sem_t mutex;
}SharedData;


#define SHN_NAME "/my_shared_memory"  //nome della cartella condivisa



void critical_section_work(SharedData *data, const char* process_name,int iteration){
    for (int  i = 0; i < iteration; i++)
    {
        printf("%s: Tentativo di entrare nella sezione critica....\n",process_name);

        //Entra nella sezione cristica
        if(sem_wait(&data->mutex)==-1){
            perror("sem_wait error");
            exit(1);
        }

        printf("🔒 %s: DENTRO la sezione critica\n", process_name);


        //Sezione critica

        int old_counter = data->counter;

        printf("%s :Leggo counter = %d\n",process_name,old_counter);
        

        //Simuliamo il tempo di esecuzione
        usleep(100000);


        data->counter = old_counter + 1;
        data->operation_done++;
        strcpy(data->last_process, process_name);

        printf("%s: Scrivo counter = %d\n", process_name, data->counter);
        printf("🔓 %s: ESCO dalla sezione critica\n", process_name);

    

        //uscita dalla sezione critica
        if(sem_post(&data->mutex) == -1){
            perror("sem_post");
            exit(1);
        }



         // Lavoro fuori dalla sezione critica
        printf("%s: Lavoro fuori dalla sezione critica...\n", process_name);
        usleep(50000);  // 50ms di lavoro non critico
    }
    
}


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
    shared_data->operation_done = 0;
    strcpy(shared_data->last_process,"NESSUNO");
   

    //inizializziamo il semaforo
    // 🎯 INIZIALIZZA IL SEMAFORO
    // sem_init(semaforo, condiviso_tra_processi, valore_iniziale)
    if (sem_init(&shared_data->mutex, 1, 1) == -1) {
        perror("sem_init");
        exit(1);
    }
    
    printf("Dati iniziali: counter=%d\n", shared_data->counter);
    printf("Semaforo inizializzato (valore=1 = libero)\n\n");

    //Fork per creare un processo figlio

    pid_t pid1 = fork();

    if(pid1 == -1){
        //Failed
        perror("fork failed");
        exit(1);
    } if (pid1 == 0) {
        //processo figlio
         // === PRIMO FIGLIO ===
        printf("🟢 FIGLIO-1 avviato (PID: %d)\n", getpid());
        critical_section_work(shared_data, "FIGLIO-1", 3);
        printf("🟢 FIGLIO-1 terminato\n");
        exit(0);
        

    }


    //il padre crea il secondo figlio
    
    pid_t pid2 = fork();

    if (pid2 == -1) {
        perror("fork");
        exit(1);
    }
    
    if (pid2 == 0) {
        // === SECONDO FIGLIO ===
        printf("🔵 FIGLIO-2 avviato (PID: %d)\n", getpid());
        critical_section_work(shared_data, "FIGLIO-2", 3);
        printf("🔵 FIGLIO-2 terminato\n");
        exit(0);
    }


    // === PADRE ===
    printf("🟡 PADRE in attesa dei figli...\n");
    
    // Aspetta entrambi i figli
    wait(NULL);
    wait(NULL);
    
    printf("\n=== RISULTATI FINALI ===\n");
    printf("Counter finale: %d\n", shared_data->counter);
    printf("Operazioni totali: %d\n", shared_data->operation_done);
    printf("Ultimo processo: %s\n", shared_data->last_process);
    printf("Risultato atteso: counter=6, operazioni=6\n");
    
    if (shared_data->counter == 6 && shared_data->operation_done == 6) {
        printf("✅ SUCCESSO: Nessuna race condition!\n");
    } else {
        printf("❌ ERRORE: Race condition rilevata!\n");
    }


    //CleanUp della memoria
    sem_destroy(&shared_data->mutex);
    munmap(shared_data,sizeof(SharedData));
    shm_unlink(SHN_NAME);

    return 0;


}
