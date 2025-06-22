#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    
    // 1. Crea socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(1);
    }
    
    // 2. Riusa indirizzo
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // 3. Configura indirizzo
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // 4. Bind
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(1);
    }
    
    // 5. Listen
    if (listen(server_fd, 5) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(1);
    }
    
    printf("Server in ascolto sulla porta %d...\n", PORT);
    
    // 6. Accept e gestione client
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }
        
        printf("Client connesso: %s:%d\n", 
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));
        
        // Echo loop
        while (1) {
            memset(buffer, 0, BUFFER_SIZE);
            ssize_t bytes = recv(client_fd, buffer, BUFFER_SIZE-1, 0);
            
            if (bytes <= 0) {
                printf("Client disconnesso\n");
                break;
            }
            
            printf("Ricevuto: %s", buffer);
            char server_input[BUFFER_SIZE];
            if (fgets(server_input, sizeof(server_input), stdin)) {
            // Invia al client
            send(client_fd, server_input, strlen(server_input), 0);
          // Echo back
            }
        
        //close(client_fd);
    }
    close(client_fd);
    close(server_fd);
    return 0;
}
}