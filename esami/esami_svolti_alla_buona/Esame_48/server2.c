/* server2.c - Secondo server TCP */
#include "shared_mem.h"

/* Variabili globali per cleanup */
static int g_shmid = -1;
static int g_semid = -1;

/* Gestore segnale per cleanup */
static void signal_handler(int sig) {
    if (g_shmid != -1) {
        shmdt((void*)g_shmid);
    }
    exit(0);
}

/* Ottiene memoria condivisa esistente */
int get_shared_memory(void) {
    int shmid;
    
    shmid = shmget(SHM_KEY, sizeof(shared_data_t), 0666);
    if (shmid == -1) {
        err_sys("shmget error - memoria condivisa non trovata");
    }
    
    return shmid;
}

/* Ottiene semaforo esistente */
int get_semaphore(void) {
    int semid;
    
    semid = semget(SEM_KEY, 1, 0666);
    if (semid == -1) {
        err_sys("semget error - semaforo non trovato");
    }
    
    return semid;
}

/* Funzione principale */
int main(void) {
    int server_fd, client_fd;
    int shmid, semid;
    shared_data_t *shared_data;
    struct sockaddr_in client_addr;
    socklen_t client_len;
    pid_t pid;
    
    /* Installa gestore segnali */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* Ottiene memoria condivisa esistente */
    shmid = get_shared_memory();
    g_shmid = shmid;
    
    /* Ottiene semaforo esistente */
    semid = get_semaphore();
    g_semid = semid;
    
    /* Attach memoria condivisa */
    shared_data = (shared_data_t*)shmat(shmid, NULL, 0);
    if (shared_data == (void*)-1) {
        err_sys("shmat error");
    }
    
    /* Crea server TCP */
    server_fd = create_tcp_server(PORT2);
    
    printf("Server2 in ascolto sulla porta %d\n", PORT2);
    
    /* Loop principale */
    while (1) {
        client_len = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) {
            err_sys("accept error");
        }
        
        printf("Server2: Connessione da %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        /* Fork per gestire client */
        if ((pid = fork()) == 0) {
            /* Processo figlio */
            close(server_fd);
            handle_client(client_fd, shared_data, semid);
            exit(0);
        } else if (pid < 0) {
            err_sys("fork error");
        } else {
            /* Processo padre */
            close(client_fd);
        }
    }
    
    return 0;
}