/*
 * server.c - Server per invio stream continuo caratteri alfanumerici
 * Sistema client-server con controllo velocità trasmissione
 * Compatibile Linux/macOS con libreria APUE
 */

#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>

#define PORT 8080
#define INITIAL_SPEED 10  /* caratteri per secondo */
#define MIN_SPEED 1
#define MAX_SPEED 100
#define SPEED_INCREMENT 5

/* Variabili globali */
static int current_speed = INITIAL_SPEED;
static volatile sig_atomic_t running = 1;

/* Prototipi delle funzioni */
static void sig_handler(int signo);
static int setup_server_socket(void);
static char generate_random_char(void);
static void handle_client(int client_fd);
static void send_speed_info(int client_fd);

/*
 * Gestore per segnale SIGINT (Ctrl+C)
 * Permette terminazione pulita del server
 */
static void 
sig_handler(int signo)
{
    if (signo == SIGINT) {
        printf("\nServer terminato dall'utente\n");
        running = 0;
    }
}

/*
 * Configura e crea il socket del server
 * Restituisce il file descriptor del socket o -1 in caso di errore
 */
static int 
setup_server_socket(void)
{
    int server_fd;
    struct sockaddr_in server_addr;
    int opt = 1;

    /* Crea socket TCP */
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        err_sys("Errore creazione socket");
    }

    /* Permette riutilizzo dell'indirizzo */
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, 
                   &opt, sizeof(opt)) < 0) {
        err_sys("Errore setsockopt");
    }

    /* Configura indirizzo server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    /* Bind del socket */
    if (bind(server_fd, (struct sockaddr*)&server_addr, 
             sizeof(server_addr)) < 0) {
        err_sys("Errore bind");
    }

    /* Mette il socket in ascolto */
    if (listen(server_fd, 1) < 0) {
        err_sys("Errore listen");
    }

    printf("Server in ascolto su localhost:%d\n", PORT);
    printf("Velocità iniziale: %d caratteri/secondo\n", current_speed);
    
    return server_fd;
}

/*
 * Genera un carattere alfanumerico casuale
 * Restituisce un carattere tra [A-Z, a-z, 0-9]
 */
static char 
generate_random_char(void)
{
    static const char charset[] = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    return charset[rand() % (sizeof(charset) - 1)];
}

/*
 * Invia informazioni sulla velocità corrente al client
 */
static void 
send_speed_info(int client_fd)
{
    char speed_msg[64];
    
    snprintf(speed_msg, sizeof(speed_msg), "SPEED:%d\n", current_speed);
    if (write(client_fd, speed_msg, strlen(speed_msg)) < 0) {
        if (errno != EPIPE && errno != ECONNRESET) {
            err_msg("Errore invio velocità");
        }
    }
}

/*
 * Gestisce la comunicazione con un client connesso
 * Invia stream di caratteri e gestisce comandi di controllo velocità
 */
static void 
handle_client(int client_fd)
{
    char buffer[256];
    char data_char;
    struct timeval tv;
    fd_set read_fds;
    int delay_usec;
    ssize_t n;
    time_t start_time, current_time;
    int chars_sent = 0;

    printf("Client connesso\n");
    
    /* Invia velocità iniziale */
    send_speed_info(client_fd);
    
    /* Inizializza generatore numeri casuali */
    srand((unsigned int)time(NULL));
    
    /* Registra tempo inizio per statistiche */
    start_time = time(NULL);
    
    while (running) {
        /* Calcola delay tra caratteri basato sulla velocità */
        delay_usec = 1000000 / current_speed;  /* microsecondi */
        
        /* Configura timeout per select */
        tv.tv_sec = 0;
        tv.tv_usec = delay_usec;
        
        /* Prepara set per select */
        FD_ZERO(&read_fds);
        FD_SET(client_fd, &read_fds);
        
        /* Controlla se ci sono comandi dal client */
        if (select(client_fd + 1, &read_fds, NULL, NULL, &tv) > 0) {
            if (FD_ISSET(client_fd, &read_fds)) {
                n = read(client_fd, buffer, sizeof(buffer) - 1);
                if (n <= 0) {
                    /* Client disconnesso */
                    printf("Client disconnesso\n");
                    break;
                }
                
                buffer[n] = '\0';
                
                /* Gestisce comandi velocità */
                if (strstr(buffer, "INCREASE")) {
                    if (current_speed < MAX_SPEED) {
                        current_speed += SPEED_INCREMENT;
                        if (current_speed > MAX_SPEED) {
                            current_speed = MAX_SPEED;
                        }
                        printf("Velocità aumentata a: %d char/sec\n", 
                               current_speed);
                        send_speed_info(client_fd);
                    }
                } else if (strstr(buffer, "DECREASE")) {
                    if (current_speed > MIN_SPEED) {
                        current_speed -= SPEED_INCREMENT;
                        if (current_speed < MIN_SPEED) {
                            current_speed = MIN_SPEED;
                        }
                        printf("Velocità diminuita a: %d char/sec\n", 
                               current_speed);
                        send_speed_info(client_fd);
                    }
                }
            }
        }
        
        /* Genera e invia carattere */
        data_char = generate_random_char();
        if (write(client_fd, &data_char, 1) < 0) {
            if (errno == EPIPE || errno == ECONNRESET) {
                printf("Client disconnesso\n");
                break;
            }
            err_msg("Errore invio carattere");
        }
        
        chars_sent++;
        
        /* Mostra statistiche ogni 5 secondi */
        current_time = time(NULL);
        if (current_time - start_time >= 5) {
            printf("Statistiche: %d caratteri inviati, "
                   "velocità corrente: %d char/sec\n", 
                   chars_sent, current_speed);
            start_time = current_time;
            chars_sent = 0;
        }
    }
}

/*
 * Funzione main - entry point del server
 */
int 
main(void)
{
    int server_fd, client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    /* Installa gestore segnale */
    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        err_sys("Errore installazione gestore segnale");
    }
    
    /* Configura socket server */
    server_fd = setup_server_socket();
    
    /* Loop principale - accetta connessioni */
    while (running) {
        printf("In attesa di connessioni...\n");
        
        /* Accetta connessione client */
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, 
                          &client_len);
        if (client_fd < 0) {
            if (errno == EINTR && !running) {
                break;  /* Interruzione da segnale */
            }
            err_sys("Errore accept");
        }
        
        /* Gestisce client */
        handle_client(client_fd);
        
        /* Chiude connessione client */
        if (close(client_fd) < 0) {
            err_msg("Errore chiusura socket client");
        }
    }
    
    /* Cleanup */
    if (close(server_fd) < 0) {
        err_msg("Errore chiusura socket server");
    }
    
    printf("Server terminato\n");
    return 0;
}