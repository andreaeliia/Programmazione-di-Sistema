#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int client_fd;
    struct sockaddr_in server_addr;
    char message[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    
    /* 1. Crea socket UDP */
    client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_fd < 0) {
        perror("socket failed");
        exit(1);
    }
    
    /* 2. Configura indirizzo server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
    
    printf("Client UDP connesso al server %s:%d\n", SERVER_IP, SERVER_PORT);
    
    /* 3. Loop di invio messaggi */
    while (1) {
        printf("Inserisci un numero (quit per uscire): ");
        fgets(message, BUFFER_SIZE, stdin);
        
        /* Rimuovi newline */
        message[strcspn(message, "\n")] = 0;
        
        if (strcmp(message, "quit") == 0) {
            break;
        }
        
        /* Invia messaggio */
        sendto(client_fd, message, strlen(message), 0,
               (struct sockaddr*)&server_addr, sizeof(server_addr));
        
        /* Ricevi risposta */
        int bytes_received = recvfrom(client_fd, response, BUFFER_SIZE - 1, 0,
                                     NULL, NULL);
        
        if (bytes_received > 0) {
            response[bytes_received] = '\0';
            printf("Risposta server: %s\n", response);
        }
    }
    
    close(client_fd);
    return 0;
}