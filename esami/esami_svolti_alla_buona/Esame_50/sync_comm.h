/* sync_comm.h - Definizioni comuni per comunicazione sincronizzata */
#ifndef SYNC_COMM_H
#define SYNC_COMM_H

#include "apue.h"
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

/* Configurazione rete */
#define P1_TO_P2_PORT 8001
#define P3_TO_P1_PORT 8002
#define LOOPBACK_IP "127.0.0.1"
#define BACKLOG 5

/* Configurazione memoria condivisa */
#define SHM_KEY 12345
#define SEM_KEY 54321
#define MAX_STRING_LEN 256

/* Struttura per memoria condivisa */
typedef struct {
    char data[MAX_STRING_LEN];
    int ready;        /* Flag: 1 se P2 ha scritto, 0 se P3 ha letto */
} shared_data_t;

/* Struttura per thread T1 */
typedef struct {
    int socket_fd;
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
    int *can_send;
} t1_data_t;

/* Struttura per thread T2 */
typedef struct {
    int socket_fd;
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
    int *can_send;
} t2_data_t;

/* Prototipi funzioni comuni */
void print_nanosecond_time(void);
void generate_random_string(char *str, int length);
int create_tcp_server(int port);
int connect_to_tcp_server(const char *ip, int port);
void send_string(int sockfd, const char *str);
int receive_string(int sockfd, char *str, int max_len);

/* Prototipi funzioni memoria condivisa */
int create_shared_memory(void);
int get_shared_memory(void);
int create_semaphore(void);
int get_semaphore(void);
void sem_wait_op(int semid);
void sem_signal_op(int semid);
void cleanup_ipc_resources(int shmid, int semid);

#endif