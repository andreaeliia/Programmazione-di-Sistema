/*
 * client.c - Client per la ricerca di file JPEG e PNG
 * 
 * Il client si connette al server e invia richieste per la ricerca
 * di file JPEG o PNG nella home directory del server.
 */

#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define SERVER_PORT 8080
#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"

/*
 * Funzione per inviare una richiesta al server
 * sockfd: socket descriptor
 * request: stringa da inviare ("JPEG" o "PNG")
 */
int send_request(int sockfd, const char *request) {
    if (send(sockfd, request, strlen(request), 0) < 0) {
        err_sys("send error");
        return -1;
    }
    return 0;
}

/*
 * Funzione per ricevere e stampare i risultati dal server
 * sockfd: socket descriptor
 */
void receive_results(int sockfd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    
    printf("Risultati ricevuti dal server:\n");
    printf("===============================\n");
    
    /* Riceve i dati dal server fino alla chiusura della connessione */
    while ((bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
    }
    
    if (bytes_received < 0) {
        err_sys("recv error");
    }
    
    printf("\n===============================\n");
}

/*
 * Funzione per creare e configurare il socket client
 * Ritorna il file descriptor del socket
 */
int create_client_socket(void) {
    int sockfd;
    struct sockaddr_in server_addr;
    
    /* Crea il socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        err_sys("socket error");
    }
    
    /* Configura l'indirizzo del server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        err_sys("inet_pton error");
    }
    
    /* Connette al server */
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        err_sys("connect error");
    }
    
    return sockfd;
}

/*
 * Funzione principale del client
 */
int main(int argc, char *argv[]) {
    int sockfd;
    char request[10];
    
    printf("Client per ricerca file JPEG/PNG\n");
    printf("Digita 'JPEG' per cercare file JPEG\n");
    printf("Digita 'PNG' per cercare file PNG\n");
    printf("Digita 'quit' per uscire\n\n");
    
    while (1) {
        printf("Inserisci richiesta: ");
        if (fgets(request, sizeof(request), stdin) == NULL) {
            break;
        }
        
        /* Rimuove il newline dalla stringa */
        request[strcspn(request, "\n")] = '\0';
        
        /* Controlla se l'utente vuole uscire */
        if (strcmp(request, "quit") == 0) {
            break;
        }
        
        /* Verifica che la richiesta sia valida */
        if (strcmp(request, "JPEG") != 0 && strcmp(request, "PNG") != 0) {
            printf("Richiesta non valida. Usa 'JPEG' o 'PNG'\n");
            continue;
        }
        
        /* Crea connessione al server */
        sockfd = create_client_socket();
        
        /* Invia la richiesta */
        if (send_request(sockfd, request) == 0) {
            /* Riceve e stampa i risultati */
            receive_results(sockfd);
        }
        
        /* Chiude la connessione */
        close(sockfd);
    }
    
    printf("Client terminato.\n");
    return 0;
}