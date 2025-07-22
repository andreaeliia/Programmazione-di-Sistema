/*
 * A.c - Programma Client per conteggio file/directory
 * 
 * Descrizione: Client UDP che chiede all'utente se contare file o directory,
 * invia la richiesta al server e visualizza il risultato.
 * 
 * Utilizzo: ./A
 * 
 * Argomenti traccia:
 * - Comunicazione client-server UDP
 * - Input utente per scelta tipo conteggio
 * - Invio datagramma UDP
 * - Ricezione e visualizzazione risultato
 */

#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SERVER_PORT 12345
#define BUFFER_SIZE 1024

/* Prototipi delle funzioni */
static int create_client_socket(void);
static int get_server_address(struct sockaddr_in *server_addr, const char *hostname);
static char get_user_choice(void);
static void send_request(int sockfd, const struct sockaddr_in *server_addr, char choice);
static void receive_response(int sockfd);

/*
 * Funzione principale del client
 */
int main(void)
{
    int sockfd;
    struct sockaddr_in server_addr;
    char choice;
    char hostname[256];
    
    printf("=== CLIENT CONTATORE FILE/DIRECTORY ===\n\n");
    
    /* Richiedi hostname del server all'utente */
    printf("Inserisci l'hostname o IP del server: ");
    if (fgets(hostname, sizeof(hostname), stdin) == NULL)
        err_sys("errore lettura hostname");
    
    /* Rimuovi newline dalla fine */
    hostname[strcspn(hostname, "\n")] = 0;
    
    /* Crea socket client */
    sockfd = create_client_socket();
    
    /* Ottieni indirizzo del server */
    if (get_server_address(&server_addr, hostname) < 0) {
        close(sockfd);
        exit(1);
    }
    
    /* Ottieni scelta dell'utente */
    choice = get_user_choice();
    
    /* Invia richiesta al server */
    send_request(sockfd, &server_addr, choice);
    
    /* Ricevi e mostra risposta */
    receive_response(sockfd);
    
    close(sockfd);
    printf("\nClient terminato.\n");
    
    return 0;
}

/*
 * Crea e configura il socket client UDP
 * Ritorna: file descriptor del socket
 */
static int create_client_socket(void)
{
    int sockfd;
    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        err_sys("errore socket()");
    
    printf("Socket client creato.\n");
    return sockfd;
}

/*
 * Risolve hostname e prepara indirizzo server
 * server_addr: struttura da riempire
 * hostname: nome o IP del server
 * Ritorna: 0 successo, -1 errore
 */
static int get_server_address(struct sockaddr_in *server_addr, const char *hostname)
{
    struct hostent *he;
    
    memset(server_addr, 0, sizeof(*server_addr));
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(SERVER_PORT);
    
    /* Prova prima come indirizzo IP diretto */
    if (inet_aton(hostname, &server_addr->sin_addr) == 1) {
        printf("Connessione a IP: %s:%d\n", hostname, SERVER_PORT);
        return 0;
    }
    
    /* Risolvi come hostname */
    if ((he = gethostbyname(hostname)) == NULL) {
        fprintf(stderr, "Errore risoluzione hostname: %s\n", hostname);
        return -1;
    }
    
    memcpy(&server_addr->sin_addr, he->h_addr_list[0], he->h_length);
    printf("Connessione a host: %s (%s):%d\n", 
           hostname, inet_ntoa(server_addr->sin_addr), SERVER_PORT);
    
    return 0;
}

/*
 * Chiede all'utente di scegliere tra file e directory
 * Ritorna: 'f' per file, 'd' per directory
 */
static char get_user_choice(void)
{
    char choice;
    char line[10];
    
    do {
        printf("\nVuoi contare file o directory?\n");
        printf("Digita 'f' per file, 'd' per directory: ");
        
        if (fgets(line, sizeof(line), stdin) == NULL)
            err_sys("errore lettura input");
        
        choice = tolower(line[0]);
        
        if (choice != 'f' && choice != 'd') {
            printf("Scelta non valida! Usa 'f' o 'd'.\n");
        }
        
    } while (choice != 'f' && choice != 'd');
    
    if (choice == 'f') {
        printf("Hai scelto di contare i FILE.\n");
    } else {
        printf("Hai scelto di contare le DIRECTORY.\n");
    }
    
    return choice;
}

/*
 * Invia la richiesta al server tramite UDP
 * sockfd: socket del client
 * server_addr: indirizzo del server
 * choice: 'f' o 'd'
 */
static void send_request(int sockfd, const struct sockaddr_in *server_addr, char choice)
{
    if (sendto(sockfd, &choice, 1, 0, 
               (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        err_sys("errore sendto()");
    }
    
    printf("Richiesta '%c' inviata al server...\n", choice);
}

/*
 * Riceve la risposta dal server e la visualizza
 * sockfd: socket del client
 */
static void receive_response(int sockfd)
{
    char buffer[BUFFER_SIZE];
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    ssize_t n;
    long count;
    
    printf("Attendo risposta dal server...\n");
    
    /* Ricevi risposta */
    if ((n = recvfrom(sockfd, buffer, sizeof(buffer)-1, 0,
                     (struct sockaddr *)&from_addr, &from_len)) < 0) {
        err_sys("errore recvfrom()");
    }
    
    buffer[n] = '\0';  /* Termina stringa */
    
    printf("Risposta ricevuta da %s:%d\n", 
           inet_ntoa(from_addr.sin_addr), ntohs(from_addr.sin_port));
    
    /* Converti risultato in numero */
    count = atol(buffer);
    
    if (count >= 0) {
        printf("\n=== RISULTATO ===\n");
        printf("Totale elementi contati: %ld\n", count);
    } else {
        printf("Errore nel conteggio ricevuto dal server.\n");
    }
}