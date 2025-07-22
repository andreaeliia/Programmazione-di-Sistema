/*
 * network_data_collector.c - Sistema multi-thread per raccolta dati di rete
 * 
 * Implementa due thread sincronizzati:
 * - Thread network: accede periodicamente a servizio di rete
 * - Thread writer: salva le risposte ricevute in un file
 * 
 * Sincronizzazione tramite condition variable per notifica
 * immediata della disponibilità di nuovi dati.
 *
 * Uso: ./network_data_collector [interval_sec] [output_file] [service_url]
 * Esempio: ./network_data_collector 5 responses.txt "httpbin.org/uuid"
 *
 * Compatibile Linux/macOS con libreria APUE
 */

#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

/* Costanti di configurazione */
#define MAX_RESPONSE_SIZE 4096
#define MAX_URL_SIZE 256
#define MAX_FILENAME_SIZE 256
#define DEFAULT_INTERVAL 10
#define DEFAULT_OUTPUT_FILE "network_responses.txt"
#define DEFAULT_SERVICE_URL "httpbin.org/uuid"
#define HTTP_PORT 80

/* Struttura per dati condivisi tra thread */
typedef struct {
    char response_data[MAX_RESPONSE_SIZE];
    int data_length;
    int data_ready;                    /* Flag: nuovi dati disponibili */
    int thread_running;                /* Flag: thread attivi */
    pthread_mutex_t mutex;             /* Mutex per accesso esclusivo */
    pthread_cond_t data_available;     /* Condition: dati pronti */
} shared_data_t;

/* Struttura parametri configurazione */
typedef struct {
    int interval_seconds;
    char output_filename[MAX_FILENAME_SIZE];
    char service_url[MAX_URL_SIZE];
} config_t;

/* Variabili globali */
static shared_data_t g_shared_data;
static config_t g_config;
static volatile int g_program_running = 1;

/* Thread handles */
static pthread_t g_network_thread;
static pthread_t g_writer_thread;

/* Contatori per statistiche */
static int g_requests_made = 0;
static int g_responses_saved = 0;

/* Prototipi funzioni */
static void init_shared_data(void);
static void cleanup_shared_data(void);
static int parse_url(const char *url, char *hostname, char *path);
static int connect_to_server(const char *hostname, int port);
static int send_http_request(int sockfd, const char *hostname, const char *path);
static int receive_http_response(int sockfd, char *response, int max_size);
static int fetch_network_data(const char *url, char *response, int max_size);
static void store_response_data(const char *data, int length);
static int get_stored_response(char *buffer, int *length);
static void* network_thread_func(void *arg);
static void* writer_thread_func(void *arg);
static void signal_handler(int sig);
static void setup_signal_handling(void);
static void print_usage(const char *progname);
static int parse_arguments(int argc, char *argv[]);
static void print_statistics(void);

/*
 * Inizializza struttura dati condivisa
 */
static void init_shared_data(void)
{
    memset(&g_shared_data, 0, sizeof(g_shared_data));
    g_shared_data.data_ready = 0;
    g_shared_data.thread_running = 1;
    
    /* Inizializza mutex e condition variable */
    if (pthread_mutex_init(&g_shared_data.mutex, NULL) != 0) {
        err_sys("pthread_mutex_init failed");
    }
    
    if (pthread_cond_init(&g_shared_data.data_available, NULL) != 0) {
        err_sys("pthread_cond_init failed");
    }
    
    printf("Shared data structure initialized\n");
}

/*
 * Cleanup struttura dati condivisa
 */
static void cleanup_shared_data(void)
{
    pthread_mutex_destroy(&g_shared_data.mutex);
    pthread_cond_destroy(&g_shared_data.data_available);
}

/*
 * Parsing URL per estrarre hostname e path
 */
static int parse_url(const char *url, char *hostname, char *path)
{
    const char *start;
    const char *slash_pos;
    int hostname_len;
    
    /* Salta prefisso http:// se presente */
    if (strncmp(url, "http://", 7) == 0) {
        start = url + 7;
    } else {
        start = url;
    }
    
    /* Trova primo slash per separare hostname da path */
    slash_pos = strchr(start, '/');
    
    if (slash_pos != NULL) {
        /* URL con path */
        hostname_len = slash_pos - start;
        strncpy(hostname, start, hostname_len);
        hostname[hostname_len] = '\0';
        strcpy(path, slash_pos);
    } else {
        /* Solo hostname */
        strcpy(hostname, start);
        strcpy(path, "/");
    }
    
    return 0;
}

/*
 * Stabilisce connessione TCP al server
 */
static int connect_to_server(const char *hostname, int port)
{
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *host_entry;
    
    /* Risolvi hostname */
    host_entry = gethostbyname(hostname);
    if (host_entry == NULL) {
        printf("Failed to resolve hostname: %s\n", hostname);
        return -1;
    }
    
    /* Crea socket TCP */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        return -1;
    }
    
    /* Configura indirizzo server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr, host_entry->h_addr_list[0], host_entry->h_length);
    
    /* Connetti */
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connection failed");
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

/*
 * Invia richiesta HTTP GET
 */
static int send_http_request(int sockfd, const char *hostname, const char *path)
{
    char request[1024];
    int request_len;
    ssize_t bytes_sent;
    
    /* Costruisci richiesta HTTP */
    request_len = snprintf(request, sizeof(request),
                          "GET %s HTTP/1.1\r\n"
                          "Host: %s\r\n"
                          "Connection: close\r\n"
                          "User-Agent: NetworkDataCollector/1.0\r\n"
                          "\r\n",
                          path, hostname);
    
    /* Invia richiesta */
    bytes_sent = write(sockfd, request, request_len);
    if (bytes_sent != request_len) {
        perror("failed to send HTTP request");
        return -1;
    }
    
    return 0;
}

/*
 * Riceve risposta HTTP
 */
static int receive_http_response(int sockfd, char *response, int max_size)
{
    char buffer[1024];
    ssize_t bytes_received;
    int total_received = 0;
    char *body_start;
    int body_length;
    
    /* Leggi risposta completa */
    while (total_received < max_size - 1) {
        bytes_received = read(sockfd, buffer, sizeof(buffer));
        if (bytes_received <= 0) {
            break;  /* Fine dati o errore */
        }
        
        if (total_received + bytes_received >= max_size) {
            bytes_received = max_size - total_received - 1;
        }
        
        memcpy(response + total_received, buffer, bytes_received);
        total_received += bytes_received;
    }
    
    response[total_received] = '\0';
    
    /* Trova inizio del body HTTP (dopo \r\n\r\n) */
    body_start = strstr(response, "\r\n\r\n");
    if (body_start != NULL) {
        body_start += 4;  /* Salta \r\n\r\n */
        body_length = strlen(body_start);
        
        /* Sposta body all'inizio del buffer */
        memmove(response, body_start, body_length + 1);
        return body_length;
    }
    
    /* Se non troviamo header, ritorna tutto */
    return total_received;
}

/*
 * Effettua richiesta di rete completa
 */
static int fetch_network_data(const char *url, char *response, int max_size)
{
    char hostname[256];
    char path[256];
    int sockfd;
    int response_len;
    
    /* Parsing URL */
    if (parse_url(url, hostname, path) < 0) {
        return -1;
    }
    
    /* Connetti al server */
    sockfd = connect_to_server(hostname, HTTP_PORT);
    if (sockfd < 0) {
        return -1;
    }
    
    /* Invia richiesta HTTP */
    if (send_http_request(sockfd, hostname, path) < 0) {
        close(sockfd);
        return -1;
    }
    
    /* Ricevi risposta */
    response_len = receive_http_response(sockfd, response, max_size);
    
    close(sockfd);
    
    return response_len;
}

/*
 * Memorizza dati di risposta nella struttura condivisa
 */
static void store_response_data(const char *data, int length)
{
    pthread_mutex_lock(&g_shared_data.mutex);
    
    /* Copia dati nel buffer condiviso */
    if (length > 0 && length < MAX_RESPONSE_SIZE) {
        memcpy(g_shared_data.response_data, data, length);
        g_shared_data.response_data[length] = '\0';
        g_shared_data.data_length = length;
        g_shared_data.data_ready = 1;
        
        /* Notifica al writer thread che ci sono nuovi dati */
        pthread_cond_signal(&g_shared_data.data_available);
    }
    
    pthread_mutex_unlock(&g_shared_data.mutex);
}

/*
 * Preleva dati dalla struttura condivisa
 */
static int get_stored_response(char *buffer, int *length)
{
    int data_available;
    
    pthread_mutex_lock(&g_shared_data.mutex);
    
    /* Attendi disponibilità dati */
    while (!g_shared_data.data_ready && g_shared_data.thread_running) {
        pthread_cond_wait(&g_shared_data.data_available, &g_shared_data.mutex);
    }
    
    data_available = g_shared_data.data_ready;
    
    if (data_available) {
        /* Copia dati nel buffer di output */
        memcpy(buffer, g_shared_data.response_data, g_shared_data.data_length);
        buffer[g_shared_data.data_length] = '\0';
        *length = g_shared_data.data_length;
        
        /* Marca dati come consumati */
        g_shared_data.data_ready = 0;
    }
    
    pthread_mutex_unlock(&g_shared_data.mutex);
    
    return data_available;
}

/*
 * Thread per accesso periodico alla rete
 */
static void* network_thread_func(void *arg)
{
    char response[MAX_RESPONSE_SIZE];
    int response_len;
    time_t current_time;
    struct tm *time_info;
    char timestamp[64];
    
    printf("Network thread started (interval: %d seconds)\n", g_config.interval_seconds);
    
    while (g_program_running && g_shared_data.thread_running) {
        /* Timestamp per logging */
        time(&current_time);
        time_info = localtime(&current_time);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", time_info);
        
        printf("[%s] Making network request to: %s\n", timestamp, g_config.service_url);
        
        /* Effettua richiesta di rete */
        response_len = fetch_network_data(g_config.service_url, response, sizeof(response));
        
        if (response_len > 0) {
            /* Rimuovi newline finali per pulizia */
            while (response_len > 0 && 
                   (response[response_len-1] == '\n' || response[response_len-1] == '\r')) {
                response[response_len-1] = '\0';
                response_len--;
            }
            
            printf("[%s] Received %d bytes: %.100s%s\n", 
                   timestamp, response_len, response, 
                   response_len > 100 ? "..." : "");
            
            /* Memorizza risposta nella struttura condivisa */
            store_response_data(response, response_len);
            g_requests_made++;
        } else {
            printf("[%s] Network request failed\n", timestamp);
        }
        
        /* Attendi intervallo specificato */
        sleep(g_config.interval_seconds);
    }
    
    printf("Network thread terminated\n");
    return NULL;
}

/*
 * Thread per scrittura su file
 */
static void* writer_thread_func(void *arg)
{
    FILE *output_file;
    char data_buffer[MAX_RESPONSE_SIZE];
    int data_length;
    time_t current_time;
    struct tm *time_info;
    char timestamp[64];
    
    printf("Writer thread started (output file: %s)\n", g_config.output_filename);
    
    /* Apri file in modalità append */
    output_file = fopen(g_config.output_filename, "a");
    if (output_file == NULL) {
        printf("Failed to open output file: %s\n", g_config.output_filename);
        return NULL;
    }
    
    /* Scrivi header nel file */
    time(&current_time);
    time_info = localtime(&current_time);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", time_info);
    fprintf(output_file, "\n=== Network Data Collection Session Started: %s ===\n", timestamp);
    fflush(output_file);
    
    while (g_shared_data.thread_running || g_shared_data.data_ready) {
        /* Attendi nuovi dati disponibili */
        if (get_stored_response(data_buffer, &data_length)) {
            /* Timestamp per entry */
            time(&current_time);
            time_info = localtime(&current_time);
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", time_info);
            
            /* Scrivi dati nel file */
            fprintf(output_file, "[%s] %s\n", timestamp, data_buffer);
            fflush(output_file);  /* Forza scrittura immediata */
            
            g_responses_saved++;
            printf("Saved response #%d to file\n", g_responses_saved);
        }
    }
    
    /* Scrivi footer nel file */
    time(&current_time);
    time_info = localtime(&current_time);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", time_info);
    fprintf(output_file, "=== Session Ended: %s (Responses saved: %d) ===\n\n", 
            timestamp, g_responses_saved);
    
    fclose(output_file);
    printf("Writer thread terminated\n");
    return NULL;
}

/*
 * Handler per segnali di terminazione
 */
static void signal_handler(int sig)
{
    printf("\nSignal %d received. Shutting down...\n", sig);
    g_program_running = 0;
    
    /* Notifica terminazione ai thread */
    pthread_mutex_lock(&g_shared_data.mutex);
    g_shared_data.thread_running = 0;
    pthread_cond_signal(&g_shared_data.data_available);
    pthread_mutex_unlock(&g_shared_data.mutex);
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
    
    /* Ignora SIGPIPE per evitare crash su connessioni interrotte */
    signal(SIGPIPE, SIG_IGN);
}

/*
 * Stampa istruzioni d'uso
 */
static void print_usage(const char *progname)
{
    printf("Uso: %s [intervallo] [file_output] [url_servizio]\n", progname);
    printf("\nParametri:\n");
    printf("  intervallo    - Secondi tra richieste di rete (default: %d)\n", DEFAULT_INTERVAL);
    printf("  file_output   - File per salvare risposte (default: %s)\n", DEFAULT_OUTPUT_FILE);
    printf("  url_servizio  - URL servizio di rete (default: %s)\n", DEFAULT_SERVICE_URL);
    printf("\nEsempi:\n");
    printf("  %s                                    # Usa tutti i default\n", progname);
    printf("  %s 5                                  # Intervallo 5 secondi\n", progname);
    printf("  %s 10 responses.txt                   # Intervallo e file custom\n", progname);
    printf("  %s 15 data.txt \"httpbin.org/time\"     # Tutti parametri custom\n", progname);
    printf("\nNote:\n");
    printf("  - I due thread si sincronizzano con condition variable\n");
    printf("  - Le risposte vengono salvate immediatamente quando disponibili\n");
    printf("  - Usa Ctrl+C per terminare il programma\n");
}

/*
 * Parsing argomenti linea di comando
 */
static int parse_arguments(int argc, char *argv[])
{
    /* Valori di default */
    g_config.interval_seconds = DEFAULT_INTERVAL;
    strcpy(g_config.output_filename, DEFAULT_OUTPUT_FILE);
    strcpy(g_config.service_url, DEFAULT_SERVICE_URL);
    
    /* Parse argomenti opzionali */
    if (argc > 1) {
        g_config.interval_seconds = atoi(argv[1]);
        if (g_config.interval_seconds <= 0) {
            printf("Errore: intervallo deve essere positivo\n");
            return -1;
        }
    }
    
    if (argc > 2) {
        strncpy(g_config.output_filename, argv[2], MAX_FILENAME_SIZE - 1);
        g_config.output_filename[MAX_FILENAME_SIZE - 1] = '\0';
    }
    
    if (argc > 3) {
        strncpy(g_config.service_url, argv[3], MAX_URL_SIZE - 1);
        g_config.service_url[MAX_URL_SIZE - 1] = '\0';
    }
    
    if (argc > 4) {
        print_usage(argv[0]);
        return -1;
    }
    
    return 0;
}

/*
 * Stampa statistiche finali
 */
static void print_statistics(void)
{
    printf("\n=== Final Statistics ===\n");
    printf("Network requests made: %d\n", g_requests_made);
    printf("Responses saved to file: %d\n", g_responses_saved);
    printf("Output file: %s\n", g_config.output_filename);
}

/*
 * Funzione principale
 */
int main(int argc, char *argv[])
{
    printf("=== Network Data Collector ===\n");
    printf("Multi-thread system with condition variable synchronization\n\n");
    
    /* Parse argomenti */
    if (parse_arguments(argc, argv) < 0) {
        exit(EXIT_FAILURE);
    }
    
    /* Mostra configurazione */
    printf("Configuration:\n");
    printf("  Request interval: %d seconds\n", g_config.interval_seconds);
    printf("  Output file: %s\n", g_config.output_filename);
    printf("  Service URL: %s\n", g_config.service_url);
    printf("  Synchronization: condition variable\n\n");
    
    /* Inizializza sistema */
    init_shared_data();
    setup_signal_handling();
    
    /* Crea i due thread */
    printf("Starting threads...\n");
    
    if (pthread_create(&g_network_thread, NULL, network_thread_func, NULL) != 0) {
        err_sys("pthread_create network thread failed");
    }
    
    if (pthread_create(&g_writer_thread, NULL, writer_thread_func, NULL) != 0) {
        err_sys("pthread_create writer thread failed");
    }
    
    printf("Both threads started successfully!\n");
    printf("Network thread will fetch data every %d seconds\n", g_config.interval_seconds);
    printf("Writer thread will save responses immediately when available\n");
    printf("Press Ctrl+C to stop...\n\n");
    
    /* Attendi terminazione thread */
    pthread_join(g_network_thread, NULL);
    pthread_join(g_writer_thread, NULL);
    
    /* Stampa statistiche finali */
    print_statistics();
    
    /* Cleanup */
    cleanup_shared_data();
    
    printf("Program terminated successfully.\n");
    return 0;
}