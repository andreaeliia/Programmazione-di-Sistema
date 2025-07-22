/*
 * slave.c - Programma Slave per sistema distribuito con multicast
 * 
 * Riceve messaggi multicast dal master e calcola radici quadrate
 * in un range specifico basato sul proprio ID (K)
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
#include <math.h>

#define MULTICAST_PORT 9999
#define UNICAST_PORT 9998
#define MULTICAST_ADDR "224.0.0.1"  /* Modificare con il proprio IP */
#define MASTER_ADDR "127.0.0.1"     /* Indirizzo del master */

/* ID della macchina slave (0-9) - modificare per ogni istanza */
#define SLAVE_K 0

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

/* Struttura per passare dati ai thread */
typedef struct {
    int N;
    int M;
    int K;
} calc_params_t;

/* Variabili globali per sincronizzazione */
static pthread_mutex_t calc_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t calc_cond = PTHREAD_COND_INITIALIZER;
static int calc_ready = 0;
static calc_params_t calc_params;
static int calc_completed = 0;

/*
 * Crea socket multicast per ricevere messaggi dal master
 */
int create_multicast_receiver_socket(void)
{
    int sockfd;
    struct sockaddr_in servaddr;
    struct ip_mreq mreq;
    int reuse = 1;
    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        err_sys("socket error");
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
        err_sys("setsockopt error");
    
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(MULTICAST_PORT);
    
    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        err_sys("bind error");
    
    /* Unisciti al gruppo multicast */
    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_ADDR);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    
    if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
        err_sys("setsockopt IP_ADD_MEMBERSHIP error");
    
    return sockfd;
}

/*
 * Crea socket unicast per inviare conferme al master
 */
int create_unicast_sender_socket(void)
{
    int sockfd;
    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        err_sys("socket error");
    
    return sockfd;
}

/*
 * Invia messaggio di completamento al master
 */
void send_completion_message(int sockfd, int slave_id, int round)
{
    struct sockaddr_in masteraddr;
    completion_msg_t msg;
    
    bzero(&masteraddr, sizeof(masteraddr));
    masteraddr.sin_family = AF_INET;
    masteraddr.sin_port = htons(UNICAST_PORT);
    inet_pton(AF_INET, MASTER_ADDR, &masteraddr.sin_addr);
    
    msg.slave_id = slave_id;
    msg.round_completed = round;
    
    if (sendto(sockfd, &msg, sizeof(msg), 0, 
              (struct sockaddr *)&masteraddr, sizeof(masteraddr)) < 0)
        err_sys("sendto error");
    
    printf("Inviato completamento round %d al master\n", round);
}

/*
 * Thread per calcolare le radici quadrate
 */
void *calculation_thread(void *arg)
{
    int unicast_sock = *(int *)arg;
    FILE *fp;
    char filename[100];
    long start_range, end_range;
    long h;
    double sqrt_result;
    
    /* Crea nome file per i risultati */
    snprintf(filename, sizeof(filename), "results_slave_%d.txt", SLAVE_K);
    
    while (1) {
        /* Attendi nuovi parametri di calcolo */
        pthread_mutex_lock(&calc_mutex);
        while (!calc_ready) {
            pthread_cond_wait(&calc_cond, &calc_mutex);
        }
        
        /* Copia i parametri localmente */
        int N = calc_params.N;
        int M = calc_params.M;
        int K = calc_params.K;
        calc_ready = 0;  /* Reset per il prossimo round */
        pthread_mutex_unlock(&calc_mutex);
        
        printf("Thread calcolo: inizio round %d (K=%d, M=%d)\n", N, K, M);
        
        /* Calcola il range: M*(N*10 + K) <= H < M*(N*10 + K + 1) */
        start_range = (long)M * (N * 10 + K);
        end_range = (long)M * (N * 10 + K + 1);
        
        printf("Calcolo range: %ld <= H < %ld\n", start_range, end_range);
        
        /* Apri file per scrivere i risultati */
        if ((fp = fopen(filename, "a")) == NULL)
            err_sys("fopen error");
        
        fprintf(fp, "=== Round %d ===\n", N);
        fprintf(fp, "Range: %ld <= H < %ld\n", start_range, end_range);
        
        /* Calcola e salva le radici quadrate */
        for (h = start_range; h < end_range; h++) {
            sqrt_result = sqrt((double)h);
            fprintf(fp, "sqrt(%ld) = %.6f\n", h, sqrt_result);
        }
        
        fprintf(fp, "Calcolo completato per round %d\n\n", N);
        fclose(fp);
        
        printf("Completato calcolo per round %d (%ld numeri processati)\n", 
               N, end_range - start_range);
        
        /* Invia conferma al master */
        send_completion_message(unicast_sock, K, N);
        
        /* Segnala completamento */
        pthread_mutex_lock(&calc_mutex);
        calc_completed = 1;
        pthread_mutex_unlock(&calc_mutex);
    }
    
    return NULL;
}

/*
 * Thread per ricevere messaggi multicast
 */
void *communication_thread(void *arg)
{
    int multicast_sock = *(int *)arg;
    multicast_msg_t msg;
    struct sockaddr_in senderaddr;
    socklen_t senderlen = sizeof(senderaddr);
    
    printf("Thread comunicazione avviato, in attesa di messaggi multicast...\n");
    
    while (1) {
        if (recvfrom(multicast_sock, &msg, sizeof(msg), 0, 
                    (struct sockaddr *)&senderaddr, &senderlen) < 0)
            err_sys("recvfrom error");
        
        printf("Ricevuto messaggio multicast: N=%d, M=%d\n", msg.N, msg.M);
        
        /* Passa i parametri al thread di calcolo */
        pthread_mutex_lock(&calc_mutex);
        
        /* Attendi che il calcolo precedente sia completato */
        while (calc_ready && !calc_completed) {
            pthread_mutex_unlock(&calc_mutex);
            usleep(10000);  /* Attendi 10ms */
            pthread_mutex_lock(&calc_mutex);
        }
        
        calc_params.N = msg.N;
        calc_params.M = msg.M;
        calc_params.K = SLAVE_K;
        calc_ready = 1;
        calc_completed = 0;
        
        pthread_cond_signal(&calc_cond);
        pthread_mutex_unlock(&calc_mutex);
    }
    
    return NULL;
}

int main(void)
{
    int multicast_sock, unicast_sock;
    pthread_t comm_thread, calc_thread;
    
    printf("=== SLAVE %d AVVIATO ===\n", SLAVE_K);
    printf("Multicast: %s:%d\n", MULTICAST_ADDR, MULTICAST_PORT);
    printf("Master: %s:%d\n\n", MASTER_ADDR, UNICAST_PORT);
    
    /* Crea socket */
    multicast_sock = create_multicast_receiver_socket();
    unicast_sock = create_unicast_sender_socket();
    
    /* Avvia thread di comunicazione */
    if (pthread_create(&comm_thread, NULL, communication_thread, &multicast_sock) != 0)
        err_sys("pthread_create communication error");
    
    /* Avvia thread di calcolo */
    if (pthread_create(&calc_thread, NULL, calculation_thread, &unicast_sock) != 0)
        err_sys("pthread_create calculation error");
    
    /* Attendi terminazione thread (mai raggiunto in questo esempio) */
    pthread_join(comm_thread, NULL);
    pthread_join(calc_thread, NULL);
    
    /* Chiudi socket */
    close(multicast_sock);
    close(unicast_sock);
    
    return 0;
}