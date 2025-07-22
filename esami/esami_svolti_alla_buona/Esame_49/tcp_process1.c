/* tcp_process1.c - Processo 1 per versione TCP */
#include "token_ring.h"

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
int connect_to_tcp_server(int port) {
    int sockfd;
    struct sockaddr_in server_addr;
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        err_sys("socket error");
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        err_sys("inet_pton error");
    }
    
    /* Retry connessione */
    while (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        sleep(1);
    }
    
    return sockfd;
}

/* Invia token TCP */
void send_token_tcp(int sockfd, tcp_token_t *token) {
    if (write(sockfd, token, sizeof(tcp_token_t)) != sizeof(tcp_token_t)) {
        err_sys("write error");
    }
}

/* Riceve token TCP */
int receive_token_tcp(int sockfd, tcp_token_t *token) {
    ssize_t bytes_read = read(sockfd, token, sizeof(tcp_token_t));
    if (bytes_read != sizeof(tcp_token_t)) {
        return -1;
    }
    return 0;
}

/* Funzione principale */
int main(void) {
    tcp_token_t token;
    struct timeval start_time, current_time;
    int server_fd, client_from_3, client_to_2;
    int tokens_processed = 0;
    double elapsed_time;
    struct sockaddr_in client_addr;
    socklen_t client_len;
    
    printf("Processo 1 (TCP): Avvio server porta %d...\n", TCP_BASE_PORT + 1);
    
    /* Crea server per ricevere da processo 3 */
    server_fd = create_tcp_server(TCP_BASE_PORT + 1);
    
    /* Connetti a processo 2 */
    printf("Processo 1 (TCP): Connessione a processo 2...\n");
    client_to_2 = connect_to_tcp_server(TCP_BASE_PORT + 2);
    
    /* Accetta connessione da processo 3 */
    printf("Processo 1 (TCP): Attendo connessione da processo 3...\n");
    client_len = sizeof(client_addr);
    client_from_3 = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_from_3 == -1) {
        err_sys("accept error");
    }
    
    /* Inizializza token */
    memset(&token, 0, sizeof(token));
    token.counter = 0;
    token.process_id = 1;
    strcpy(token.data, "TOKEN_DATA_PAYLOAD_FOR_BENCHMARK");
    
    printf("Processo 1 (TCP): Avvio token ring per %d secondi...\n", TEST_DURATION);
    
    /* Invia token iniziale */
    send_token_tcp(client_to_2, &token);
    tokens_processed++;
    
    gettimeofday(&start_time, NULL);
    
    /* Loop principale */
    do {
        /* Riceve token da processo 3 */
        if (receive_token_tcp(client_from_3, &token) == 0) {
            token.counter++;
            tokens_processed++;
            
            /* Invia a processo 2 */
            send_token_tcp(client_to_2, &token);
        }
        
        gettimeofday(&current_time, NULL);
        elapsed_time = get_time_diff(&start_time, &current_time);
        
    } while (elapsed_time < TEST_DURATION);
    
    print_benchmark_results(1, tokens_processed, elapsed_time);
    
    close(client_from_3);
    close(client_to_2);
    close(server_fd);
    
    return 0;
}