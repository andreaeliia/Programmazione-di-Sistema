/*
 * Client di test per il server concorrente
 * Invia richieste al server e riceve risposte
 */

#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#define SERVER_PORT 8080
#define BUFFER_SIZE 256

/*
 * Connette al server
 */
int connect_to_server(void) {
    int sock_fd;
    struct sockaddr_in server_addr;
    
    /* Crea socket */
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        err_sys("socket failed");
    }
    
    /* Configura indirizzo server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    
    /* Converte indirizzo IP */
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        err_sys("inet_pton failed");
    }
    
    /* Connette al server */
    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        err_sys("connect failed");
    }
    
    return sock_fd;
}

/*
 * Funzione principale del client
 */
int main(int argc, char *argv[]) {
    int sock_fd;
    char message[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    ssize_t bytes_sent, bytes_received;
    
    /* Messaggio da inviare */
    if (argc > 1) {
        strncpy(message, argv[1], BUFFER_SIZE - 1);
        message[BUFFER_SIZE - 1] = '\0';
    } else {
        strcpy(message, "Richiesta di test dal client");
    }
    
    printf("Client: connessione al server...\n");
    
    /* Connette al server */
    sock_fd = connect_to_server();
    
    printf("Client: connesso! Invio messaggio: %s\n", message);
    
    /* Invia messaggio */
    bytes_sent = write(sock_fd, message, strlen(message));
    if (bytes_sent < 0) {
        err_sys("write failed");
    }
    
    printf("Client: messaggio inviato (%ld bytes)\n", (long)bytes_sent);
    
    /* Riceve risposta */
    bytes_received = read(sock_fd, response, BUFFER_SIZE - 1);
    if (bytes_received < 0) {
        err_sys("read failed");
    }
    
    response[bytes_received] = '\0';
    printf("Client: risposta ricevuta: %s", response);
    
    /* Chiude connessione */
    close(sock_fd);
    printf("Client: connessione chiusa\n");
    
    return 0;
}