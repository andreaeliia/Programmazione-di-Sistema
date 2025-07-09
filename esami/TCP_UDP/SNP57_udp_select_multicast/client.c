/*
 * Client per sistema di feedback studenti
 * Uso: ./client <indirizzo_multicast> <porta>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <termios.h>
#include <time.h>

#define BUFFER_SIZE 256

/* Struttura per il messaggio */
typedef struct {
    char tipo;
    time_t timestamp;
} messaggio_t;

/* Funzione per impostare il terminale in modalità raw */
void set_raw_mode(struct termios *orig_termios) {
    struct termios raw;
    
    tcgetattr(STDIN_FILENO, orig_termios);
    raw = *orig_termios;
    
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

/* Funzione per ripristinare il terminale */
void restore_terminal(struct termios *orig_termios) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, orig_termios);
}

/* Funzione per stampare le istruzioni */
void stampa_istruzioni() {
    printf("=== CLIENT FEEDBACK STUDENTI ===\n");
    printf("Premi i seguenti tasti per inviare feedback:\n");
    printf("A = Applauso\n");
    printf("R = Ripetere\n");
    printf("I = Incomprensibile\n");
    printf("N = Troppo noioso\n");
    printf("C = Continuare così\n");
    printf("B = Interrompere, inutile continuare\n");
    printf("Q = Quit (esci dal programma)\n");
    printf("\nInizio monitoraggio...\n\n");
}

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in server_addr;
    messaggio_t msg;
    char input;
    struct termios orig_termios;
    
    /* Controllo argomenti */
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <indirizzo_multicast> <porta>\n", argv[0]);
        exit(1);
    }
    
    /* Creazione socket UDP */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Errore nella creazione del socket");
        exit(1);
    }
    
    /* Configurazione indirizzo server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    
    /* Verifica indirizzo multicast */
    if (!IN_MULTICAST(ntohl(server_addr.sin_addr.s_addr))) {
        fprintf(stderr, "Errore: %s non è un indirizzo multicast valido\n", argv[1]);
        close(sockfd);
        exit(1);
    }
    
    /* Impostazione modalità raw per il terminale */
    set_raw_mode(&orig_termios);
    
    stampa_istruzioni();
    
    /* Loop principale */
    while (1) {
        /* Lettura carattere */
        if (read(STDIN_FILENO, &input, 1) == 1) {
            /* Conversione a maiuscolo */
            if (input >= 'a' && input <= 'z') {
                input = input - 'a' + 'A';
            }
            
            /* Controllo caratteri validi */
            if (input == 'A' || input == 'R' || input == 'I' || 
                input == 'N' || input == 'C' || input == 'B') {
                
                /* Preparazione messaggio */
                msg.tipo = input;
                msg.timestamp = time(NULL);
                
                /* Invio messaggio */
                if (sendto(sockfd, &msg, sizeof(msg), 0, 
                          (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                    perror("Errore nell'invio del messaggio");
                } else {
                    printf("Inviato: %c\n", input);
                }
            }
            else if (input == 'Q') {
                printf("\nUscita dal programma...\n");
                break;
            }
            else if (input == '\n' || input == '\r') {
                /* Ignora invio */
                continue;
            }
            else {
                printf("Carattere non valido: %c\n", input);
            }
        }
    }
    
    /* Cleanup */
    restore_terminal(&orig_termios);
    close(sockfd);
    
    return 0;
}