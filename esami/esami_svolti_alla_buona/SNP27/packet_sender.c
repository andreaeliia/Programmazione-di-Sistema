/*
 * packet_sender.c - Processo che invia pacchetti a lunghezza variabile
 * 
 * Genera pacchetti con contenuto casuale di dimensioni variabili e
 * li invia tramite socket UNIX domain al processo receiver.
 * Ogni pacchetto ha un header con la lunghezza totale.
 *
 * Uso: ./packet_sender [num_packets] [delay_ms]
 * Esempio: ./packet_sender 100 50
 *
 * Compatibile Linux/macOS con libreria APUE
 */

#include "apue.h"
#include "packet_protocol.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>

/* Parametri di default */
#define DEFAULT_NUM_PACKETS 50
#define DEFAULT_DELAY_MS 100

/* Prototipi funzioni */
static int create_socket_connection(void);
static void generate_random_packet(packet_t *packet);
static int send_packet(int sockfd, const packet_t *packet);
static void print_packet_info(const packet_t *packet, int packet_num);
static void cleanup_sender(int sockfd);

/*
 * Crea connessione socket UNIX domain al receiver
 */
static int create_socket_connection(void)
{
    int sockfd;
    struct sockaddr_un server_addr;
    
    /* Crea socket UNIX domain */
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        err_sys("socket creation failed");
    }
    
    /* Configura indirizzo server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    /* Connetti al receiver */
    printf("Connecting to receiver at %s...\n", SOCKET_PATH);
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sockfd);
        err_sys("connection to receiver failed");
    }
    
    printf("Connected successfully!\n");
    return sockfd;
}

/*
 * Genera pacchetto con contenuto casuale
 */
static void generate_random_packet(packet_t *packet)
{
    int data_size;
    int i;
    
    /* Genera dimensione casuale del payload */
    data_size = MIN_PACKET_SIZE + (rand() % (MAX_PACKET_SIZE - MIN_PACKET_SIZE - sizeof(packet_header_t)));
    
    /* Imposta header con lunghezza totale */
    packet->header.length = sizeof(packet_header_t) + data_size;
    
    /* Riempie payload con dati casuali */
    for (i = 0; i < data_size; i++) {
        packet->data[i] = (char)(rand() % 256);
    }
}

/*
 * Invia pacchetto tramite socket
 */
static int send_packet(int sockfd, const packet_t *packet)
{
    ssize_t bytes_sent;
    int total_length = packet->header.length;
    
    /* Invia pacchetto completo */
    bytes_sent = write(sockfd, packet, total_length);
    
    if (bytes_sent != total_length) {
        if (bytes_sent < 0) {
            perror("write failed");
        } else {
            printf("Partial write: sent %zd of %d bytes\n", bytes_sent, total_length);
        }
        return -1;
    }
    
    return 0;
}

/*
 * Stampa informazioni pacchetto per debugging
 */
static void print_packet_info(const packet_t *packet, int packet_num)
{
    int payload_size = packet->header.length - sizeof(packet_header_t);
    
    printf("Packet #%d: length=%d bytes (header=%lu + payload=%d)\n",
           packet_num, packet->header.length, 
           (unsigned long)sizeof(packet_header_t), payload_size);
}

/*
 * Cleanup risorse sender
 */
static void cleanup_sender(int sockfd)
{
    if (sockfd >= 0) {
        close(sockfd);
    }
}

/*
 * Funzione principale
 */
int main(int argc, char *argv[])
{
    int sockfd;
    int num_packets = DEFAULT_NUM_PACKETS;
    int delay_ms = DEFAULT_DELAY_MS;
    packet_t packet;
    int i;
    struct timespec delay;
    
    printf("=== Packet Sender ===\n");
    
    /* Parse argomenti */
    if (argc > 1) {
        num_packets = atoi(argv[1]);
        if (num_packets <= 0) {
            num_packets = DEFAULT_NUM_PACKETS;
        }
    }
    
    if (argc > 2) {
        delay_ms = atoi(argv[2]);
        if (delay_ms < 0) {
            delay_ms = DEFAULT_DELAY_MS;
        }
    }
    
    printf("Configuration:\n");
    printf("  Packets to send: %d\n", num_packets);
    printf("  Delay between packets: %d ms\n", delay_ms);
    printf("  Packet size range: %d - %d bytes\n", MIN_PACKET_SIZE, MAX_PACKET_SIZE);
    
    /* Inizializza generatore numeri casuali */
    srand(time(NULL));
    
    /* Connetti al receiver */
    sockfd = create_socket_connection();
    
    /* Configura delay */
    delay.tv_sec = delay_ms / 1000;
    delay.tv_nsec = (delay_ms % 1000) * 1000000;
    
    printf("\nSending packets...\n");
    
    /* Loop invio pacchetti */
    for (i = 1; i <= num_packets; i++) {
        /* Genera pacchetto casuale */
        generate_random_packet(&packet);
        
        /* Stampa info pacchetto */
        print_packet_info(&packet, i);
        
        /* Invia pacchetto */
        if (send_packet(sockfd, &packet) < 0) {
            printf("Failed to send packet #%d\n", i);
            break;
        }
        
        /* Attendi prima del prossimo pacchetto */
        if (i < num_packets && delay_ms > 0) {
            nanosleep(&delay, NULL);
        }
    }
    
    printf("\nSending completed. Sent %d packets.\n", i - 1);
    printf("Closing connection...\n");
    
    /* Cleanup */
    cleanup_sender(sockfd);
    
    return 0;
}