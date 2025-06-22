#include "apue.h"
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>

sem_t *semaforo;

void* worker_thread(void* arg) {
    int thread_id = *((int*)arg);
    
    for (int i = 0; i < 3; i++) {
        printf("Thread %d: richiedo accesso...\n", thread_id);
        
        // TODO: Prendi il semaforo
        sem_wait(semaforo);
        
        printf("Thread %d: ACCESSO CONCESSO - lavoro...\n", thread_id);
        sleep(1);  // Simula lavoro
        printf("Thread %d: finito, rilascio\n", thread_id);
        
        // TODO: Rilascia il semaforo
        sem_post(semaforo);
        
        sleep(1);  // Pausa tra iterazioni
    }
    
    return NULL;
}

int main(void) {
    pthread_t thread1, thread2;
    int id1 = 1, id2 = 2;
    
    // TODO: Crea semaforo con nome "/test_sem" e valore iniziale 1
    semaforo = sem_open("/my_semaphore", O_CREAT, 0644, 1);
    if (semaforo == SEM_FAILED) {
        err_sys("sem_open failed");
    }
    
    printf("Creazione thread...\n");
    
    pthread_create(&thread1, NULL, worker_thread, &id1);
    pthread_create(&thread2, NULL, worker_thread, &id2);
    
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    // TODO: Chiudi e rimuovi semaforo
    sem_close(semaforo);
    sem_unlink("/my_semaphore");
    
    printf("Tutti finiti!\n");
    return 0;
}