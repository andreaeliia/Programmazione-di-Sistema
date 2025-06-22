#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <time.h>
#include <stdlib.h>

#define PORT 8080
#define MAX_CLIENTS 10

int main(void) {
    int server_fd, client_sockets[MAX_CLIENTS];
    struct sockaddr_in address;
    fd_set readfds;
    int max_fd, activity, i, new_socket, addrlen;
    char random_char;
    
    // Inizializza array client socket
    for (i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;
    }
    
    // Crea socket del server
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        err_sys("socket failed");
    }
    
    // Configura indirizzo server
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // Bind socket all'indirizzo
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        err_sys("bind failed");
    }
    
    // Ascolta connessioni (max 3 in coda)
    if (listen(server_fd, 3) < 0) {
        err_sys("listen failed");
    }
    
    printf("Server in ascolto sulla porta %d\n", PORT);
    addrlen = sizeof(address);
    
    // Inizializza generatore numeri casuali
    srand(time(NULL));
    
    while (1) {
        // Prepara set di file descriptor per select
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_fd = server_fd;
        
        // Aggiungi client socket al set
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] > 0) {
                FD_SET(client_sockets[i], &readfds);
            }
            if (client_sockets[i] > max_fd) {
                max_fd = client_sockets[i];
            }
        }
        
        // Aspetta attività sui socket (timeout 1 secondo)
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        activity = select(max_fd + 1, &readfds, NULL, NULL, &timeout);
        
        if (activity < 0) {
            err_sys("select error");
        }
        
        // Nuova connessione sul server socket
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                err_sys("accept failed");
            }
            
            printf("Nuova connessione: socket fd %d, ip %s, porta %d\n",
                   new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
            
            // Aggiungi nuovo socket all'array
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    printf("Aggiunto alla lista socket index %d\n", i);
                    break;
                }
            }
        }
        
        // Controlla se qualche client ha inviato dati o si è disconnesso
        for (i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            
            if (FD_ISSET(sd, &readfds)) {
                char buffer[1];
                int valread = read(sd, buffer, 1);
                
                if (valread == 0) {
                    // Client disconnesso
                    printf("Client disconnesso, socket fd %d\n", sd);
                    close(sd);
                    client_sockets[i] = 0;
                } else {
                    // Client attivo - invia carattere casuale
                    random_char = 'A' + (rand() % 26);
                    send(sd, &random_char, 1, 0);
                    printf("Inviato '%c' al client fd %d\n", random_char, sd);
                }
            }
        }
    }
    
    close(server_fd);
    return 0;
}