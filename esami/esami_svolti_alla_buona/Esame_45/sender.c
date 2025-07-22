/*
 * Sender - Programma per controllo segnaposto e trasmissione multicast
 * Permette di muovere un segnaposto in una matrice 20x20
 * e trasmette la posizione in tempo reale via multicast
 */

#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/time.h>

#define MATRIX_SIZE 20
#define MULTICAST_ADDR "239.255.255.250"
#define MULTICAST_PORT 12345
#define BUFFER_SIZE 256

/* Struttura per posizione del segnaposto */
struct position {
    int x;
    int y;
    time_t timestamp;
};

/* Variabili globali */
static struct position current_pos = {10, 10, 0}; /* Posizione iniziale al centro */
static struct termios old_termios;
static int sockfd;

/* Prototipi delle funzioni */
static void setup_multicast_sender(void);
static void setup_terminal(void);
static void restore_terminal(void);
static void display_matrix(void);
static void send_position(void);
static int handle_input(void);
static void cleanup_and_exit(int sig);
static void clear_screen(void);
static void move_cursor(int row, int col);

/*
 * Funzione principale del sender
 */
int main(void)
{
    fd_set readfds;
    struct timeval tv;
    int retval;
    
    printf("=== Multicast Matrix Sender ===\n");
    printf("Initializing...\n");
    
    /* Configura gestione segnali */
    signal(SIGINT, cleanup_and_exit);
    signal(SIGTERM, cleanup_and_exit);
    
    /* Inizializza socket multicast */
    setup_multicast_sender();
    
    /* Configura terminale per input non-echo */
    setup_terminal();
    
    /* Mostra interfaccia iniziale */
    clear_screen();
    display_matrix();
    
    printf("\nControls:\n");
    printf("  Arrow keys or WASD: Move marker\n");
    printf("  'q' or ESC: Quit\n");
    printf("\nPosition: (%d,%d)\n", current_pos.x, current_pos.y);
    
    /* Invia posizione iniziale */
    send_position();
    
    /* Loop principale */
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        
        /* Timeout per refresh periodico */
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        
        retval = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);
        
        if (retval == -1) {
            if (errno == EINTR) continue;
            err_sys("select error");
        } else if (retval > 0) {
            /* Input disponibile */
            if (FD_ISSET(STDIN_FILENO, &readfds)) {
                if (handle_input() == 0) {
                    break; /* Exit richiesto */
                }
            }
        }
        
        /* Refresh periodico della visualizzazione */
        move_cursor(MATRIX_SIZE + 5, 1);
        printf("Position: (%d,%d) - Broadcasting...   ", 
               current_pos.x, current_pos.y);
        fflush(stdout);
    }
    
    cleanup_and_exit(0);
    return 0;
}

/*
 * Configura socket multicast per invio
 */
static void setup_multicast_sender(void)
{
    struct sockaddr_in addr;
    int ttl = 1; /* TTL per rete locale */
    
    /* Crea socket UDP */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        err_sys("socket error");
    }
    
    /* Configura TTL multicast */
    if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, 
                   &ttl, sizeof(ttl)) < 0) {
        err_sys("setsockopt IP_MULTICAST_TTL error");
    }
    
    /* Configura indirizzo multicast */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(MULTICAST_ADDR);
    addr.sin_port = htons(MULTICAST_PORT);
    
    /* Connetti al gruppo multicast */
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        err_sys("connect error");
    }
    
    printf("Multicast sender configured: %s:%d\n", 
           MULTICAST_ADDR, MULTICAST_PORT);
}

/*
 * Configura terminale per input immediato senza echo
 */
static void setup_terminal(void)
{
    struct termios new_termios;
    
    /* Salva configurazione corrente */
    if (tcgetattr(STDIN_FILENO, &old_termios) < 0) {
        err_sys("tcgetattr error");
    }
    
    /* Configura nuovo modo */
    new_termios = old_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO); /* Disabilita canonical mode e echo */
    new_termios.c_cc[VMIN] = 1;
    new_termios.c_cc[VTIME] = 0;
    
    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_termios) < 0) {
        err_sys("tcsetattr error");
    }
}

/*
 * Ripristina configurazione terminale
 */
static void restore_terminal(void)
{
    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
}

/*
 * Mostra la matrice con il segnaposto
 */
static void display_matrix(void)
{
    int i, j;
    
    move_cursor(1, 1);
    
    /* Bordo superiore */
    printf("+");
    for (j = 0; j < MATRIX_SIZE; j++) printf("-");
    printf("+\n");
    
    /* Righe della matrice */
    for (i = 0; i < MATRIX_SIZE; i++) {
        printf("|");
        for (j = 0; j < MATRIX_SIZE; j++) {
            if (i == current_pos.y && j == current_pos.x) {
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
 * Invia posizione corrente via multicast
 */
static void send_position(void)
{
    char buffer[BUFFER_SIZE];
    int len;
    
    current_pos.timestamp = time(NULL);
    
    /* Formato: "POS x y timestamp" */
    len = snprintf(buffer, BUFFER_SIZE, "POS %d %d %ld", 
                   current_pos.x, current_pos.y, current_pos.timestamp);
    
    if (send(sockfd, buffer, len, 0) < 0) {
        err_sys("send error");
    }
}

/*
 * Gestisce input da tastiera
 */
static int handle_input(void)
{
    char ch;
    int moved = 0;
    
    if (read(STDIN_FILENO, &ch, 1) <= 0) {
        return 1;
    }
    
    /* Gestione tasti freccia (sequenze escape) */
    if (ch == 27) { /* ESC */
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) == 1 && seq[0] == '[') {
            if (read(STDIN_FILENO, &seq[1], 1) == 1) {
                switch (seq[1]) {
                    case 'A': /* Freccia su */
                        if (current_pos.y > 0) {
                            current_pos.y--;
                            moved = 1;
                        }
                        break;
                    case 'B': /* Freccia giu */
                        if (current_pos.y < MATRIX_SIZE - 1) {
                            current_pos.y++;
                            moved = 1;
                        }
                        break;
                    case 'C': /* Freccia destra */
                        if (current_pos.x < MATRIX_SIZE - 1) {
                            current_pos.x++;
                            moved = 1;
                        }
                        break;
                    case 'D': /* Freccia sinistra */
                        if (current_pos.x > 0) {
                            current_pos.x--;
                            moved = 1;
                        }
                        break;
                }
            }
        } else {
            /* ESC da solo - esci */
            return 0;
        }
    } else {
        /* Gestione tasti alternativi */
        switch (ch) {
            case 'w': case 'W':
                if (current_pos.y > 0) {
                    current_pos.y--;
                    moved = 1;
                }
                break;
            case 's': case 'S':
                if (current_pos.y < MATRIX_SIZE - 1) {
                    current_pos.y++;
                    moved = 1;
                }
                break;
            case 'a': case 'A':
                if (current_pos.x > 0) {
                    current_pos.x--;
                    moved = 1;
                }
                break;
            case 'd': case 'D':
                if (current_pos.x < MATRIX_SIZE - 1) {
                    current_pos.x++;
                    moved = 1;
                }
                break;
            case 'q': case 'Q':
                return 0; /* Esci */
        }
    }
    
    /* Se c'e stato movimento, aggiorna display e invia */
    if (moved) {
        display_matrix();
        send_position();
        
        move_cursor(MATRIX_SIZE + 5, 1);
        printf("Position: (%d,%d) - Sent!        ", 
               current_pos.x, current_pos.y);
        fflush(stdout);
    }
    
    return 1; /* Continua */
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
    printf("\n\nShutting down sender...\n");
    restore_terminal();
    close(sockfd);
    exit(0);
}