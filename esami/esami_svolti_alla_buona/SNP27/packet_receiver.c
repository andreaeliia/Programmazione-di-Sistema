/*
 * packet_receiver.c - Processo con coda circolare thread-safe
 * 
 * Riceve pacchetti a lunghezza variabile e li gestisce con 3 thread:
 * - Thread receiver: riceve pacchetti e li inserisce in coda
 * - Thread consumer 1: rimuove pacchetti dalla coda
 * - Thread consumer 2: rimuove pacchetti dalla coda
 * 
 * La coda è implementata come buffer circolare con disciplina FIFO.
 *
 * Uso: ./packet_receiver
 *
 * Compatibile Linux/macOS con libreria APUE
 */

#include "apue.h"
#include "packet_protocol.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <signal.h>

/* Variabili globali */
static circular_queue_t g_packet_queue;
static volatile int g_running = 1;
static int g_server_socket = -1;
static int g_client_socket = -1;

/* Contatori per statistiche */
static int g_packets_received = 0;
static int g_packets_processed_1 = 0;
static int g_packets_processed_2 = 0;

/* Thread handles */
static pthread_t g_receiver_thread;
static pthread_t g_consumer1_thread;
static pthread_t g_consumer2_thread;

/* Prototipi funzioni */
static void init_circular_queue(circular_queue_t *queue);
static void destroy_circular_queue(circular_queue_t *queue);
static int queue_put(circular_queue_t *queue, const packet_t *packet);
static int queue_get(circular_queue_t *queue, packet_t *packet);
static void print_queue_status(const circular_queue_t *queue);

static int setup_server_socket(void);
static int accept_client_connection(int server_sock);
static int receive_packet(int sockfd, packet_t *packet);

static void* receiver_thread_func(void* arg);
static void* consumer_thread_func(void* arg);

static void signal_handler(int sig);
static void setup_signal_handling(void);
static void cleanup_receiver(void);
static void print_statistics(void);

/*
 * Inizializza coda circolare thread-safe
 */
static void init_circular_queue(circular_queue_t *queue)
{
    int i;
    
    /* Inizializza struttura */
    memset(queue, 0, sizeof(circular_queue_t));
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    queue->shutdown = 0;
    
    /* Marca tutti gli slot come liberi */
    for (i = 0; i < QUEUE_MAX_SIZE; i++) {
        queue->buffer[i].used = 0;
    }
    
    /* Inizializza mutex e condition variables */
    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        err_sys("pthread_mutex_init failed");
    }
    
    if (pthread_cond_init(&queue->not_empty, NULL) != 0) {
        err_sys("pthread_cond_init not_empty failed");
    }
    
    if (pthread_cond_init(&queue->not_full, NULL) != 0) {
        err_sys("pthread_cond_init not_full failed");
    }
    
    printf("Circular queue initialized (capacity: %d)\n", QUEUE_MAX_SIZE);
}

/*
 * Distrugge coda circolare e libera risorse
 */
static void destroy_circular_queue(circular_queue_t *queue)
{
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);
}

/*
 * Inserisce pacchetto in coda (blocking se piena)
 */
static int queue_put(circular_queue_t *queue, const packet_t *packet)
{
    pthread_mutex_lock(&queue->mutex);
    
    /* Attendi spazio disponibile */
    while (queue->count >= QUEUE_MAX_SIZE && !queue->shutdown) {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }
    
    if (queue->shutdown) {
        pthread_mutex_unlock(&queue->mutex);
        return -1;  /* Terminazione richiesta */
    }
    
    /* Inserisce pacchetto */
    queue->buffer[queue->tail].packet = *packet;
    queue->buffer[queue->tail].used = 1;
    queue->tail = (queue->tail + 1) % QUEUE_MAX_SIZE;
    queue->count++;
    
    /* Segnala ai consumer che c'è un nuovo elemento */
    pthread_cond_signal(&queue->not_empty);
    
    pthread_mutex_unlock(&queue->mutex);
    return 0;
}

/*
 * Rimuove pacchetto dalla coda (blocking se vuota)
 */
static int queue_get(circular_queue_t *queue, packet_t *packet)
{
    pthread_mutex_lock(&queue->mutex);
    
    /* Attendi elemento disponibile */
    while (queue->count == 0 && !queue->shutdown) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }
    
    if (queue->shutdown && queue->count == 0) {
        pthread_mutex_unlock(&queue->mutex);
        return -1;  /* Coda vuota e terminazione richiesta */
    }
    
    /* Rimuove pacchetto */
    *packet = queue->buffer[queue->head].packet;
    queue->buffer[queue->head].used = 0;
    queue->head = (queue->head + 1) % QUEUE_MAX_SIZE;
    queue->count--;
    
    /* Segnala al receiver che c'è spazio */
    pthread_cond_signal(&queue->not_full);
    
    pthread_mutex_unlock(&queue->mutex);
    return 0;
}

/*
 * Stampa stato attuale della coda
 */
static void print_queue_status(const circular_queue_t *queue)
{
    pthread_mutex_lock((pthread_mutex_t*)&queue->mutex);
    printf("Queue status: %d/%d packets (head=%d, tail=%d)\n",
           queue->count, QUEUE_MAX_SIZE, queue->head, queue->tail);
    pthread_mutex_unlock((pthread_mutex_t*)&queue->mutex);
}

/*
 * Configura server socket UNIX domain
 */
static int setup_server_socket(void)
{
    int sockfd;
    struct sockaddr_un server_addr;
    
    /* Rimuovi socket esistente */
    unlink(SOCKET_PATH);
    
    /* Crea socket UNIX domain */
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        err_sys("socket creation failed");
    }
    
    /* Configura indirizzo */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    /* Bind socket */
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sockfd);
        err_sys("bind failed");
    }
    
    /* Listen per connessioni */
    if (listen(sockfd, 1) < 0) {
        close(sockfd);
        err_sys("listen failed");
    }
    
    printf("Server socket listening on %s\n", SOCKET_PATH);
    return sockfd;
}

/*
 * Accetta connessione da client
 */
static int accept_client_connection(int server_sock)
{
    int client_sock;
    struct sockaddr_un client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    printf("Waiting for client connection...\n");
    
    client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
    if (client_sock < 0) {
        err_sys("accept failed");
    }
    
    printf("Client connected!\n");
    return client_sock;
}

/*
 * Riceve un pacchetto completo dal socket
 */
static int receive_packet(int sockfd, packet_t *packet)
{
    ssize_t bytes_read;
    int remaining_bytes;
    char *buffer_ptr;
    
    /* Leggi header per ottenere lunghezza */
    bytes_read = read(sockfd, &packet->header, sizeof(packet_header_t));
    if (bytes_read != sizeof(packet_header_t)) {
        if (bytes_read == 0) {
            return 0;  /* Client disconnesso */
        }
        return -1;  /* Errore */
    }
    
    /* Verifica lunghezza valida */
    if (packet->header.length < sizeof(packet_header_t) || 
        packet->header.length > MAX_PACKET_SIZE) {
        printf("Invalid packet length: %d\n", packet->header.length);
        return -1;
    }
    
    /* Leggi resto del pacchetto */
    remaining_bytes = packet->header.length - sizeof(packet_header_t);
    buffer_ptr = packet->data;
    
    while (remaining_bytes > 0) {
        bytes_read = read(sockfd, buffer_ptr, remaining_bytes);
        if (bytes_read <= 0) {
            return -1;  /* Errore o disconnessione */
        }
        
        buffer_ptr += bytes_read;
        remaining_bytes -= bytes_read;
    }
    
    return packet->header.length;
}

/*
 * Thread che riceve pacchetti e li inserisce in coda
 */
static void* receiver_thread_func(void* arg)
{
    packet_t packet;
    int result;
    
    printf("Receiver thread started\n");
    
    while (g_running) {
        /* Ricevi pacchetto */
        result = receive_packet(g_client_socket, &packet);
        
        if (result == 0) {
            printf("Client disconnected\n");
            break;
        } else if (result < 0) {
            if (g_running) {
                printf("Error receiving packet\n");
            }
            break;
        }
        
        /* Inserisci in coda */
        if (queue_put(&g_packet_queue, &packet) == 0) {
            g_packets_received++;
            printf("Received packet #%d (length: %d bytes)\n", 
                   g_packets_received, packet.header.length);
        }
    }
    
    printf("Receiver thread terminated\n");
    return NULL;
}

/*
 * Thread consumer che rimuove pacchetti dalla coda
 */
static void* consumer_thread_func(void* arg)
{
    consumer_thread_params_t *params = (consumer_thread_params_t*)arg;
    packet_t packet;
    int payload_size;
    
    printf("Consumer thread %d started\n", params->thread_id);
    
    while (g_running || g_packet_queue.count > 0) {
        /* Rimuovi pacchetto dalla coda */
        if (queue_get(params->queue, &packet) == 0) {
            (*params->packets_processed)++;
            payload_size = packet.header.length - sizeof(packet_header_t);
            
            printf("Consumer %d processed packet #%d (payload: %d bytes)\n",
                   params->thread_id, *params->packets_processed, payload_size);
            
            /* Simula elaborazione */
            usleep(10000);  /* 10ms */
        }
    }
    
    printf("Consumer thread %d terminated\n", params->thread_id);
    return NULL;
}

/*
 * Handler per segnali di terminazione
 */
static void signal_handler(int sig)
{
    printf("\nSignal %d received. Shutting down...\n", sig);
    g_running = 0;
    
    /* Notifica terminazione alla coda */
    pthread_mutex_lock(&g_packet_queue.mutex);
    g_packet_queue.shutdown = 1;
    pthread_cond_broadcast(&g_packet_queue.not_empty);
    pthread_cond_broadcast(&g_packet_queue.not_full);
    pthread_mutex_unlock(&g_packet_queue.mutex);
}

/*
 * Configura gestione segnali
 */
static void setup_signal_handling(void)
{
    struct sigaction sa;
    
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGINT, &sa, NULL);   /* Ctrl+C */
    sigaction(SIGTERM, &sa, NULL);  /* Terminazione */
    
    /* Ignora SIGPIPE per evitare crash su client disconnessi */
    signal(SIGPIPE, SIG_IGN);
}

/*
 * Stampa statistiche finali
 */
static void print_statistics(void)
{
    printf("\n=== Final Statistics ===\n");
    printf("Packets received: %d\n", g_packets_received);
    printf("Packets processed by consumer 1: %d\n", g_packets_processed_1);
    printf("Packets processed by consumer 2: %d\n", g_packets_processed_2);
    printf("Total packets processed: %d\n", 
           g_packets_processed_1 + g_packets_processed_2);
    print_queue_status(&g_packet_queue);
}

/*
 * Cleanup risorse receiver
 */
static void cleanup_receiver(void)
{
    /* Chiudi socket */
    if (g_client_socket >= 0) {
        close(g_client_socket);
    }
    if (g_server_socket >= 0) {
        close(g_server_socket);
        unlink(SOCKET_PATH);
    }
    
    /* Distruggi coda */
    destroy_circular_queue(&g_packet_queue);
}

/*
 * Funzione principale
 */
int main(void)
{
    consumer_thread_params_t consumer1_params;
    consumer_thread_params_t consumer2_params;
    
    printf("=== Packet Receiver with Circular Queue ===\n");
    printf("Queue capacity: %d packets\n", QUEUE_MAX_SIZE);
    printf("Packet size range: %d - %d bytes\n", MIN_PACKET_SIZE, MAX_PACKET_SIZE);
    
    /* Inizializza sistema */
    init_circular_queue(&g_packet_queue);
    setup_signal_handling();
    
    /* Configura server socket */
    g_server_socket = setup_server_socket();
    
    /* Accetta connessione client */
    g_client_socket = accept_client_connection(g_server_socket);
    
    /* Configura parametri consumer thread */
    consumer1_params.queue = &g_packet_queue;
    consumer1_params.thread_id = 1;
    consumer1_params.packets_processed = &g_packets_processed_1;
    
    consumer2_params.queue = &g_packet_queue;
    consumer2_params.thread_id = 2;
    consumer2_params.packets_processed = &g_packets_processed_2;
    
    printf("\nStarting threads...\n");
    
    /* Crea thread */
    if (pthread_create(&g_receiver_thread, NULL, receiver_thread_func, NULL) != 0) {
        err_sys("pthread_create receiver failed");
    }
    
    if (pthread_create(&g_consumer1_thread, NULL, consumer_thread_func, &consumer1_params) != 0) {
        err_sys("pthread_create consumer1 failed");
    }
    
    if (pthread_create(&g_consumer2_thread, NULL, consumer_thread_func, &consumer2_params) != 0) {
        err_sys("pthread_create consumer2 failed");
    }
    
    printf("All threads started successfully!\n");
    printf("Receiver ready. Use Ctrl+C to stop.\n\n");
    
    /* Loop principale - stampa stato periodicamente */
    while (g_running) {
        sleep(5);
        if (g_running) {
            print_queue_status(&g_packet_queue);
        }
    }
    
    /* Attendi terminazione thread */
    printf("\nWaiting for threads to terminate...\n");
    pthread_join(g_receiver_thread, NULL);
    pthread_join(g_consumer1_thread, NULL);
    pthread_join(g_consumer2_thread, NULL);
    
    /* Stampa statistiche finali */
    print_statistics();
    
    /* Cleanup */
    cleanup_receiver();
    
    printf("Receiver shutdown complete.\n");
    return 0;
}