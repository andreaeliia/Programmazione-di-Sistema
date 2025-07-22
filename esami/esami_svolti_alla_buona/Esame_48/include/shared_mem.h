/* shared_mem.h - Definizioni per memoria condivisa */
#ifndef SHARED_MEM_H
#define SHARED_MEM_H

#include "apue.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define MAX_STRINGS 100
#define MAX_STRING_LEN 256
#define SHM_KEY 12345
#define SEM_KEY 54321
#define PORT1 8001
#define PORT2 8002

/* Struttura per memoria condivisa */
typedef struct {
    char strings[MAX_STRINGS][MAX_STRING_LEN];
    int count;
} shared_data_t;

/* Prototipi funzioni */
int create_shared_memory(void);
int create_semaphore(void);
void sem_wait(int semid);
void sem_signal(int semid);
int create_tcp_server(int port);
void handle_client(int client_fd, shared_data_t *shared_data, int semid);
void cleanup_resources(int shmid, int semid);

#endif