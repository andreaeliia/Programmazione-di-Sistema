/*
 * master.c - Programma Master per sistema distribuito con multicast
 * 
 * Invia messaggi multicast con parametri N e M per coordinare il calcolo
 * distribuito delle radici quadrate su 10 macchine slave
 */

#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MULTICAST_PORT 9999
#define UNICAST_PORT 9998
#define MULTICAST_ADDR "224.0.0.1"  /* Modificare con il proprio IP */
#define MAX_ROUNDS 10
#define NUM_SLAVES 10
#define M_VALUE 15000

/* Struttura per il messaggio multicast */
typedef struct {
    int N;  /* Numero round */
    int M;  /* Valore fisso range */
} multicast_msg_t;

/* Struttura per il messaggio di completamento */
typedef struct {
    int slave_id;
    int round_completed;
} completion_msg_t;

/* Variabili globali */
static int completion_count = 0;
static pthread_mutex_t completion_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t completion_cond = PTHREAD_COND_INITIALIZER;

/*
 * Funzione per creare socket multicast
 */
int create_multicast_socket(void)
{
    int sockfd;
    int ttl = 1;
    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        err_sys("socket error");
    
    /* Imposta TTL per multicast */
    if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0)
        err_sys("setsockopt TTL error");
    
    return sockfd;
}

/*
 * Funzione per creare socket unicast per ricevere conferme
 */
int create_unicast_socket(void)
{
    int sockfd;
    struct sockaddr_in servaddr;
    int reuse = 1;
    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        err_sys("socket error");
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
        err_sys("setsockopt error");
    
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(UNICAST_PORT);
    
    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        err_sys("bind error");
    
    return sockfd;
}

/*
 * Thread per ricevere messaggi di completamento dagli slave
 */
void *completion_receiver(void *arg)
{
    int sockfd = *(int *)arg;
    completion_msg_t msg;
    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);
    
    while (1) {
        if (recvfrom(sockfd, &msg, sizeof(msg), 0, 
                    (struct sockaddr *)&cliaddr, &clilen) < 0)
            err_sys("recvfrom error");
        
        printf("Ricevuto completamento da slave %d per round %d\n", 
               msg.slave_id, msg.round_completed);
        
        pthread_mutex_lock(&completion_mutex);
        completion_count++;
        if (completion_count == NUM_SLAVES) {
            pthread_cond_signal(&completion_cond);
        }
        pthread_mutex_unlock(&completion_mutex);
    }
    
    return NULL;
}

/*
 * Invia tre datagrammi multicast identici distanziati di un secondo
 */
void send_multicast_round(int sockfd, int N, int M)
{
    struct sockaddr_in multiaddr;
    multicast_msg_t msg;
    int i;
    
    bzero(&multiaddr, sizeof(multiaddr));
    multiaddr.sin_family = AF_INET;
    multiaddr.sin_port = htons(MULTICAST_PORT);
    inet_pton(AF_INET, MULTICAST_ADDR, &multiaddr.sin_addr);
    
    msg.N = N;
    msg.M = M;
    
    printf("Invio round %d con M=%d\n", N, M);
    
    /* Invia tre datagrammi identici distanziati di un secondo */
    for (i = 0; i < 3; i++) {
        if (sendto(sockfd, &msg, sizeof(msg), 0, 
                  (struct sockaddr *)&multiaddr, sizeof(multiaddr)) < 0)
            err_sys("sendto error");
        
        printf("  Datagramma %d inviato\n", i + 1);
        
        if (i < 2)  /* Non dormire dopo l'ultimo invio */
            sleep(1);
    }
}

/*
 * Attende che tutti gli slave completino il round corrente
 */
void wait_for_completion(void)
{
    pthread_mutex_lock(&completion_mutex);
    while (completion_count < NUM_SLAVES) {
        pthread_cond_wait(&completion_cond, &completion_mutex);
    }
    completion_count = 0;  /* Reset per il prossimo round */
    pthread_mutex_unlock(&completion_mutex);
    
    printf("Tutti gli slave hanno completato il round\n\n");
}

int main(void)
{
    int multicast_sock, unicast_sock;
    pthread_t receiver_thread;
    int N;
    
    printf("=== MASTER AVVIATO ===\n");
    printf("Multicast: %s:%d\n", MULTICAST_ADDR, MULTICAST_PORT);
    printf("Unicast: porta %d\n\n", UNICAST_PORT);
    
    /* Crea socket */
    multicast_sock = create_multicast_socket();
    unicast_sock = create_unicast_socket();
    
    /* Avvia thread per ricevere conferme */
    if (pthread_create(&receiver_thread, NULL, completion_receiver, &unicast_sock) != 0)
        err_sys("pthread_create error");
    
    /* Esegui tutti i round */
    for (N = 0; N < MAX_ROUNDS; N++) {
        printf("=== ROUND %d ===\n", N);
        
        /* Invia messaggio multicast */
        send_multicast_round(multicast_sock, N, M_VALUE);
        
        /* Attendi che tutti gli slave completino */
        wait_for_completion();
    }
    
    printf("=== TUTTI I ROUND COMPLETATI ===\n");
    
    /* Chiudi socket */
    close(multicast_sock);
    close(unicast_sock);
    
    /* Termina thread (in un'applicazione reale bisognerebbe gestire meglio la terminazione) */
    pthread_cancel(receiver_thread);
    
    return 0;
}