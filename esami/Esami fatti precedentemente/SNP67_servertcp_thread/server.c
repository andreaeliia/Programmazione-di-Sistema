

/*

Un processo client ha due thread che cambiano di continuo a intervalli
di tempo casuali il valore di una variabile d'ambiente A, assegnandole
un numero intero casuale. 

Gestire la concorrenza delle operazioni svolte dalle thread e fare in
modo che un'altra thread comunichi via TCP a un server ogni valore
modificato della variabile d'ambiente A. Il server, a sua volta, dovrà
provedere ad aggiornare ad ogni ricezione il valore della sua variabile
d'ambiente A.

I due processi devono procedere fino a quando il client non sia
interrotto. Non appena ciò accade, anche il server deve essere
interrotto.


*/




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Macro per pulizia stringhe
#define REMOVE_NEWLINE(str) do { \
    (str)[strcspn((str), "\n\r")] = '\0'; \
} while(0)

// Verifica se una stringa contiene solo cifre
int is_digit_string(const char* str) {
    if (str == NULL || *str == '\0') return 0;
    
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) return 0;
    }
    return 1;
}

int safe_string_to_int(const char* str, int* result) {
    if (str == NULL || result == NULL) return -1;
    
    errno = 0;
    char* endptr;
    long val = strtol(str, &endptr, 10);
    
    if (errno == ERANGE) {
        printf("Errore: numero fuori range\n");
        return -1;
    }
    
    if (endptr == str || *endptr != '\0') {
        printf("Errore: formato numero non valido\n");
        return -1;
    }
    
    if (val < INT_MIN || val > INT_MAX) {
        printf("Errore: numero troppo grande\n");
        return -1;
    }
    
    *result = (int)val;
    return 0;
}
 
int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    int opt = 1;

    int A = 0;   // Variabile da modificare
    
    // Creazione socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Riuso indirizzo
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Configurazione indirizzo
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // Bind e listen
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, 5) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Server avviato su porta %d\n", PORT);
    
    // Main loop - un client alla volta
    while (1) {
        printf("Aspettando nuova connessione...\n");
        
        // Blocca fino a nuova connessione
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }
        
        printf("Client connesso: %s:%d [fd=%d]\n", 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port), client_fd);
        
        // Messaggio benvenuto
        send(client_fd, "Server Sequenziale - Connesso\n", 30, 0);
        
        // Gestione client (fino a disconnessione)
        while (1) {
            int bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
            
            if (bytes_read <= 0) {
                if (bytes_read == 0) {
                    printf("Client [%d] disconnesso\n", client_fd);
                    close(client_fd);
                    printf("Connessione chiusa\n\n");
                    close(server_fd);
                    return EXIT_SUCCESS;
                } else {
                    perror("recv error");
                }
                break;
            }

            buffer[bytes_read] = '\0';

            printf("Ricevuto: '%s'\n", buffer);
            REMOVE_NEWLINE(buffer);

            if(safe_string_to_int(buffer, &A) != 0){
                printf("Errore nel casting\n");
                continue;
            }

            printf("Variabile A aggiornata: %d\n", A);
            
            // Aggiorna variabile di ambiente
            char A_string[20];
            sprintf(A_string, "%d", A);
            setenv("A", A_string, 1);
            
            // Echo al client
            snprintf(buffer, BUFFER_SIZE, "%d", A);
            send(client_fd, buffer, strlen(buffer), 0);
        }
    }

    return EXIT_SUCCESS;
}