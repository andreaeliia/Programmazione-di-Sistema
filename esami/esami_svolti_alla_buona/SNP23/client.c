/*
 * Client TCP di Test per Server Multi-Thread
 * 
 * Client semplice per testare il server TCP multi-thread.
 * Si connette a una porta specificata, invia un intero e riceve risposta.
 *
 * Uso: ./tcp_client <hostname> <porta> <valore>
 * Esempio: ./tcp_client localhost 8001 42
 *
 * Compatibile Linux/macOS con libreria APUE
 */

#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/* Costanti */
#define RESPONSE_BUFFER_SIZE 256

/* Prototipi funzioni */
static int connect_to_server(const char *hostname, int port);
static int send_integer_to_server(int sockfd, int value);
static void print_usage(const char *progname);

/*
 * Stabilisce connessione TCP al server
 */
static int connect_to_server(const char *hostname, int port)
{
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *host_entry;
    
    /* Crea socket TCP */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        err_sys("socket creation failed");
    }
    
    /* Risolvi hostname */
    host_entry = gethostbyname(hostname);
    if (host_entry == NULL) {
        fprintf(stderr, "Failed to resolve hostname: %s\n", hostname);
        close(sockfd);
        return -1;
    }
    
    /* Configura indirizzo server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr, host_entry->h_addr_list[0], host_entry->h_length);
    
    /* Connetti al server */
    printf("Connecting to %s:%d...\n", hostname, port);
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        return -1;
    }
    
    printf("Connected successfully!\n");
    return sockfd;
}

/*
 * Invia intero al server e riceve risposta
 */
static int send_integer_to_server(int sockfd, int value)
{
    char send_buffer[32];
    char response_buffer[RESPONSE_BUFFER_SIZE];
    ssize_t bytes_sent, bytes_received;
    
    /* Prepara dati da inviare */
    snprintf(send_buffer, sizeof(send_buffer), "%d", value);
    
    /* Invia valore al server */
    printf("Sending value: %d\n", value);
    bytes_sent = send(sockfd, send_buffer, strlen(send_buffer), 0);
    if (bytes_sent < 0) {
        perror("send failed");
        return -1;
    }
    
    /* Ricevi risposta dal server */
    bytes_received = recv(sockfd, response_buffer, sizeof(response_buffer) - 1, 0);
    if (bytes_received < 0) {
        perror("recv failed");
        return -1;
    }
    
    if (bytes_received > 0) {
        response_buffer[bytes_received] = '\0';
        printf("Server response: %s", response_buffer);
    } else {
        printf("No response received from server\n");
    }
    
    return 0;
}

/*
 * Stampa istruzioni d'uso
 */
static void print_usage(const char *progname)
{
    printf("Uso: %s <hostname> <porta> <valore>\n", progname);
    printf("\nParametri:\n");
    printf("  hostname - Indirizzo server (es: localhost, 192.168.1.10)\n");
    printf("  porta    - Porta TCP del server (1024-65535)\n");
    printf("  valore   - Intero da inviare al server\n");
    printf("\nEsempi:\n");
    printf("  %s localhost 8001 42\n", progname);
    printf("  %s 127.0.0.1 8002 100\n", progname);
    printf("  %s server.domain.com 8003 -50\n", progname);
}

/*
 * Funzione principale
 */
int main(int argc, char *argv[])
{
    const char *hostname;
    int port;
    int value;
    int sockfd;
    
    printf("=== TCP Client Test ===\n");
    
    /* Verifica argomenti */
    if (argc != 4) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    
    /* Parse argomenti */
    hostname = argv[1];
    port = atoi(argv[2]);
    value = atoi(argv[3]);
    
    /* Validazione porta */
    if (port < 1 || port > 65535) {
        fprintf(stderr, "Errore: porta deve essere tra 1 e 65535\n");
        exit(EXIT_FAILURE);
    }
    
    printf("Target: %s:%d\n", hostname, port);
    printf("Value to send: %d\n\n", value);
    
    /* Connetti al server */
    sockfd = connect_to_server(hostname, port);
    if (sockfd < 0) {
        exit(EXIT_FAILURE);
    }
    
    /* Invia valore e ricevi risposta */
    if (send_integer_to_server(sockfd, value) < 0) {
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    /* Chiudi connessione */
    close(sockfd);
    printf("Connection closed.\n");
    
    return 0;
}