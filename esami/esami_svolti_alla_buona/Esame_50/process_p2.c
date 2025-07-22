/* process_p2.c - Processo P2 */
#include "sync_comm.h"

/* Crea memoria condivisa */
int create_shared_memory(void) {
    int shmid;
    
    shmid = shmget(SHM_KEY, sizeof(shared_data_t), IPC_CREAT | 0666);
    if (shmid == -1) {
        err_sys("shmget error");
    }
    
    return shmid;
}

/* Crea semaforo */
int create_semaphore(void) {
    int semid;
    union semun {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
    } arg;
    
    semid = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (semid == -1) {
        err_sys("semget error");
    }
    
    /* Inizializza semaforo */
    arg.val = 1;
    if (semctl(semid, 0, SETVAL, arg) == -1) {
        err_sys("semctl error");
    }
    
    return semid;
}

/* Operazione wait su semaforo */
void sem_wait_op(int semid) {
    struct sembuf sb;
    
    sb.sem_num = 0;
    sb.sem_op = -1;
    sb.sem_flg = 0;
    
    if (semop(semid, &sb, 1) == -1) {
        err_sys("semop wait error");
    }
}

/* Operazione signal su semaforo */
void sem_signal_op(int semid) {
    struct sembuf sb;
    
    sb.sem_num = 0;
    sb.sem_op = 1;
    sb.sem_flg = 0;
    
    if (semop(semid, &sb, 1) == -1) {
        err_sys("semop signal error");
    }
}

/* Cleanup risorse IPC */
void cleanup_ipc_resources(int shmid, int semid) {
    if (shmid != -1) {
        shmctl(shmid, IPC_RMID, NULL);
    }
    if (semid != -1) {
        semctl(semid, 0, IPC_RMID);
    }
    printf("P2: Risorse IPC rilasciate\n");
}

/* Funzione principale */
int main(void) {
    int server_fd, client_fd;
    int shmid, semid;
    shared_data_t *shared_data;
    char received_str[MAX_STRING_LEN];
    struct sockaddr_in client_addr;
    socklen_t client_len;
    
    printf("P2: Avvio processo P2...\n");
    
    /* Crea memoria condivisa e semaforo */
    shmid = create_shared_memory();
    semid = create_semaphore();
    
    /* Attach memoria condivisa */
    shared_data = (shared_data_t*)shmat(shmid, NULL, 0);
    if (shared_data == (void*)-1) {
        err_sys("shmat error");
    }
    
    /* Inizializza memoria condivisa */
    shared_data->ready = 0;
    
    /* Crea server TCP per ricevere da P1 */
    server_fd = create_tcp_server(P1_TO_P2_PORT);
    printf("P2: Server TCP avviato, attesa connessioni da P1...\n");
    
    /* Accetta connessione da P1 */
    client_len = sizeof(client_addr);
    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd == -1) {
        err_sys("accept error");
    }
    
    printf("P2: Connesso a P1, inizio ricezione stringhe...\n");
    
    /* Loop principale */
    while (1) {
        /* Ricevi stringa da T1 */
        if (receive_string(client_fd, received_str, sizeof(received_str)) > 0) {
            
            /* Condividi con P3 tramite memoria condivisa */
            sem_wait_op(semid);
            
            /* Attendi che P3 abbia letto il messaggio precedente */
            while (shared_data->ready == 1) {
                sem_signal_op(semid);
                usleep(1000); /* Breve pausa */
                sem_wait_op(semid);
            }
            
            /* Scrivi nuova stringa */
            strcpy(shared_data->data, received_str);
            shared_data->ready = 1;
            
            sem_signal_op(semid);
        }
    }
    
    /* Cleanup */
    cleanup_ipc_resources(shmid, semid);
    close(client_fd);
    close(server_fd);
    
    return 0;
}