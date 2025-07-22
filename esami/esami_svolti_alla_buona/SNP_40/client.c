/*
 * client.c - Client per ricezione stream caratteri alfanumerici
 * Controllo velocità interattivo e salvataggio su file
 * Compatibile Linux/macOS con libreria APUE
 */

#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/time.h>

#define PORT 8080
#define OUTPUT_FILE "received_data.txt"
#define STATS_INTERVAL 2  /* secondi per aggiornamento statistiche */

/* Variabili globali */
static volatile sig_atomic_t running = 1;
static struct termios old_termios;
static int termios_set = 0;

/* Prototipi delle funzioni */
static void sig_handler(int signo);
static void setup_terminal(void);
static void restore_terminal(void);
static int connect_to_server(void);
static void process_server_data(int server_fd, FILE *output_file);
static void send_speed_command(int server_fd, const char *command);
static void display_help(void);

/*
 * Gestore per segnale SIGINT (Ctrl+C)
 * Permette terminazione pulita del client
 */
static void 
sig_handler(int signo)
{
    if (signo == SIGINT) {
        printf("\nClient terminato dall'utente\n");
        running = 0;
    }
}

/*
 * Configura il terminale per input non bloccante
 * Salva le impostazioni originali per il ripristino
 */
static void 
setup_terminal(void)
{
    struct termios new_termios;
    int flags;
    
    /* Salva impostazioni terminale correnti */
    if (tcgetattr(STDIN_FILENO, &old_termios) < 0) {
        err_sys("Errore tcgetattr");
    }
    termios_set = 1;
    
    /* Configura nuovo modo terminale */
    new_termios = old_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);  /* Disabilita mode canonico */
    new_termios.c_cc[VMIN] = 0;   /* Non bloccante */
    new_termios.c_cc[VTIME] = 0;  /* Timeout zero */
    
    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_termios) < 0) {
        err_sys("Errore tcsetattr");
    }
    
    /* Rende stdin non bloccante */
    flags = fcntl(STDIN_FILENO, F_GETFL);
    if (flags < 0) {
        err_sys("Errore fcntl F_GETFL");
    }
    
    if (fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) < 0) {
        err_sys("Errore fcntl F_SETFL");
    }
}

/*
 * Ripristina le impostazioni originali del terminale
 */
static void 
restore_terminal(void)
{
    if (termios_set) {
        if (tcsetattr(STDIN_FILENO, TCSANOW, &old_termios) < 0) {
            err_msg("Errore ripristino terminale");
        }
        termios_set = 0;
    }
}

/*
 * Stabilisce connessione TCP con il server
 * Restituisce il file descriptor del socket o -1 in caso di errore
 */
static int 
connect_to_server(void)
{
    int server_fd;
    struct sockaddr_in server_addr;
    
    /* Crea socket TCP */
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        err_sys("Errore creazione socket");
    }
    
    /* Configura indirizzo server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);
    
    /* Connessione al server */
    printf("Connessione al server localhost:%d...\n", PORT);
    if (connect(server_fd, (struct sockaddr*)&server_addr, 
                sizeof(server_addr)) < 0) {
        err_sys("Errore connessione al server");
    }
    
    printf("Connesso al server\n");
    return server_fd;
}

/*
 * Invia comando di controllo velocità al server
 */
static void 
send_speed_command(int server_fd, const char *command)
{
    if (write(server_fd, command, strlen(command)) < 0) {
        err_msg("Errore invio comando");
    }
}

/*
 * Mostra l'help per i comandi disponibili
 */
static void 
display_help(void)
{
    printf("\n=== COMANDI DISPONIBILI ===\n");
    printf("u - Aumenta velocità trasmissione\n");
    printf("d - Diminuisce velocità trasmissione\n");
    printf("h - Mostra questo help\n");
    printf("q - Termina il programma\n");
    printf("==========================\n\n");
}

/*
 * Processa i dati ricevuti dal server
 * Gestisce input da tastiera, statistiche e salvataggio file
 */
static void 
process_server_data(int server_fd, FILE *output_file)
{
    char buffer[1024];
    char key;
    ssize_t bytes_read;
    fd_set read_fds;
    struct timeval tv;
    int max_fd;
    
    /* Variabili per statistiche */
    time_t last_stats_time, current_time;
    int chars_received = 0;
    int total_chars = 0;
    int current_speed_reported = 0;
    
    last_stats_time = time(NULL);
    
    display_help();
    
    while (running) {
        /* Prepara set di file descriptor per select */
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(server_fd, &read_fds);
        max_fd = (server_fd > STDIN_FILENO) ? server_fd : STDIN_FILENO;
        
        /* Timeout per aggiornamenti periodici */
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        
        /* Attende attività su stdin o socket */
        if (select(max_fd + 1, &read_fds, NULL, NULL, &tv) < 0) {
            if (errno == EINTR) {
                continue;  /* Interruzione da segnale */
            }
            err_sys("Errore select");
        }
        
        /* Controlla input da tastiera */
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            if (read(STDIN_FILENO, &key, 1) > 0) {
                switch (key) {
                case 'u':
                case 'U':
                    send_speed_command(server_fd, "INCREASE\n");
                    printf("Comando inviato: aumenta velocità\n");
                    break;
                    
                case 'd':
                case 'D':
                    send_speed_command(server_fd, "DECREASE\n");
                    printf("Comando inviato: diminuisci velocità\n");
                    break;
                    
                case 'h':
                case 'H':
                    display_help();
                    break;
                    
                case 'q':
                case 'Q':
                    printf("Terminazione richiesta dall'utente\n");
                    running = 0;
                    break;
                    
                case '\n':
                case '\r':
                    /* Ignora invio */
                    break;
                    
                default:
                    printf("Comando non riconosciuto: '%c' "
                           "(premi 'h' per help)\n", key);
                    break;
                }
            }
        }
        
        /* Controlla dati dal server */
        if (FD_ISSET(server_fd, &read_fds)) {
            bytes_read = read(server_fd, buffer, sizeof(buffer) - 1);
            if (bytes_read <= 0) {
                printf("Server disconnesso\n");
                break;
            }
            
            buffer[bytes_read] = '\0';
            
            /* Cerca messaggi di velocità */
            char *speed_msg = strstr(buffer, "SPEED:");
            if (speed_msg) {
                sscanf(speed_msg, "SPEED:%d", &current_speed_reported);
                printf("Velocità server aggiornata: %d char/sec\n", 
                       current_speed_reported);
                continue;  /* Non salvare il messaggio di velocità */
            }
            
            /* Salva dati ricevuti su file */
            if (fwrite(buffer, 1, bytes_read, output_file) != bytes_read) {
                err_sys("Errore scrittura file");
            }
            fflush(output_file);  /* Forza scrittura su disco */
            
            /* Aggiorna contatori */
            chars_received += bytes_read;
            total_chars += bytes_read;
        }
        
        /* Mostra statistiche periodicamente */
        current_time = time(NULL);
        if (current_time - last_stats_time >= STATS_INTERVAL) {
            double rate = (double)chars_received / STATS_INTERVAL;
            printf("\r[Stats] Velocità ricezione: %.1f char/sec | "
                   "Totale ricevuti: %d | "
                   "Velocità server: %d char/sec",
                   rate, total_chars, current_speed_reported);
            fflush(stdout);
            
            /* Reset contatori per prossimo intervallo */
            chars_received = 0;
            last_stats_time = current_time;
        }
    }
}

/*
 * Funzione main - entry point del client
 */
int 
main(void)
{
    int server_fd;
    FILE *output_file;
    
    /* Installa gestore segnale */
    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        err_sys("Errore installazione gestore segnale");
    }
    
    /* Configura terminale per input non bloccante */
    setup_terminal();
    
    /* Apre file di output */
    if ((output_file = fopen(OUTPUT_FILE, "w")) == NULL) {
        restore_terminal();
        err_sys("Errore apertura file output");
    }
    
    printf("File di output: %s\n", OUTPUT_FILE);
    
    /* Connessione al server */
    server_fd = connect_to_server();
    
    /* Processa dati dal server */
    process_server_data(server_fd, output_file);
    
    /* Cleanup */
    printf("\nChiusura connessioni...\n");
    
    if (close(server_fd) < 0) {
        err_msg("Errore chiusura socket");
    }
    
    if (fclose(output_file) != 0) {
        err_msg("Errore chiusura file output");
    }
    
    restore_terminal();
    
    printf("Client terminato. Dati salvati in '%s'\n", OUTPUT_FILE);
    return 0;
}