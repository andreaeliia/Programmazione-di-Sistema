/*
 * Client per feedback anonimo studenti durante lezione
 * Invia messaggi multicast con pressione singolo tasto
 * Standard C90 compatibile
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#define MULTICAST_PORT 12345
#define MESSAGE_SIZE 2

/* Variabili globali per cleanup */
int sockfd = -1;
struct termios orig_termios;
int terminal_modified = 0;

/* Struttura per messaggio feedback */
typedef struct {
    char type;      /* A, R, I, N, C, B */
    char padding;   /* Per allineamento */
} FeedbackMessage;

/* Ripristina impostazioni terminale */
void restore_terminal() {
    if (terminal_modified) {
        tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
        terminal_modified = 0;
    }
}

/* Gestore segnali per cleanup */
void signal_handler(int sig) {
    printf("\n\nClient terminato dall'utente\n");
    restore_terminal();
    if (sockfd != -1) {
        close(sockfd);
    }
    exit(0);
}

/* Configura terminale per input raw (senza invio) */
int setup_raw_terminal() {
    struct termios new_termios;
    
    /* Salva impostazioni originali */
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        perror("tcgetattr");
        return -1;
    }
    
    /* Configura nuove impostazioni */
    new_termios = orig_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);  /* Disabilita canonical mode e echo */
    new_termios.c_cc[VMIN] = 1;               /* Leggi almeno 1 carattere */
    new_termios.c_cc[VTIME] = 0;              /* No timeout */
    
    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_termios) == -1) {
        perror("tcsetattr");
        return -1;
    }
    
    terminal_modified = 1;
    return 0;
}

/* Verifica se il carattere è un comando valido */
int is_valid_feedback(char c) {
    return (c == 'A' || c == 'a' ||
            c == 'R' || c == 'r' ||
            c == 'I' || c == 'i' ||
            c == 'N' || c == 'n' ||
            c == 'C' || c == 'c' ||
            c == 'B' || c == 'b');
}

/* Converte carattere in maiuscolo */
char to_upper(char c) {
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 'A';
    }
    return c;
}

/* Ottiene descrizione del comando */
const char* get_feedback_description(char c) {
    switch (c) {
        case 'A': return "Applauso";
        case 'R': return "Ripetere";
        case 'I': return "Incomprensibile";
        case 'N': return "Troppo noioso";
        case 'C': return "Continuare così";
        case 'B': return "Interrompere, inutile continuare";
        default: return "Sconosciuto";
    }
}

/* Invia messaggio di feedback */
int send_feedback(int sock, struct sockaddr_in *addr, char feedback_type) {
    FeedbackMessage msg;
    int bytes_sent;
    
    msg.type = to_upper(feedback_type);
    msg.padding = 0;
    
    bytes_sent = sendto(sock, &msg, sizeof(msg), 0, 
                       (struct sockaddr*)addr, sizeof(*addr));
    
    if (bytes_sent == -1) {
        perror("sendto");
        return -1;
    }
    
    return 0;
}

/* Mostra menu comandi */
void show_menu() {
    printf("\n=== CLIENT FEEDBACK STUDENTI ===\n");
    printf("Premi uno dei seguenti tasti per inviare feedback:\n\n");
    printf("  A = Applauso\n");
    printf("  R = Ripetere\n");
    printf("  I = Incomprensibile\n");
    printf("  N = Troppo noioso\n");
    printf("  C = Continuare così\n");
    printf("  B = Interrompere, inutile continuare\n");
    printf("  Q = Quit (termina client)\n");
    printf("\nI messaggi sono inviati immediatamente senza premere Invio\n");
    printf("Pronto per ricevere input...\n\n");
}

/* Funzione principale */
int main(int argc, char *argv[]) {
    struct sockaddr_in multicast_addr;
    char input_char;
    int feedback_count = 0;
    
    /* Verifica argomenti */
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <indirizzo_multicast>\n", argv[0]);
        fprintf(stderr, "Esempio: %s 224.1.1.1\n", argv[0]);
        return 1;
    }
    
    /* Installa gestori segnali */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* Crea socket UDP */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return 1;
    }
    
    /* Configura indirizzo multicast */
    memset(&multicast_addr, 0, sizeof(multicast_addr));
    multicast_addr.sin_family = AF_INET;
    multicast_addr.sin_port = htons(MULTICAST_PORT);
    
    if (inet_aton(argv[1], &multicast_addr.sin_addr) == 0) {
        fprintf(stderr, "Errore: Indirizzo multicast non valido: %s\n", argv[1]);
        close(sockfd);
        return 1;
    }
    
    /* Verifica che sia un indirizzo multicast valido (224.0.0.0 - 239.255.255.255) */
    if ((ntohl(multicast_addr.sin_addr.s_addr) >> 28) != 14) {
        fprintf(stderr, "Errore: %s non è un indirizzo multicast valido\n", argv[1]);
        fprintf(stderr, "Gli indirizzi multicast devono essere nel range 224.0.0.0 - 239.255.255.255\n");
        close(sockfd);
        return 1;
    }
    
    printf("Client avviato - Indirizzo multicast: %s:%d\n", argv[1], MULTICAST_PORT);
    
    /* Configura terminale per input raw */
    if (setup_raw_terminal() == -1) {
        fprintf(stderr, "Errore configurazione terminale\n");
        close(sockfd);
        return 1;
    }
    
    /* Mostra menu */
    show_menu();
    
    /* Loop principale per lettura input */
    while (1) {
        /* Legge un carattere */
        if (read(STDIN_FILENO, &input_char, 1) == 1) {
            
            /* Verifica se è comando di uscita */
            if (input_char == 'q' || input_char == 'Q') {
                printf("\nTerminazione richiesta dall'utente\n");
                break;
            }
            
            /* Verifica se è un feedback valido */
            if (is_valid_feedback(input_char)) {
                char upper_char = to_upper(input_char);
                
                /* Invia feedback */
                if (send_feedback(sockfd, &multicast_addr, upper_char) == 0) {
                    feedback_count++;
                    printf("Inviato: [%c] %s (totale inviati: %d)\n", 
                           upper_char, get_feedback_description(upper_char), feedback_count);
                } else {
                    printf("Errore invio feedback\n");
                }
            } else {
                /* Carattere non riconosciuto */
                printf("Tasto non riconosciuto: '%c' (ASCII: %d)\n", 
                       input_char, (int)input_char);
                printf("Usa: A, R, I, N, C, B oppure Q per uscire\n");
            }
        } else {
            /* Errore lettura */
            if (errno == EINTR) {
                continue; /* Interrotto da segnale, continua */
            }
            perror("read");
            break;
        }
    }
    
    /* Cleanup */
    restore_terminal();
    close(sockfd);
    
    printf("\nClient terminato - Feedback inviati: %d\n", feedback_count);
    return 0;
}