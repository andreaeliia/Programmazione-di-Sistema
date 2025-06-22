#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8081
#define BUFFER_SIZE 1024

int main() {
    int sock_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    
    // 1. Crea socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket failed");
        exit(1);
    }
    
    // 2. Configura indirizzo server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton failed");
        exit(1);
    }
    
    // 3. Connetti
    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        exit(1);
    }
    
    printf("Connesso al server. Digita messaggi:\n");
    
    // 4. Loop comunicazione
    while (1) {
        printf("> ");
        if (!fgets(buffer, BUFFER_SIZE, stdin)) break;
        
        if (strncmp(buffer, "quit", 4) == 0) break;
        
        // Invia messaggio
        send(sock_fd, buffer, strlen(buffer), 0);
        
        // Ricevi risposta
        memset(response, 0, BUFFER_SIZE);
        ssize_t bytes = recv(sock_fd, response, BUFFER_SIZE-1, 0);
        if (bytes > 0) {
            printf("Echo: %s", response);
        }
         if (bytes < 0) {
            printf("Client disconnesso");
            break;
        }



    }
    
    close(sock_fd);
    return 0;
}