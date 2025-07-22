/*
 * game_protocol.h - Protocolli di comunicazione per sistema multiplayer
 * 
 * Definisce strutture dati e costanti condivise tra client e server
 * per il gioco multiplayer su matrice 200x200.
 */

#ifndef GAME_PROTOCOL_H
#define GAME_PROTOCOL_H

/* Costanti del gioco */
#define MATRIX_SIZE 200
#define MAX_CLIENTS 100
#define MAX_NICKNAME_LEN 16
#define MULTICAST_PORT 8888
#define SERVER_UPDATE_INTERVAL 1000000  /* 1 secondo in microsecondi */

/* Costanti di rete */
#define MULTICAST_BASE "230.0.0."
#define UNICAST_PORT_BASE 7000

/* Tipi di messaggio */
typedef enum {
    MSG_CLIENT_JOIN = 1,     /* Client richiede partecipazione */
    MSG_CLIENT_MOVE = 2,     /* Client comunica movimento */
    MSG_CLIENT_QUIT = 3,     /* Client abbandona gioco */
    MSG_SERVER_UPDATE = 4,   /* Server invia stato completo */
    MSG_SERVER_ELIMINATION = 5, /* Server notifica eliminazione */
    MSG_SERVER_GAME_END = 6  /* Server notifica fine gioco */
} message_type_t;

/* Direzioni movimento */
typedef enum {
    DIR_UP = 'u',      /* Alto */
    DIR_DOWN = 'n',    /* Basso */ 
    DIR_LEFT = 'h',    /* Sinistra */
    DIR_RIGHT = 'j'    /* Destra */
} direction_t;

/* Struttura posizione */
typedef struct {
    int x;
    int y;
} position_t;

/* Struttura client */
typedef struct {
    int client_id;
    char nickname[MAX_NICKNAME_LEN];
    position_t pos;
    int active;        /* 1 se attivo, 0 se eliminato */
} client_info_t;

/* Messaggio client join */
typedef struct {
    int msg_type;      /* MSG_CLIENT_JOIN */
    int client_id;
    char nickname[MAX_NICKNAME_LEN];
    position_t initial_pos;
} msg_client_join_t;

/* Messaggio client movement */
typedef struct {
    int msg_type;      /* MSG_CLIENT_MOVE */
    int client_id;
    direction_t direction;
} msg_client_move_t;

/* Messaggio client quit */
typedef struct {
    int msg_type;      /* MSG_CLIENT_QUIT */
    int client_id;
} msg_client_quit_t;

/* Messaggio server update */
typedef struct {
    int msg_type;      /* MSG_SERVER_UPDATE */
    int num_clients;
    client_info_t clients[MAX_CLIENTS];
} msg_server_update_t;

/* Messaggio server elimination */
typedef struct {
    int msg_type;      /* MSG_SERVER_ELIMINATION */
    int eliminated_client_id;
    int eliminator_client_id;
} msg_server_elimination_t;

/* Messaggio server game end */
typedef struct {
    int msg_type;      /* MSG_SERVER_GAME_END */
    int winner_client_id;
    char winner_nickname[MAX_NICKNAME_LEN];
} msg_server_game_end_t;

/* Union per tutti i tipi di messaggio */
typedef union {
    int msg_type;
    msg_client_join_t join;
    msg_client_move_t move;
    msg_client_quit_t quit;
    msg_server_update_t update;
    msg_server_elimination_t elimination;
    msg_server_game_end_t game_end;
} game_message_t;

#endif /* GAME_PROTOCOL_H */