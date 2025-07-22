/*
 * Receiver - Programma per ricezione multicast e visualizzazione
 * Riceve messaggi multicast con posizioni del segnaposto
 * e mostra graficamente la matrice aggiornata
 */

#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>

#define MATRIX_SIZE 20
#define MULTICAST_ADDR "239.255.255.250"
#define MULTICAST_PORT 12345
#define BUFFER_SIZE 256

/* Struttura per posizione ricevuta */
struct position {
    int x;
    int y;
    time_t timestamp;
    int valid;
};

/* Variabili globali */
static struct position current_pos = {-1, -1, 0, 0}; /* Inizialmente invalida */
static int sockfd;

/* Prototipi delle funzioni */
static void setup_multicast_receiver(void);
static void display_matrix(void);
static int parse_position(const char *buffer);
static void clear_screen(void);
static void move_cursor(int row, int col);
static void cleanup_and_exit(int sig);

/*
 * Funzione principale del receiver
 */
int main(void)
{
    char buffer[BUFFER_SIZE];
    fd_set readfds;
    struct timeval tv;
    int retval;
    ssize_t bytes_received;
    time_t last_update = 0;
    
    printf("=== Multicast Matrix Receiver ===\n");
    printf("Initializing...\n");
    
    /* Configura gestione segnali */
    signal(SIGINT, cleanup_and_exit);
    signal(SIGTERM, cleanup_and_exit);
    
    /* Inizializza socket multicast */
    setup_multicast_receiver();
    
    /* Mostra interfaccia iniziale */
    clear_screen();
    printf("Waiting for multicast messages...\n");
    printf("Multicast group: %s:%d\n\n", MULTICAST_ADDR, MULTICAST_PORT);
    
    display_matrix();
    
    /* Loop principale di ricezione */
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        
        /* Timeout per refresh periodico */
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        
        retval = select(sockfd + 1, &readfds, NULL, NULL, &tv);
        
        if (retval == -1) {
            if (errno == EINTR) continue;
            err_sys("select error");
        } else if (retval > 0) {
            /* Messaggio ricevuto */
            if (FD_ISSET(sockfd, &readfds)) {
                bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
                if (bytes_received > 0) {
                    buffer[bytes_received] = '\0';
                    
                    /* Parsa e aggiorna posizione */
                    if (parse_position(buffer)) {
                        display_matrix();
                        last_update = time(NULL);
                        
                        move_cursor(MATRIX_SIZE + 4, 1);
                        printf("Last update: %s", ctime(&last_update));
                        move_cursor(MATRIX_SIZE + 5, 1);
                        printf("Position: (%d,%d)    ", 
                               current_pos.x, current_pos.y);
                        fflush(stdout);
                    }
                }
            }
        } else {
            /* Timeout - mostra stato */
            move_cursor(MATRIX_SIZE + 6, 1);
            if (current_pos.valid) {
                printf("No updates received (waiting...)    ");
            } else {
                printf("Waiting for first position...       ");
            }
            fflush(stdout);
        }
    }
    
    return 0;
}

/*
 * Configura socket multicast per ricezione
 */
static void setup_multicast_receiver(void)
{
    struct sockaddr_in addr;
    struct ip_mreq mreq;
    int opt = 1;
    
    /* Crea socket UDP */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        err_sys("socket error");
    }
    
    /* Abilita riuso indirizzo */
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
                   &opt, sizeof(opt)) < 0) {
        err_sys("setsockopt SO_REUSEADDR error");
    }
    
    /* Configura indirizzo locale */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(MULTICAST_PORT);
    
    /* Bind del socket */
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        err_sys("bind error");
    }
    
    /* Join al gruppo multicast */
    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_ADDR);
    mreq.imr_interface.s_addr = INADDR_ANY;
    
    if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, 
                   &mreq, sizeof(mreq)) < 0) {
        err_sys("setsockopt IP_ADD_MEMBERSHIP error");
    }
    
    printf("Joined multicast group: %s:%d\n", 
           MULTICAST_ADDR, MULTICAST_PORT);
}

/*
 * Mostra la matrice con il segnaposto
 */
static void display_matrix(void)
{
    int i, j;
    
    move_cursor(4, 1); /* Posiziona dopo header */
    
    /* Bordo superiore */
    printf("+");
    for (j = 0; j < MATRIX_SIZE; j++) printf("-");
    printf("+\n");
    
    /* Righe della matrice */
    for (i = 0; i < MATRIX_SIZE; i++) {
        printf("|");
        for (j = 0; j < MATRIX_SIZE; j++) {
            if (current_pos.valid && 
                i == current_pos.y && j == current_pos.x) {
                printf("*"); /* Segnaposto */
            } else {
                printf(" ");
            }
        }
        printf("|\n");
    }
    
    /* Bordo inferiore */
    printf("+");
    for (j = 0; j < MATRIX_SIZE; j++) printf("-");
    printf("+\n");
    
    fflush(stdout);
}

/*
 * Parsa messaggio ricevuto ed estrae posizione
 */
static int parse_position(const char *buffer)
{
    int x, y;
    long timestamp;
    
    /* Formato atteso: "POS x y timestamp" */
    if (sscanf(buffer, "POS %d %d %ld", &x, &y, &timestamp) == 3) {
        /* Verifica validità coordinate */
        if (x >= 0 && x < MATRIX_SIZE && y >= 0 && y < MATRIX_SIZE) {
            current_pos.x = x;
            current_pos.y = y;
            current_pos.timestamp = timestamp;
            current_pos.valid = 1;
            return 1;
        }
    }
    
    return 0; /* Formato non valido */
}

/*
 * Pulisce schermo
 */
static void clear_screen(void)
{
    printf("\033[2J"); /* Clear screen */
    printf("\033[H");  /* Home cursor */
}

/*
 * Muove cursore a posizione specifica
 */
static void move_cursor(int row, int col)
{
    printf("\033[%d;%dH", row, col);
}

/*
 * Gestisce terminazione pulita
 */
static void cleanup_and_exit(int sig)
{
    struct ip_mreq mreq;
    
    printf("\n\nShutting down receiver...\n");
    
    /* Leave dal gruppo multicast */
    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_ADDR);
    mreq.imr_interface.s_addr = INADDR_ANY;
    setsockopt(sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
    
    close(sockfd);
    exit(0);
}