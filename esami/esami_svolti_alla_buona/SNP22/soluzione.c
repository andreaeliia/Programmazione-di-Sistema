/*
 * SMTP Latency Monitor - Client TCP per monitoraggio prestazioni SMTP
 * 
 * Implementa un client che:
 * - Si connette periodicamente a server SMTP pubblici
 * - Misura latenza di connessione e risposta
 * - Chiude connessioni pulitamente con comando QUIT
 * - Log strutturato delle misure su file
 * - Gestione centralizzata segnali con thread dedicato
 *
 * Uso: ./smtp_monitor <server> <porta> <intervallo_sec> <logfile>
 * Esempio: ./smtp_monitor smtp.gmail.com 587 30 smtp_monitor.log
 *
 * Compatibile Linux/macOS con libreria APUE
 */

#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

/* Costanti configurazione */
#define MAX_HOSTNAME_LEN 256
#define MAX_RESPONSE_LEN 1024
#define MAX_LOGLINE_LEN 512
#define SMTP_TIMEOUT_SEC 10
#define SIGNAL_LOG_FILE "signals.log"

/* Struttura per parametri del monitor */
typedef struct {
    char hostname[MAX_HOSTNAME_LEN];
    int port;
    int interval_sec;
    char logfile[MAX_HOSTNAME_LEN];
    volatile int running;
} monitor_config_t;

/* Struttura per risultati misurazione */
typedef struct {
    double connect_time_ms;
    double response_time_ms;
    int success;
    char error_msg[256];
    time_t timestamp;
} measurement_result_t;

/* Variabili globali */
static monitor_config_t g_config;
static pthread_t signal_thread;
static sigset_t signal_set;

/* Prototipi funzioni */
static double get_time_diff_ms(struct timeval *start, struct timeval *end);
static int resolve_hostname(const char *hostname, struct in_addr *addr);
static int connect_to_smtp_server(const char *hostname, int port, double *connect_time);
static int read_smtp_response(int sockfd, char *response, size_t max_len, double *response_time);
static int send_smtp_quit(int sockfd);
static void log_measurement(const measurement_result_t *result);
static void* signal_handler_thread(void* arg);
static void setup_signal_handling(void);
static measurement_result_t perform_smtp_test(void);
static void print_usage(const char *progname);
static int parse_arguments(int argc, char *argv[]);

/*
 * Calcola differenza di tempo in millisecondi
 */
static double get_time_diff_ms(struct timeval *start, struct timeval *end)
{
    return (end->tv_sec - start->tv_sec) * 1000.0 + 
           (end->tv_usec - start->tv_usec) / 1000.0;
}

/*
 * Risolve hostname in indirizzo IP
 */
static int resolve_hostname(const char *hostname, struct in_addr *addr)
{
    struct hostent *host_entry;
    
    /* Prova prima come indirizzo IP diretto */
    if (inet_aton(hostname, addr) != 0) {
        return 0; /* Successo */
    }
    
    /* Risoluzione DNS */
    host_entry = gethostbyname(hostname);
    if (host_entry == NULL) {
        return -1; /* Errore risoluzione */
    }
    
    memcpy(addr, host_entry->h_addr_list[0], sizeof(struct in_addr));
    return 0;
}

/*
 * Stabilisce connessione TCP al server SMTP
 */
static int connect_to_smtp_server(const char *hostname, int port, double *connect_time)
{
    int sockfd;
    struct sockaddr_in server_addr;
    struct timeval start, end;
    struct in_addr ip_addr;
    
    /* Risolvi hostname */
    if (resolve_hostname(hostname, &ip_addr) < 0) {
        return -1;
    }
    
    /* Crea socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return -1;
    }
    
    /* Prepara indirizzo server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr = ip_addr;
    
    /* Misura tempo di connessione */
    gettimeofday(&start, NULL);
    
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sockfd);
        return -1;
    }
    
    gettimeofday(&end, NULL);
    *connect_time = get_time_diff_ms(&start, &end);
    
    return sockfd;
}

/*
 * Legge risposta dal server SMTP
 */
static int read_smtp_response(int sockfd, char *response, size_t max_len, double *response_time)
{
    struct timeval start, end;
    ssize_t bytes_read;
    fd_set readfds;
    struct timeval timeout;
    int result;
    
    /* Setup timeout per lettura */
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);
    timeout.tv_sec = SMTP_TIMEOUT_SEC;
    timeout.tv_usec = 0;
    
    gettimeofday(&start, NULL);
    
    /* Attendi dati disponibili */
    result = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
    if (result <= 0) {
        return -1; /* Timeout o errore */
    }
    
    /* Leggi risposta */
    bytes_read = read(sockfd, response, max_len - 1);
    if (bytes_read <= 0) {
        return -1;
    }
    
    gettimeofday(&end, NULL);
    *response_time = get_time_diff_ms(&start, &end);
    
    response[bytes_read] = '\0';
    return bytes_read;
}

/*
 * Invia comando QUIT SMTP per chiusura pulita
 */
static int send_smtp_quit(int sockfd)
{
    const char quit_cmd[] = "QUIT\r\n";
    char response[MAX_RESPONSE_LEN];
    ssize_t bytes_sent;
    double dummy_time;
    
    /* Invia comando QUIT */
    bytes_sent = write(sockfd, quit_cmd, strlen(quit_cmd));
    if (bytes_sent != strlen(quit_cmd)) {
        return -1;
    }
    
    /* Leggi risposta conferma (opzionale) */
    read_smtp_response(sockfd, response, sizeof(response), &dummy_time);
    
    return 0;
}

/*
 * Registra misurazione nel file di log
 */
static void log_measurement(const measurement_result_t *result)
{
    FILE *logfile;
    char timestamp_str[64];
    struct tm *tm_info;
    
    /* Converti timestamp in stringa leggibile */
    tm_info = localtime(&result->timestamp);
    strftime(timestamp_str, sizeof(timestamp_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    /* Apri file log in append mode */
    logfile = fopen(g_config.logfile, "a");
    if (logfile == NULL) {
        fprintf(stderr, "Errore apertura log file: %s\n", g_config.logfile);
        return;
    }
    
    /* Scrivi entry di log */
    if (result->success) {
        fprintf(logfile, "[%s] SUCCESS: %s:%d - Connect: %.2fms, Response: %.2fms\n",
                timestamp_str, g_config.hostname, g_config.port,
                result->connect_time_ms, result->response_time_ms);
    } else {
        fprintf(logfile, "[%s] FAILED: %s:%d - Error: %s\n",
                timestamp_str, g_config.hostname, g_config.port,
                result->error_msg);
    }
    
    fclose(logfile);
    
    /* Stampa anche su stdout per monitoraggio real-time */
    if (result->success) {
        printf("[%s] Latency: %.2fms + %.2fms = %.2fms total\n",
               timestamp_str, result->connect_time_ms, result->response_time_ms,
               result->connect_time_ms + result->response_time_ms);
    } else {
        printf("[%s] Test failed: %s\n", timestamp_str, result->error_msg);
    }
}

/*
 * Thread dedicato per gestione segnali
 */
static void* signal_handler_thread(void* arg)
{
    int sig;
    FILE *signal_log;
    char timestamp_str[64];
    struct tm *tm_info;
    time_t now;
    const char *signal_names[] = {
        "Unknown", "SIGHUP", "SIGINT", "SIGQUIT", "SIGILL", "SIGTRAP",
        "SIGABRT", "SIGBUS", "SIGFPE", "SIGKILL", "SIGUSR1", "SIGSEGV",
        "SIGUSR2", "SIGPIPE", "SIGALRM", "SIGTERM", "SIGSTKFLT", "SIGCHLD",
        "SIGCONT", "SIGSTOP", "SIGTSTP", "SIGTTIN", "SIGTTOU", "SIGURG",
        "SIGXCPU", "SIGXFSZ", "SIGVTALRM", "SIGPROF", "SIGWINCH", "SIGIO",
        "SIGPWR", "SIGSYS"
    };
    
    printf("Signal handler thread started\n");
    
    while (g_config.running) {
        /* Attendi segnale */
        if (sigwait(&signal_set, &sig) == 0) {
            time(&now);
            tm_info = localtime(&now);
            strftime(timestamp_str, sizeof(timestamp_str), "%Y-%m-%d %H:%M:%S", tm_info);
            
            /* Log segnale ricevuto */
            signal_log = fopen(SIGNAL_LOG_FILE, "a");
            if (signal_log != NULL) {
                fprintf(signal_log, "[%s] Received signal %d (%s)\n",
                        timestamp_str, sig,
                        (sig > 0 && sig < 32) ? signal_names[sig] : "Unknown");
                fclose(signal_log);
            }
            
            printf("[%s] Signal received: %d (%s)\n",
                   timestamp_str, sig,
                   (sig > 0 && sig < 32) ? signal_names[sig] : "Unknown");
            
            /* Gestisci segnali di terminazione */
            if (sig == SIGINT || sig == SIGTERM || sig == SIGQUIT) {
                printf("Termination signal received, shutting down...\n");
                g_config.running = 0;
                break;
            }
        }
    }
    
    printf("Signal handler thread exiting\n");
    return NULL;
}

/*
 * Configura gestione segnali con thread dedicato
 */
static void setup_signal_handling(void)
{
    /* Blocca tutti i segnali per il thread principale */
    sigfillset(&signal_set);
    pthread_sigmask(SIG_BLOCK, &signal_set, NULL);
    
    /* Crea thread dedicato per gestione segnali */
    if (pthread_create(&signal_thread, NULL, signal_handler_thread, NULL) != 0) {
        err_sys("pthread_create signal thread failed");
    }
}

/*
 * Esegue test completo di connettività SMTP
 */
static measurement_result_t perform_smtp_test(void)
{
    measurement_result_t result;
    int sockfd;
    char response[MAX_RESPONSE_LEN];
    
    /* Inizializza risultato */
    memset(&result, 0, sizeof(result));
    time(&result.timestamp);
    result.success = 0;
    
    /* Stabilisci connessione */
    sockfd = connect_to_smtp_server(g_config.hostname, g_config.port, 
                                   &result.connect_time_ms);
    if (sockfd < 0) {
        snprintf(result.error_msg, sizeof(result.error_msg), 
                "Connection failed to %s:%d", g_config.hostname, g_config.port);
        return result;
    }
    
    /* Leggi risposta iniziale del server */
    if (read_smtp_response(sockfd, response, sizeof(response), 
                          &result.response_time_ms) < 0) {
        snprintf(result.error_msg, sizeof(result.error_msg), 
                "Failed to read server response");
        close(sockfd);
        return result;
    }
    
    /* Verifica che sia una risposta SMTP valida (codice 220) */
    if (strncmp(response, "220", 3) != 0) {
        snprintf(result.error_msg, sizeof(result.error_msg), 
                "Invalid SMTP response: %.50s", response);
        close(sockfd);
        return result;
    }
    
    /* Invia comando QUIT per chiusura pulita */
    send_smtp_quit(sockfd);
    close(sockfd);
    
    result.success = 1;
    return result;
}

/*
 * Stampa istruzioni d'uso
 */
static void print_usage(const char *progname)
{
    printf("Uso: %s <hostname> <porta> <intervallo_sec> <logfile>\n", progname);
    printf("\nParametri:\n");
    printf("  hostname      - Server SMTP da testare (es: smtp.gmail.com)\n");
    printf("  porta         - Porta SMTP (es: 25, 587, 465)\n");
    printf("  intervallo_sec- Secondi tra test successivi\n");
    printf("  logfile       - File per log delle misurazioni\n");
    printf("\nEsempio:\n");
    printf("  %s smtp.gmail.com 587 30 smtp_monitor.log\n", progname);
    printf("\nNote:\n");
    printf("  - I segnali vengono loggati in: %s\n", SIGNAL_LOG_FILE);
    printf("  - Usa Ctrl+C per terminare il programma\n");
}

/*
 * Parsing e validazione argomenti linea di comando
 */
static int parse_arguments(int argc, char *argv[])
{
    if (argc != 5) {
        print_usage(argv[0]);
        return -1;
    }
    
    /* Copia hostname */
    strncpy(g_config.hostname, argv[1], MAX_HOSTNAME_LEN - 1);
    g_config.hostname[MAX_HOSTNAME_LEN - 1] = '\0';
    
    /* Parsing porta */
    g_config.port = atoi(argv[2]);
    if (g_config.port <= 0 || g_config.port > 65535) {
        fprintf(stderr, "Errore: porta deve essere tra 1 e 65535\n");
        return -1;
    }
    
    /* Parsing intervallo */
    g_config.interval_sec = atoi(argv[3]);
    if (g_config.interval_sec <= 0) {
        fprintf(stderr, "Errore: intervallo deve essere positivo\n");
        return -1;
    }
    
    /* Copia nome file log */
    strncpy(g_config.logfile, argv[4], MAX_HOSTNAME_LEN - 1);
    g_config.logfile[MAX_HOSTNAME_LEN - 1] = '\0';
    
    g_config.running = 1;
    return 0;
}

/*
 * Funzione principale
 */
int main(int argc, char *argv[])
{
    measurement_result_t result;
    int test_count = 0;
    
    printf("=== SMTP Latency Monitor ===\n");
    printf("Monitoring SMTP server connectivity and response times\n\n");
    
    /* Parse argomenti */
    if (parse_arguments(argc, argv) < 0) {
        exit(EXIT_FAILURE);
    }
    
    /* Setup gestione segnali */
    setup_signal_handling();
    
    printf("Target: %s:%d\n", g_config.hostname, g_config.port);
    printf("Interval: %d seconds\n", g_config.interval_sec);
    printf("Log file: %s\n", g_config.logfile);
    printf("Signal log: %s\n", SIGNAL_LOG_FILE);
    printf("Starting monitoring... (Ctrl+C to stop)\n\n");
    
    /* Log header nel file */
    {
        FILE *logfile = fopen(g_config.logfile, "a");
        if (logfile != NULL) {
            fprintf(logfile, "\n=== SMTP Monitor Started ===\n");
            fprintf(logfile, "Target: %s:%d, Interval: %ds\n", 
                    g_config.hostname, g_config.port, g_config.interval_sec);
            fclose(logfile);
        }
    }
    
    /* Loop principale di monitoraggio */
    while (g_config.running) {
        test_count++;
        printf("Test #%d: ", test_count);
        fflush(stdout);
        
        /* Esegui test */
        result = perform_smtp_test();
        
        /* Log risultato */
        log_measurement(&result);
        
        /* Attendi intervallo specificato */
        if (g_config.running) {
            sleep(g_config.interval_sec);
        }
    }
    
    /* Attendi terminazione thread segnali */
    printf("\nShutting down...\n");
    pthread_join(signal_thread, NULL);
    
    /* Log finale */
    {
        FILE *logfile = fopen(g_config.logfile, "a");
        if (logfile != NULL) {
            fprintf(logfile, "=== SMTP Monitor Stopped (Total tests: %d) ===\n\n", test_count);
            fclose(logfile);
        }
    }
    
    printf("Monitor stopped. Total tests performed: %d\n", test_count);
    return 0;
}