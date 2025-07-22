/* token_ring.h - Definizioni comuni per token ring */
#ifndef TOKEN_RING_H
#define TOKEN_RING_H

#include "apue.h"
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <string.h>

/* Configurazione benchmark */
#define TEST_DURATION 10    /* Durata test in secondi */
#define TOKEN_DATA_SIZE 256 /* Dimensione payload token */

/* Configurazione code messaggi */
#define MSG_KEY_BASE 12345
#define MSG_TYPE 1

/* Configurazione TCP */
#define TCP_BASE_PORT 9000
#define BACKLOG 3

/* Struttura token per code messaggi */
typedef struct {
    long mtype;
    int counter;
    int process_id;
    char data[TOKEN_DATA_SIZE];
} msg_token_t;

/* Struttura token per TCP */
typedef struct {
    int counter;
    int process_id;
    char data[TOKEN_DATA_SIZE];
} tcp_token_t;

/* Prototipi funzioni comuni */
double get_time_diff(struct timeval *start, struct timeval *end);
void print_benchmark_results(int process_id, int tokens_processed, double elapsed_time);

/* Prototipi funzioni code messaggi */
int create_message_queue(int process_id);
int get_message_queue(int process_id);
void send_token_msg(int qid, msg_token_t *token, int next_process);
int receive_token_msg(int qid, msg_token_t *token);
void cleanup_message_queues(void);

/* Prototipi funzioni TCP */
int create_tcp_server(int port);
int connect_to_tcp_server(int port);
void send_token_tcp(int sockfd, tcp_token_t *token);
int receive_token_tcp(int sockfd, tcp_token_t *token);

#endif