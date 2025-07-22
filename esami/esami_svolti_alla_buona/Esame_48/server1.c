/* server1.c - Primo server TCP */
#include "shared_mem.h"

/* Variabili globali per cleanup */
static int g_shmid = -1;
static int g_semid = -1;

/* Gestore segnale per cleanup */
static void signal_handler(int sig) {
    cleanup_resources(g_shmid, g_semid);
    exit(0);
}

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
    
    /* Inizializza semaforo a 1 (mutex) */
    arg.val = 1;
    if (semctl(semid, 0, SETVAL, arg) == -1) {
        err_sys("semctl SETVAL error");
    }
    
    return semid;
}

/* Operazione wait sul semaforo */
void sem_wait(int semid) {
    struct sembuf sb;
    
    sb.sem_num = 0;
    sb.sem_op = -1;
    sb.sem_flg = 0;
    
    if (semop(semid, &sb, 1) == -1) {
        err_sys("semop wait error");
    }
}

/* Operazione signal sul semaforo */
void sem_signal(int semid) {
    struct sembuf sb;
    
    sb.sem_num = 0;
    sb.sem_op = 1;
    sb.sem_flg = 0;
    
    if (semop(semid, &sb, 1) == -1) {
        err_sys("semop signal error");
    }
}

/* Crea server TCP */
int create_tcp_server(int port) {
    int sockfd;
    struct sockaddr_in server_addr;
    int opt = 1;
    
    /* Crea socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        err_sys("socket error");
    }
    
    /* Imposta opzioni socket */
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        err_sys("setsockopt error");
    }
    
    /* Configura indirizzo server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    /* Bind */
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        err_sys("bind error");
    }
    
    /* Listen */
    if (listen(sockfd, 5) == -1) {
        err_sys("listen error");
    }
    
    return sockfd;
}

/* Gestisce connessione client */
void handle_client(int client_fd, shared_data_t *shared_data, int semid) {
    char buffer[MAX_STRING_LEN];
    ssize_t bytes_read;
    
    while ((bytes_read = read(client_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        
        /* Rimuovi newline finale se presente */
        if (buffer[bytes_read - 1] == '\n') {
            buffer[bytes_read - 1] = '\0';
        }
        
        /* Sezione critica: accesso memoria condivisa */
        sem_wait(semid);
        
        if (shared_data->count < MAX_STRINGS) {
            strcpy(shared_data->strings[shared_data->count], buffer);
            printf("Server1: Memorizzata stringa %d: %s\n", 
                   shared_data->count, buffer);
            shared_data->count++;
        } else {
            printf("Server1: Memoria condivisa piena!\n");
        }
        
        sem_signal(semid);
    }
    
    close(client_fd);
}

/* Cleanup risorse */
void cleanup_resources(int shmid, int semid) {
    if (shmid != -1) {
        shmctl(shmid, IPC_RMID, NULL);
    }
    if (semid != -1) {
        semctl(semid, 0, IPC_RMID);
    }
    printf("Server1: Risorse rilasciate\n");
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
    
    /* Crea memoria condivisa */
    shmid = create_shared_memory();
    g_shmid = shmid;
    
    /* Crea semaforo */
    semid = create_semaphore();
    g_semid = semid;
    
    /* Attach memoria condivisa */
    shared_data = (shared_data_t*)shmat(shmid, NULL, 0);
    if (shared_data == (void*)-1) {
        err_sys("shmat error");
    }
    
    /* Inizializza memoria condivisa (solo il primo server) */
    shared_data->count = 0;
    
    /* Crea server TCP */
    server_fd = create_tcp_server(PORT1);
    
    printf("Server1 in ascolto sulla porta %d\n", PORT1);
    
    /* Loop principale */
    while (1) {
        client_len = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd == -1) {
            err_sys("accept error");
        }
        
        printf("Server1: Connessione da %s:%d\n", 
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