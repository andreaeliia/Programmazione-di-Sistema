/*
 * game_client.c - Client per sistema multiplayer su matrice 200x200
 * 
 * Gestisce input utente, comunicazione con server e rendering
 * della matrice di gioco in tempo reale.
 *
 * Uso: ./game_client <nickname> [server_ip]
 * Esempio: ./game_client Player1 192.168.1.100
 *
 * Controlli: u=su, n=giù, h=sinistra, j=destra, q=quit
 *
 * Compatibile Linux/macOS con libreria APUE
 */

#include "apue.h"
#include "game_protocol.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <termios.h>
#include <fcntl.h>

/* Stato del client */
typedef struct {
    int client_id;
    char nickname[MAX_NICKNAME_LEN];
    position_t current_pos;
    int game_active;
    
    /* Configurazione rete */
    char server_ip[32];
    int server_port;
    char multicast_addr[32];
    int unicast_sock;
    int multicast_sock;
    struct sockaddr_in server_sockaddr;
    
    /* Stato gioco */
    client_info_t other_clients[MAX_CLIENTS];
    int num_other_clients;
    
    /* Terminale */
    struct termios orig_termios;
    int terminal_configured;
} game_client_t;

static game_client_t g_client;

/* Prototipi funzioni */
static int get_local_ip_last_octet(void);
static void init_client(const char *nickname, const char *server_ip);
static void setup_network(void);
static void setup_terminal(void);
static void restore_terminal(void);
static int kbhit(void);
static char getch(void);
static void send_join_request(void);
static void send_move_request(direction_t direction);
static void send_quit_request(void);
static void handle_server_message(void);
static void process_server_update(msg_server_update_t *msg);
static void process_elimination(msg_server_elimination_t *msg);
static void process_game_end(msg_server_game_end_t *msg);
static void render_game_matrix(void);
static position_t get_random_start_position(void);
static void print_instructions(void);
static void cleanup_client(void);

/*
 * Ottiene ultimo ottetto dell'IP locale
 */
static int get_local_ip_last_octet(void)
{
    int sockfd;
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    unsigned char *ip_bytes;
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        return 100;
    }
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "8.8.8.8", &addr.sin_addr);
    
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sockfd);
        return 100;
    }
    
    if (getsockname(sockfd, (struct sockaddr*)&addr, &addr_len) < 0) {
        close(sockfd);
        return 100;
    }
    
    close(sockfd);
    
    ip_bytes = (unsigned char*)&addr.sin_addr.s_addr;
    return (int)ip_bytes[3];
}

/*
 * Inizializza client
 */
static void init_client(const char *nickname, const char *server_ip)
{
    int last_octet;
    
    memset(&g_client, 0, sizeof(g_client));
    
    /* Genera ID client unico */
    g_client.client_id = getpid();
    
    /* Copia nickname */
    strncpy(g_client.nickname, nickname, MAX_NICKNAME_LEN - 1);
    g_client.nickname[MAX_NICKNAME_LEN - 1] = '\0';
    
    /* Configura IP server */
    if (server_ip) {
        strncpy(g_client.server_ip, server_ip, sizeof(g_client.server_ip) - 1);
    } else {
        strcpy(g_client.server_ip, "127.0.0.1");  /* Default localhost */
    }
    
    /* Calcola porte e indirizzo multicast */
    last_octet = get_local_ip_last_octet();
    g_client.server_port = UNICAST_PORT_BASE + last_octet;
    snprintf(g_client.multicast_addr, sizeof(g_client.multicast_addr),
             "%s%d", MULTICAST_BASE, last_octet);
    
    /* Posizione iniziale casuale */
    g_client.current_pos = get_random_start_position();
    g_client.game_active = 1;
    
    printf("=== Game Client Initialized ===\n");
    printf("Client ID: %d\n", g_client.client_id);
    printf("Nickname: %s\n", g_client.nickname);
    printf("Server: %s:%d\n", g_client.server_ip, g_client.server_port);
    printf("Multicast: %s:%d\n", g_client.multicast_addr, MULTICAST_PORT);
    printf("Start position: (%d,%d)\n", g_client.current_pos.x, g_client.current_pos.y);
}

/*
 * Configura socket di rete
 */
static void setup_network(void)
{
    struct sockaddr_in multicast_addr;
    struct ip_mreq mreq;
    int opt = 1;
    
    /* Socket unicast per comunicazione con server */
    g_client.unicast_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_client.unicast_sock < 0) {
        err_sys("unicast socket creation failed");
    }
    
    /* Configura indirizzo server */
    memset(&g_client.server_sockaddr, 0, sizeof(g_client.server_sockaddr));
    g_client.server_sockaddr.sin_family = AF_INET;
    g_client.server_sockaddr.sin_port = htons(g_client.server_port);
    inet_pton(AF_INET, g_client.server_ip, &g_client.server_sockaddr.sin_addr);
    
    /* Socket multicast per ricezione aggiornamenti */
    g_client.multicast_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_client.multicast_sock < 0) {
        err_sys("multicast socket creation failed");
    }
    
    setsockopt(g_client.multicast_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    /* Bind per ricezione multicast */
    memset(&multicast_addr, 0, sizeof(multicast_addr));
    multicast_addr.sin_family = AF_INET;
    multicast_addr.sin_addr.s_addr = INADDR_ANY;
    multicast_addr.sin_port = htons(MULTICAST_PORT);
    
    if (bind(g_client.multicast_sock, (struct sockaddr*)&multicast_addr, 
             sizeof(multicast_addr)) < 0) {
        err_sys("multicast bind failed");
    }
    
    /* Join gruppo multicast */
    mreq.imr_multiaddr.s_addr = inet_addr(g_client.multicast_addr);
    mreq.imr_interface.s_addr = INADDR_ANY;
    
    if (setsockopt(g_client.multicast_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                   &mreq, sizeof(mreq)) < 0) {
        err_sys("multicast join failed");
    }
    
    printf("Network setup complete\n");
}

/*
 * Configura terminale per input non-bloccante
 */
static void setup_terminal(void)
{
    struct termios new_termios;
    
    /* Salva configurazione originale */
    if (tcgetattr(STDIN_FILENO, &g_client.orig_termios) < 0) {
        err_sys("tcgetattr failed");
    }
    
    g_client.terminal_configured = 1;
    
    /* Configura modalità raw */
    new_termios = g_client.orig_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    new_termios.c_cc[VMIN] = 0;
    new_termios.c_cc[VTIME] = 0;
    
    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_termios) < 0) {
        err_sys("tcsetattr failed");
    }
    
    /* Rendi stdin non-bloccante */
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
}

/*
 * Ripristina configurazione terminale
 */
static void restore_terminal(void)
{
    if (g_client.terminal_configured) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_client.orig_termios);
        fcntl(STDIN_FILENO, F_SETFL, 0);  /* Rimuovi O_NONBLOCK */
    }
}

/*
 * Verifica se tasto premuto
 */
static int kbhit(void)
{
    int ch = getchar();
    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }
    return 0;
}

/*
 * Legge carattere senza echo
 */
static char getch(void)
{
    return getchar();
}

/*
 * Invia richiesta di join al server
 */
static void send_join_request(void)
{
    msg_client_join_t join_msg;
    
    memset(&join_msg, 0, sizeof(join_msg));
    join_msg.msg_type = MSG_CLIENT_JOIN;
    join_msg.client_id = g_client.client_id;
    strncpy(join_msg.nickname, g_client.nickname, MAX_NICKNAME_LEN - 1);
    join_msg.initial_pos = g_client.current_pos;
    
    sendto(g_client.unicast_sock, &join_msg, sizeof(join_msg), 0,
           (struct sockaddr*)&g_client.server_sockaddr, 
           sizeof(g_client.server_sockaddr));
    
    printf("Join request sent\n");
}

/*
 * Invia richiesta di movimento al server
 */
static void send_move_request(direction_t direction)
{
    msg_client_move_t move_msg;
    
    memset(&move_msg, 0, sizeof(move_msg));
    move_msg.msg_type = MSG_CLIENT_MOVE;
    move_msg.client_id = g_client.client_id;
    move_msg.direction = direction;
    
    sendto(g_client.unicast_sock, &move_msg, sizeof(move_msg), 0,
           (struct sockaddr*)&g_client.server_sockaddr, 
           sizeof(g_client.server_sockaddr));
}

/*
 * Invia richiesta di quit al server
 */
static void send_quit_request(void)
{
    msg_client_quit_t quit_msg;
    
    memset(&quit_msg, 0, sizeof(quit_msg));
    quit_msg.msg_type = MSG_CLIENT_QUIT;
    quit_msg.client_id = g_client.client_id;
    
    sendto(g_client.unicast_sock, &quit_msg, sizeof(quit_msg), 0,
           (struct sockaddr*)&g_client.server_sockaddr, 
           sizeof(g_client.server_sockaddr));
    
    printf("Quit request sent\n");
}

/*
 * Gestisce messaggio dal server
 */
static void handle_server_message(void)
{
    game_message_t msg;
    ssize_t bytes_received;
    
    bytes_received = recv(g_client.multicast_sock, &msg, sizeof(msg), MSG_DONTWAIT);
    
    if (bytes_received < 0) {
        return;  /* Nessun messaggio */
    }
    
    switch (msg.msg_type) {
        case MSG_SERVER_UPDATE:
            process_server_update(&msg.update);
            break;
        case MSG_SERVER_ELIMINATION:
            process_elimination(&msg.elimination);
            break;
        case MSG_SERVER_GAME_END:
            process_game_end(&msg.game_end);
            break;
        default:
            break;
    }
}

/*
 * Processa aggiornamento stato dal server
 */
static void process_server_update(msg_server_update_t *msg)
{
    int i;
    
    /* Aggiorna lista altri client */
    g_client.num_other_clients = 0;
    
    for (i = 0; i < msg->num_clients; i++) {
        if (msg->clients[i].client_id == g_client.client_id) {
            /* Aggiorna la mia posizione */
            g_client.current_pos = msg->clients[i].pos;
        } else if (msg->clients[i].active) {
            /* Aggiungi altro client */
            if (g_client.num_other_clients < MAX_CLIENTS - 1) {
                g_client.other_clients[g_client.num_other_clients] = msg->clients[i];
                g_client.num_other_clients++;
            }
        }
    }
    
    render_game_matrix();
}

/*
 * Processa notifica eliminazione
 */
static void process_elimination(msg_server_elimination_t *msg)
{
    if (msg->eliminated_client_id == g_client.client_id) {
        printf("\n=== YOU HAVE BEEN ELIMINATED ===\n");
        printf("Eliminated by client %d\n", msg->eliminator_client_id);
        g_client.game_active = 0;
    } else {
        printf("\nClient %d eliminated by client %d\n", 
               msg->eliminated_client_id, msg->eliminator_client_id);
    }
}

/*
 * Processa notifica fine gioco
 */
static void process_game_end(msg_server_game_end_t *msg)
{
    printf("\n=== GAME ENDED ===\n");
    if (msg->winner_client_id == g_client.client_id) {
        printf("CONGRATULATIONS! YOU WON!\n");
    } else {
        printf("Winner: %s (Client %d)\n", 
               msg->winner_nickname, msg->winner_client_id);
    }
    g_client.game_active = 0;
}

/*
 * Renderizza matrice di gioco (versione semplificata)
 */
static void render_game_matrix(void)
{
    int i;
    
    /* Clear screen (ANSI escape) */
    printf("\033[2J\033[H");
    
    printf("=== Game Matrix ===\n");
    printf("Your position: (%d,%d) [ID: %d]\n", 
           g_client.current_pos.x, g_client.current_pos.y, g_client.client_id);
    
    printf("\nOther players:\n");
    for (i = 0; i < g_client.num_other_clients; i++) {
        printf("  %s [ID: %d] at (%d,%d)\n",
               g_client.other_clients[i].nickname,
               g_client.other_clients[i].client_id,
               g_client.other_clients[i].pos.x,
               g_client.other_clients[i].pos.y);
    }
    
    printf("\nTotal players: %d\n", g_client.num_other_clients + 1);
    printf("\nControls: u=up, n=down, h=left, j=right, q=quit\n");
}

/*
 * Genera posizione di partenza casuale
 */
static position_t get_random_start_position(void)
{
    position_t pos;
    
    srand(time(NULL) + getpid());
    pos.x = rand() % MATRIX_SIZE;
    pos.y = rand() % MATRIX_SIZE;
    
    return pos;
}

/*
 * Stampa istruzioni
 */
static void print_instructions(void)
{
    printf("\n=== Game Instructions ===\n");
    printf("Move with: u (up), n (down), h (left), j (right)\n");
    printf("Quit with: q\n");
    printf("Collide with other players to eliminate them!\n");
    printf("Last player standing wins!\n\n");
}

/*
 * Cleanup risorse client
 */
static void cleanup_client(void)
{
    restore_terminal();
    
    if (g_client.unicast_sock >= 0) {
        close(g_client.unicast_sock);
    }
    if (g_client.multicast_sock >= 0) {
        close(g_client.multicast_sock);
    }
}

/*
 * Funzione principale
 */
int main(int argc, char *argv[])
{
    char *nickname;
    char *server_ip = NULL;
    char input_char;
    fd_set readfds;
    struct timeval timeout;
    int result;
    
    printf("=== Multiplayer Game Client ===\n");
    
    /* Verifica argomenti */
    if (argc < 2) {
        printf("Usage: %s <nickname> [server_ip]\n", argv[0]);
        printf("Example: %s Player1 192.168.1.100\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    nickname = argv[1];
    if (argc > 2) {
        server_ip = argv[2];
    }
    
    /* Inizializza client */
    init_client(nickname, server_ip);
    setup_network();
    setup_terminal();
    
    print_instructions();
    
    /* Invia richiesta di join */
    send_join_request();
    
    printf("Connecting to game server...\n");
    printf("Waiting for game updates...\n\n");
    
    /* Loop principale del client */
    while (g_client.game_active) {
        /* Verifica input da tastiera */
        if (kbhit()) {
            input_char = getch();
            
            switch (input_char) {
                case 'u':
                    send_move_request(DIR_UP);
                    break;
                case 'n':
                    send_move_request(DIR_DOWN);
                    break;
                case 'h':
                    send_move_request(DIR_LEFT);
                    break;
                case 'j':
                    send_move_request(DIR_RIGHT);
                    break;
                case 'q':
                    send_quit_request();
                    g_client.game_active = 0;
                    break;
                default:
                    break;
            }
        }
        
        /* Verifica messaggi dal server */
        FD_ZERO(&readfds);
        FD_SET(g_client.multicast_sock, &readfds);
        
        timeout.tv_sec = 0;
        timeout.tv_usec = 50000;  /* 50ms timeout */
        
        result = select(g_client.multicast_sock + 1, &readfds, NULL, NULL, &timeout);
        
        if (result > 0 && FD_ISSET(g_client.multicast_sock, &readfds)) {
            handle_server_message();
        }
    }
    
    printf("\nGame session ended. Press Enter to exit...\n");
    restore_terminal();
    getchar();
    
    cleanup_client();
    return 0;
}