/*
 * packet_protocol.h - Protocolli e strutture per sistema pacchetti
 * 
 * Definisce strutture dati e costanti condivise tra sender e receiver
 * per comunicazione con pacchetti a lunghezza variabile.
 */

#ifndef PACKET_PROTOCOL_H
#define PACKET_PROTOCOL_H

#include <sys/types.h>

/* Costanti di configurazione */
#define MAX_PACKET_SIZE 4096        /* Dimensione massima pacchetto in byte */
#define MIN_PACKET_SIZE 16          /* Dimensione minima pacchetto in byte */
#define QUEUE_MAX_SIZE 50           /* Numero massimo pacchetti in coda */
#define SOCKET_PATH "/tmp/packet_socket"  /* Path socket UNIX domain */

/* Struttura header pacchetto */
typedef struct {
    int length;                     /* Lunghezza totale pacchetto (incluso header) */
} packet_header_t;

/* Struttura pacchetto completo */
typedef struct {
    packet_header_t header;         /* Header con lunghezza */
    char data[MAX_PACKET_SIZE - sizeof(packet_header_t)];  /* Dati payload */
} packet_t;

/* Struttura elemento coda circolare */
typedef struct {
    packet_t packet;                /* Pacchetto memorizzato */
    int used;                       /* 1 se slot occupato, 0 se libero */
} queue_element_t;

/* Struttura coda circolare thread-safe */
typedef struct {
    queue_element_t buffer[QUEUE_MAX_SIZE];  /* Buffer circolare */
    int head;                       /* Indice primo elemento (read) */
    int tail;                       /* Indice prossimo slot libero (write) */
    int count;                      /* Numero elementi attualmente in coda */
    pthread_mutex_t mutex;          /* Mutex per accesso esclusivo */
    pthread_cond_t not_empty;       /* Condition: coda non vuota */
    pthread_cond_t not_full;        /* Condition: coda non piena */
    int shutdown;                   /* Flag per terminazione thread */
} circular_queue_t;

/* Struttura parametri thread consumer */
typedef struct {
    circular_queue_t *queue;        /* Puntatore alla coda condivisa */
    int thread_id;                  /* ID thread per logging */
    int *packets_processed;         /* Contatore pacchetti elaborati */
} consumer_thread_params_t;

#endif /* PACKET_PROTOCOL_H */