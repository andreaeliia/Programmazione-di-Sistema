/*
 * game_server.c - Server per sistema multiplayer su matrice 200x200
 * 
 * Gestisce stato del gioco, movimenti client, eliminazioni e 
 * comunicazione multicast per aggiornamenti real-time.
 *
 * Uso: ./game_server
 * Il server rileva automaticamente IP e configura porte/multicast
 *
 * Compatibile Linux/macOS con libreria APUE
 */

#include "apue.h"
#include "game_protocol.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

/* Stato globale del server */
typedef struct {
    client_info_t clients[MAX_CLIENTS];
    int num_clients;
    int game_matrix[MATRIX_SIZE][MATRIX_SIZE];  /* 0=vuoto, client_id=occupato */
    int game_running;
    int next_client_id;
    
    /* Configurazione rete */
    int unicast_port;
    char multicast_addr[32];
    int unicast_sock;
    int multicast_sock;
    struct sockaddr_in multicast_sockaddr;
} game_server_t;

static game_server_t g_server;

/* Prototipi funzioni */
static int get_local_ip_last_octet(void);
static void init_server(void);
static void setup_network(void);
static void handle_client_message(void);
static void process_client_join(msg_client_join_t *msg, struct sockaddr_in *client_addr);
static void process_client_move(msg_client_move_t *msg);
static void process_client_quit(msg_client_quit_t *msg);
static int find_client_by_id(int client_id);
static int is_position_valid(position_t pos);
static int check_collision(position_t pos, int exclude_client_id);
static void eliminate_client(int client_id);
static void update_client_position(int client_id, position_t new_pos);
static void send_multicast_update(void);
static void send_elimination_notice(int eliminated_id, int eliminator_id);
static void send_game_end_notice(int winner_id);
static void check_game_end(void);
static void cleanup_server(void);
static void print_game_status(void);

/*
 * Ottiene ultimo ottetto dell'IP locale
 */
static int get_local_ip_last_octet(void)
{
    int sockfd;
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    unsigned char *ip_bytes;
    
    /* Crea socket temporaneo per ottenere IP locale */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        return 100;  /* Fallback default */
    }
    
    /* Connetti a indirizzo pubblico per ottenere IP locale */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "8.8.8.8", &addr.sin_addr);
    
    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sockfd);
        return 100;  /* Fallback default */
    }
    
    /* Ottieni IP locale usato per la connessione */
    if (getsockname(sockfd, (struct sockaddr*)&addr, &addr_len) < 0) {
        close(sockfd);
        return 100;  /* Fallback default */
    }
    
    close(sockfd);
    
    /* Estrai ultimo ottetto */
    ip_bytes = (unsigned char*)&addr.sin_addr.s_addr;
    return (int)ip_bytes[3];
}

/*
 * Inizializza stato del server
 */
static void init_server(void)
{
    int i, j;
    int last_octet;
    
    /* Inizializza strutture dati */
    memset(&g_server, 0, sizeof(g_server));
    g_server.game_running = 1;
    g_server.next_client_id = 1;
    
    /* Inizializza matrice di gioco */
    for (i = 0; i < MATRIX_SIZE; i++) {
        for (j = 0; j < MATRIX_SIZE; j++) {
            g_server.game_matrix[i][j] = 0;  /* Vuoto */
        }
    }
    
    /* Configura indirizzi di rete */
    last_octet = get_local_ip_last_octet();
    g_server.unicast_port = UNICAST_PORT_BASE + last_octet;
    snprintf(g_server.multicast_addr, sizeof(g_server.multicast_addr), 
             "%s%d", MULTICAST_BASE, last_octet);
    
    printf("=== Game Server Initialized ===\n");
    printf("Unicast port: %d\n", g_server.unicast_port);
    printf("Multicast address: %s:%d\n", g_server.multicast_addr, MULTICAST_PORT);
    printf("Matrix size: %dx%d\n", MATRIX_SIZE, MATRIX_SIZE);
    printf("Max clients: %d\n", MAX_CLIENTS);
}

/*
 * Configura socket di rete
 */
static void setup_network(void)
{
    struct sockaddr_in unicast_addr;
    int opt = 1;
    
    /* Socket unicast per ricezione da client */
    g_server.unicast_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_server.unicast_sock < 0) {
        err_sys("unicast socket creation failed");
    }
    
    setsockopt(g_server.unicast_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    /* Bind socket unicast */
    memset(&unicast_addr, 0, sizeof(unicast_addr));
    unicast_addr.sin_family = AF_INET;
    unicast_addr.sin_addr.s_addr = INADDR_ANY;
    unicast_addr.sin_port = htons(g_server.unicast_port);
    
    if (bind(g_server.unicast_sock, (struct sockaddr*)&unicast_addr, 
             sizeof(unicast_addr)) < 0) {
        err_sys("unicast bind failed");
    }
    
    /* Socket multicast per invio a client */
    g_server.multicast_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (g_server.multicast_sock < 0) {
        err_sys("multicast socket creation failed");
    }
    
    /* Configura indirizzo multicast */
    memset(&g_server.multicast_sockaddr, 0, sizeof(g_server.multicast_sockaddr));
    g_server.multicast_sockaddr.sin_family = AF_INET;
    g_server.multicast_sockaddr.sin_port = htons(MULTICAST_PORT);
    inet_pton(AF_INET, g_server.multicast_addr, &g_server.multicast_sockaddr.sin_addr);
    
    printf("Network setup complete\n");
}

/*
 * Gestisce messaggio ricevuto da client
 */
static void handle_client_message(void)
{
    game_message_t msg;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    ssize_t bytes_received;
    
    bytes_received = recvfrom(g_server.unicast_sock, &msg, sizeof(msg), 0,
                              (struct sockaddr*)&client_addr, &addr_len);
    
    if (bytes_received < 0) {
        return;  /* Errore o timeout */
    }
    
    /* Processa messaggio in base al tipo */
    switch (msg.msg_type) {
        case MSG_CLIENT_JOIN:
            process_client_join(&msg.join, &client_addr);
            break;
        case MSG_CLIENT_MOVE:
            process_client_move(&msg.move);
            break;
        case MSG_CLIENT_QUIT:
            process_client_quit(&msg.quit);
            break;
        default:
            printf("Unknown message type: %d\n", msg.msg_type);
            break;
    }
}

/*
 * Processa richiesta di join da client
 */
static void process_client_join(msg_client_join_t *msg, struct sockaddr_in *client_addr)
{
    int client_index;
    position_t pos = msg->initial_pos;
    
    /* Verifica se il client esiste già */
    client_index = find_client_by_id(msg->client_id);
    if (client_index >= 0) {
        printf("Client %d already exists\n", msg->client_id);
        return;
    }
    
    /* Verifica posizione valida e libera */
    if (!is_position_valid(pos) || check_collision(pos, -1) >= 0) {
        printf("Invalid or occupied position for client %d\n", msg->client_id);
        return;
    }
    
    /* Aggiungi nuovo client */
    if (g_server.num_clients < MAX_CLIENTS) {
        client_index = g_server.num_clients++;
        g_server.clients[client_index].client_id = msg->client_id;
        strncpy(g_server.clients[client_index].nickname, msg->nickname, 
                MAX_NICKNAME_LEN - 1);
        g_server.clients[client_index].nickname[MAX_NICKNAME_LEN - 1] = '\0';
        g_server.clients[client_index].pos = pos;
        g_server.clients[client_index].active = 1;
        
        /* Aggiorna matrice */
        g_server.game_matrix[pos.y][pos.x] = msg->client_id;
        
        printf("Client %d (%s) joined at (%d,%d)\n", 
               msg->client_id, msg->nickname, pos.x, pos.y);
    } else {
        printf("Maximum clients reached, rejecting client %d\n", msg->client_id);
    }
}

/*
 * Processa movimento da client
 */
static void process_client_move(msg_client_move_t *msg)
{
    int client_index;
    position_t current_pos, new_pos;
    int collision_client;
    
    /* Trova client */
    client_index = find_client_by_id(msg->client_id);
    if (client_index < 0 || !g_server.clients[client_index].active) {
        return;  /* Client non trovato o non attivo */
    }
    
    current_pos = g_server.clients[client_index].pos;
    new_pos = current_pos;
    
    /* Calcola nuova posizione */
    switch (msg->direction) {
        case DIR_UP:
            new_pos.y--;
            break;
        case DIR_DOWN:
            new_pos.y++;
            break;
        case DIR_LEFT:
            new_pos.x--;
            break;
        case DIR_RIGHT:
            new_pos.x++;
            break;
        default:
            return;  /* Direzione non valida */
    }
    
    /* Verifica validità posizione */
    if (!is_position_valid(new_pos)) {
        return;  /* Fuori dai limiti */
    }
    
    /* Verifica collisioni */
    collision_client = check_collision(new_pos, msg->client_id);
    if (collision_client >= 0) {
        /* Elimina client in collisione */
        eliminate_client(collision_client);
        send_elimination_notice(collision_client, msg->client_id);
    }
    
    /* Aggiorna posizione */
    update_client_position(msg->client_id, new_pos);
    
    printf("Client %d moved to (%d,%d)\n", msg->client_id, new_pos.x, new_pos.y);
}

/*
 * Processa disconnessione client
 */
static void process_client_quit(msg_client_quit_t *msg)
{
    int client_index;
    
    client_index = find_client_by_id(msg->client_id);
    if (client_index >= 0) {
        eliminate_client(msg->client_id);
        printf("Client %d quit the game\n", msg->client_id);
    }
}

/*
 * Trova client per ID
 */
static int find_client_by_id(int client_id)
{
    int i;
    
    for (i = 0; i < g_server.num_clients; i++) {
        if (g_server.clients[i].client_id == client_id) {
            return i;
        }
    }
    return -1;
}

/*
 * Verifica se posizione è valida
 */
static int is_position_valid(position_t pos)
{
    return (pos.x >= 0 && pos.x < MATRIX_SIZE && 
            pos.y >= 0 && pos.y < MATRIX_SIZE);
}

/*
 * Verifica collisione in posizione
 */
static int check_collision(position_t pos, int exclude_client_id)
{
    int occupant = g_server.game_matrix[pos.y][pos.x];
    
    if (occupant == 0 || occupant == exclude_client_id) {
        return -1;  /* Nessuna collisione */
    }
    
    return occupant;  /* ID del client in collisione */
}

/*
 * Elimina client dal gioco
 */
static void eliminate_client(int client_id)
{
    int client_index;
    position_t pos;
    
    client_index = find_client_by_id(client_id);
    if (client_index < 0) {
        return;
    }
    
    /* Rimuovi da matrice */
    pos = g_server.clients[client_index].pos;
    g_server.game_matrix[pos.y][pos.x] = 0;
    
    /* Marca come non attivo */
    g_server.clients[client_index].active = 0;
    
    printf("Client %d eliminated\n", client_id);
}

/*
 * Aggiorna posizione client
 */
static void update_client_position(int client_id, position_t new_pos)
{
    int client_index;
    position_t old_pos;
    
    client_index = find_client_by_id(client_id);
    if (client_index < 0) {
        return;
    }
    
    /* Aggiorna matrice */
    old_pos = g_server.clients[client_index].pos;
    g_server.game_matrix[old_pos.y][old_pos.x] = 0;
    g_server.game_matrix[new_pos.y][new_pos.x] = client_id;
    
    /* Aggiorna posizione client */
    g_server.clients[client_index].pos = new_pos;
}

/*
 * Invia aggiornamento multicast a tutti i client
 */
static void send_multicast_update(void)
{
    msg_server_update_t update_msg;
    int i, active_count;
    
    /* Prepara messaggio di aggiornamento */
    memset(&update_msg, 0, sizeof(update_msg));
    update_msg.msg_type = MSG_SERVER_UPDATE;
    
    /* Copia solo client attivi */
    active_count = 0;
    for (i = 0; i < g_server.num_clients; i++) {
        if (g_server.clients[i].active) {
            update_msg.clients[active_count] = g_server.clients[i];
            active_count++;
        }
    }
    update_msg.num_clients = active_count;
    
    /* Invia multicast */
    sendto(g_server.multicast_sock, &update_msg, sizeof(update_msg), 0,
           (struct sockaddr*)&g_server.multicast_sockaddr, 
           sizeof(g_server.multicast_sockaddr));
}

/*
 * Invia notifica eliminazione
 */
static void send_elimination_notice(int eliminated_id, int eliminator_id)
{
    msg_server_elimination_t elim_msg;
    
    memset(&elim_msg, 0, sizeof(elim_msg));
    elim_msg.msg_type = MSG_SERVER_ELIMINATION;
    elim_msg.eliminated_client_id = eliminated_id;
    elim_msg.eliminator_client_id = eliminator_id;
    
    sendto(g_server.multicast_sock, &elim_msg, sizeof(elim_msg), 0,
           (struct sockaddr*)&g_server.multicast_sockaddr, 
           sizeof(g_server.multicast_sockaddr));
}

/*
 * Invia notifica fine gioco
 */
static void send_game_end_notice(int winner_id)
{
    msg_server_game_end_t end_msg;
    int winner_index;
    
    memset(&end_msg, 0, sizeof(end_msg));
    end_msg.msg_type = MSG_SERVER_GAME_END;
    end_msg.winner_client_id = winner_id;
    
    winner_index = find_client_by_id(winner_id);
    if (winner_index >= 0) {
        strncpy(end_msg.winner_nickname, g_server.clients[winner_index].nickname,
                MAX_NICKNAME_LEN - 1);
    }
    
    sendto(g_server.multicast_sock, &end_msg, sizeof(end_msg), 0,
           (struct sockaddr*)&g_server.multicast_sockaddr, 
           sizeof(g_server.multicast_sockaddr));
}

/*
 * Verifica condizioni di fine gioco
 */
static void check_game_end(void)
{
    int i, active_count = 0, last_active_id = -1;
    
    /* Conta client attivi */
    for (i = 0; i < g_server.num_clients; i++) {
        if (g_server.clients[i].active) {
            active_count++;
            last_active_id = g_server.clients[i].client_id;
        }
    }
    
    if (active_count <= 1) {
        if (active_count == 1) {
            printf("Game ended! Winner: Client %d\n", last_active_id);
            send_game_end_notice(last_active_id);
        } else {
            printf("Game ended! No winners\n");
        }
        g_server.game_running = 0;
    }
}

/*
 * Stampa stato attuale del gioco
 */
static void print_game_status(void)
{
    int i, active_count = 0;
    
    for (i = 0; i < g_server.num_clients; i++) {
        if (g_server.clients[i].active) {
            active_count++;
            printf("Client %d (%s) at (%d,%d)\n", 
                   g_server.clients[i].client_id,
                   g_server.clients[i].nickname,
                   g_server.clients[i].pos.x,
                   g_server.clients[i].pos.y);
        }
    }
    printf("Active clients: %d\n\n", active_count);
}

/*
 * Cleanup risorse server
 */
static void cleanup_server(void)
{
    if (g_server.unicast_sock >= 0) {
        close(g_server.unicast_sock);
    }
    if (g_server.multicast_sock >= 0) {
        close(g_server.multicast_sock);
    }
}

/*
 * Funzione principale
 */
int main(void)
{
    fd_set readfds;
    struct timeval timeout;
    int result;
    struct timeval last_update, now;
    
    printf("=== Multiplayer Game Server ===\n");
    printf("Matrix size: %dx%d\n", MATRIX_SIZE, MATRIX_SIZE);
    
    /* Inizializza server */
    init_server();
    setup_network();
    
    gettimeofday(&last_update, NULL);
    
    printf("Server running...\n");
    printf("Waiting for clients on port %d\n", g_server.unicast_port);
    printf("Multicast updates on %s:%d\n\n", g_server.multicast_addr, MULTICAST_PORT);
    
    /* Loop principale del server */
    while (g_server.game_running) {
        /* Setup select per timeout */
        FD_ZERO(&readfds);
        FD_SET(g_server.unicast_sock, &readfds);
        
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;  /* 100ms timeout */
        
        result = select(g_server.unicast_sock + 1, &readfds, NULL, NULL, &timeout);
        
        if (result > 0 && FD_ISSET(g_server.unicast_sock, &readfds)) {
            handle_client_message();
        }
        
        /* Invia aggiornamenti periodici */
        gettimeofday(&now, NULL);
        if ((now.tv_sec - last_update.tv_sec) * 1000000 + 
            (now.tv_usec - last_update.tv_usec) >= SERVER_UPDATE_INTERVAL) {
            
            send_multicast_update();
            print_game_status();
            check_game_end();
            last_update = now;
        }
    }
    
    printf("Game server shutting down...\n");
    cleanup_server();
    
    return 0;
}