/* process_p1.c - Processo P1 con thread T1 e T2 */
#include "sync_comm.h"

/* Variabili globali per sincronizzazione thread */
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_cond = PTHREAD_COND_INITIALIZER;
static int g_can_send = 1; /* T1 può iniziare */

/* Stampa timestamp con precisione nanosecondi */
void print_nanosecond_time(void) {
    struct timespec ts;
    
    if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
        printf("Loop completed at: %ld.%09ld seconds\n", 
               (long)ts.tv_sec, ts.tv_nsec);
        fflush(stdout);
    } else {
        err_sys("clock_gettime error");
    }
}

/* Genera stringa casuale */
void generate_random_string(char *str, int length) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    int i;
    
    for (i = 0; i < length - 1; i++) {
        str[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    str[length - 1] = '\0';
}

/* Crea server TCP */
int create_tcp_server(int port) {
    int sockfd;
    struct sockaddr_in server_addr;
    int opt = 1;
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        err_sys("socket error");
    }
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        err_sys("setsockopt error");
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        err_sys("bind error");
    }
    
    if (listen(sockfd, BACKLOG) == -1) {
        err_sys("listen error");
    }
    
    return sockfd;
}

/* Connessione a server TCP */
int connect_to_tcp_server(const char *ip, int port) {
    int sockfd;
    struct sockaddr_in server_addr;
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        err_sys("socket error");
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        err_sys("inet_pton error");
    }
    
    /* Retry fino a connessione riuscita */
    while (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        sleep(1);
    }
    
    return sockfd;
}

/* Invia stringa via TCP */
void send_string(int sockfd, const char *str) {
    int len = strlen(str);
    
    /* Invia lunghezza stringa */
    if (write(sockfd, &len, sizeof(len)) != sizeof(len)) {
        err_sys("write length error");
    }
    
    /* Invia stringa */
    if (write(sockfd, str, len) != len) {
        err_sys("write string error");
    }
}

/* Riceve stringa via TCP */
int receive_string(int sockfd, char *str, int max_len) {
    int len;
    
    /* Ricevi lunghezza */
    if (read(sockfd, &len, sizeof(len)) != sizeof(len)) {
        return -1;
    }
    
    if (len >= max_len) {
        len = max_len - 1;
    }
    
    /* Ricevi stringa */
    if (read(sockfd, str, len) != len) {
        return -1;
    }
    
    str[len] = '\0';
    return len;
}

/* Thread T1 - genera e invia stringhe a P2 */
void* t1_function(void* arg) {
    t1_data_t *data = (t1_data_t*)arg;
    char random_str[MAX_STRING_LEN];
    
    while (1) {
        /* Attende il permesso di inviare */
        pthread_mutex_lock(data->mutex);
        while (!(*data->can_send)) {
            pthread_cond_wait(data->cond, data->mutex);
        }
        
        /* Genera stringa casuale */
        generate_random_string(random_str, 20);
        
        /* Invia a P2 */
        send_string(data->socket_fd, random_str);
        
        /* Segnala che ha inviato, ora T2 deve ricevere */
        *data->can_send = 0;
        pthread_mutex_unlock(data->mutex);
    }
    
    return NULL;
}

/* Thread T2 - riceve stringhe da P3 */
void* t2_function(void* arg) {
    t2_data_t *data = (t2_data_t*)arg;
    char received_str[MAX_STRING_LEN];
    
    while (1) {
        /* Ricevi stringa da P3 */
        if (receive_string(data->socket_fd, received_str, sizeof(received_str)) > 0) {
            /* Stampa timestamp completamento ciclo */
            print_nanosecond_time();
            
            /* Segnala che può iniziare nuovo ciclo */
            pthread_mutex_lock(data->mutex);
            *data->can_send = 1;
            pthread_cond_signal(data->cond);
            pthread_mutex_unlock(data->mutex);
        }
    }
    
    return NULL;
}

/* Funzione principale */
int main(void) {
    pthread_t t1_thread, t2_thread;
    t1_data_t t1_data;
    t2_data_t t2_data;
    int server_fd_for_t2, client_fd_from_p3;
    struct sockaddr_in client_addr;
    socklen_t client_len;
    
    /* Inizializza generatore numeri casuali */
    srand((unsigned int)time(NULL));
    
    printf("P1: Avvio processo con thread T1 e T2...\n");
    
    /* Connetti T1 a P2 */
    printf("P1: T1 connessione a P2...\n");
    t1_data.socket_fd = connect_to_tcp_server(LOOPBACK_IP, P1_TO_P2_PORT);
    t1_data.mutex = &g_mutex;
    t1_data.cond = &g_cond;
    t1_data.can_send = &g_can_send;
    
    /* Crea server per T2 (riceve da P3) */
    printf("P1: T2 creazione server...\n");
    server_fd_for_t2 = create_tcp_server(P3_TO_P1_PORT);
    
    /* Attendi connessione da P3 */
    printf("P1: T2 attesa connessione da P3...\n");
    client_len = sizeof(client_addr);
    client_fd_from_p3 = accept(server_fd_for_t2, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd_from_p3 == -1) {
        err_sys("accept error");
    }
    
    t2_data.socket_fd = client_fd_from_p3;
    t2_data.mutex = &g_mutex;
    t2_data.cond = &g_cond;
    t2_data.can_send = &g_can_send;
    
    /* Crea thread T1 e T2 */
    if (pthread_create(&t1_thread, NULL, t1_function, &t1_data) != 0) {
        err_sys("pthread_create T1 error");
    }
    
    if (pthread_create(&t2_thread, NULL, t2_function, &t2_data) != 0) {
        err_sys("pthread_create T2 error");
    }
    
    printf("P1: Sistema avviato, inizio cicli...\n");
    
    /* Attendi thread (loop infinito) */
    pthread_join(t1_thread, NULL);
    pthread_join(t2_thread, NULL);
    
    return 0;
}