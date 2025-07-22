/* process_p3.c - Processo P3 */
#include "sync_comm.h"

/* Ottiene memoria condivisa esistente */
int get_shared_memory(void) {
    int shmid;
    
    /* Attendi che P2 crei la memoria condivisa */
    while ((shmid = shmget(SHM_KEY, sizeof(shared_data_t), 0666)) == -1) {
        sleep(1);
    }
    
    return shmid;
}

/* Ottiene semaforo esistente */
int get_semaphore(void) {
    int semid;
    
    /* Attendi che P2 crei il semaforo */
    while ((semid = semget(SEM_KEY, 1, 0666)) == -1) {
        sleep(1);
    }
    
    return semid;
}

/* Funzione principale */
int main(void) {
    int shmid, semid;
    int client_fd;
    shared_data_t *shared_data;
    char string_to_send[MAX_STRING_LEN];
    
    printf("P3: Avvio processo P3...\n");
    
    /* Ottieni memoria condivisa e semaforo */
    shmid = get_shared_memory();
    semid = get_semaphore();
    
    /* Attach memoria condivisa */
    shared_data = (shared_data_t*)shmat(shmid, NULL, 0);
    if (shared_data == (void*)-1) {
        err_sys("shmat error");
    }
    
    /* Connetti a T2 (P1) */
    printf("P3: Connessione a T2...\n");
    client_fd = connect_to_tcp_server(LOOPBACK_IP, P3_TO_P1_PORT);
    
    printf("P3: Connesso a T2, inizio lettura memoria condivisa...\n");
    
    /* Loop principale */
    while (1) {
        /* Leggi da memoria condivisa */
        sem_wait_op(semid);
        
        if (shared_data->ready == 1) {
            /* Copia stringa */
            strcpy(string_to_send, shared_data->data);
            
            /* Segna come letto */
            shared_data->ready = 0;
            
            sem_signal_op(semid);
            
            /* Invia a T2 */
            send_string(client_fd, string_to_send);
            
        } else {
            sem_signal_op(semid);
            /* Breve pausa se non ci sono dati */
            usleep(1000);
        }
    }
    
    close(client_fd);
    return 0;
}