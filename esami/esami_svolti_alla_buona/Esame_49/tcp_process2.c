/* tcp_process2.c - Processo 2 per versione TCP */
#include "token_ring.h"

/* Funzione principale */
int main(void) {
    tcp_token_t token;
    struct timeval start_time, current_time;
    int server_fd, client_from_1, client_to_3;
    int tokens_processed = 0;
    double elapsed_time;
    struct sockaddr_in client_addr;
    socklen_t client_len;
    
    printf("Processo 2 (TCP): Avvio server porta %d...\n", TCP_BASE_PORT + 2);
    
    /* Crea server per ricevere da processo 1 */
    server_fd = create_tcp_server(TCP_BASE_PORT + 2);
    
    /* Connetti a processo 3 */
    printf("Processo 2 (TCP): Connessione a processo 3...\n");
    client_to_3 = connect_to_tcp_server(TCP_BASE_PORT + 3);
    
    /* Accetta connessione da processo 1 */
    printf("Processo 2 (TCP): Attendo connessione da processo 1...\n");
    client_len = sizeof(client_addr);
    client_from_1 = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_from_1 == -1) {
        err_sys("accept error");
    }
    
    printf("Processo 2 (TCP): Pronto per token ring...\n");
    
    /* Ricevi primo token per iniziare benchmark */
    receive_token_tcp(client_from_1, &token);
    gettimeofday(&start_time, NULL);
    
    do {
        token.counter++;
        tokens_processed++;
        
        /* Invia a processo 3 */
        send_token_tcp(client_to_3, &token);
        
        /* Riceve token successivo */
        if (receive_token_tcp(client_from_1, &token) != 0) {
            break;
        }
        
        gettimeofday(&current_time, NULL);
        elapsed_time = get_time_diff(&start_time, &current_time);
        
    } while (elapsed_time < TEST_DURATION);
    
    print_benchmark_results(2, tokens_processed, elapsed_time);
    
    close(client_from_1);
    close(client_to_3);
    close(server_fd);
    
    return 0;
}