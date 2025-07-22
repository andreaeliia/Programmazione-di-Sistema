#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int client_fd;
    struct sockaddr_in server_addr;
    char message[BUFFER_SIZE];
    char response[BUFFER_SIZE];

    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    
    // 1. Crea socket UDP
    client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_fd < 0) {
        perror("socket failed");
        exit(1);
    }
    
    // 2. Configura indirizzo server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    // Timeout per evitare blocchi
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    printf("Client UDP connesso al server %s:%d\n", SERVER_IP, SERVER_PORT);
    
    // 3. Loop di invio messaggi
    while (1) {
        printf("Inserisci un numero (quit per uscire): ");
        
        // Leggi input
        if (fgets(message, BUFFER_SIZE, stdin) == NULL) {
            printf("Errore lettura input\n");
            break;
        }
        
        // RIMUOVI NEWLINE SUBITO!
        message[strcspn(message, "\n")] = '\0';
        
        // Controlla quit
        if (strcmp(message, "quit") == 0) {
            break;
        }
        
        // Controlla input vuoto
        if (strlen(message) == 0) {
            printf("Input vuoto, riprova\n");
            continue;
        }
        
        // VALIDAZIONE NUMERO SERVER 
        char *endptr;
        errno = 0;  // Reset errno per rilevare overflow
        long long number = strtoll(message, &endptr, 10);
        
        if (*endptr != '\0') {
            printf("Errore: '%s' non è un numero valido\n", message);
            continue;
        }
        
        if (errno == ERANGE) {
            printf("Errore: numero troppo grande\n");
            continue;
        }
        
        if (number < 0) {
            printf("Errore: inserisci un numero positivo\n");
            continue;
        }
        
        printf("Numero valido: %lld\n", number);
        
        // Invia messaggio al server
        sendto(client_fd, message, strlen(message), 0,
               (struct sockaddr*)&server_addr, sizeof(server_addr));
        
        // Ricevi risposte dal server
        printf("File ricevuti:\n");
        int file_count = 0;
        
        while (1) {
            int bytes_received = recvfrom(client_fd, response, BUFFER_SIZE - 1, 0,
                                         NULL, NULL);
            
            if (bytes_received > 0) {
                response[bytes_received] = '\0';
                
                // Controlla se è il messaggio di fine
                if (strcmp(response, "end") == 0) {
                    printf("Fine lista (totale: %d file)\n\n", file_count);
                    break;
                }
                
                // Stampa il percorso file
                printf("  %d. %s\n", ++file_count, response);
                
            } else if (bytes_received == 0) {
                printf("Server ha chiuso la connessione\n");
                break;
            } else {
                // bytes_received < 0
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    printf("Timeout - nessuna risposta dal server\n");
                } else {
                    printf("Errore ricezione: %s\n", strerror(errno));
                }
                break;
            }
        }
    }
    
    close(client_fd);
    printf("Client terminato\n");
    return 0;
}