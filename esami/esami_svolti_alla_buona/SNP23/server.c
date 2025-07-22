/*
 * Server TCP Multi-Thread con Vettore Condiviso
 * 
 * Implementa un server che:
 * - Gestisce simultaneamente 3 porte TCP diverse
 * - Utilizza un thread dedicato per ogni porta
 * - Mantiene un vettore condiviso di interi thread-safe
 * - Gestisce concorrenza con mutex per evitare race conditions
 * - Salva interi ricevuti dai client nel primo slot libero
 *
 * Uso: ./tcp_server <porta1> <porta2> <porta3>
 * Esempio: ./tcp_server 8001 8002 8003
 *
 * Compatibile Linux/macOS con libreria APUE
 */

#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

/* Costanti configurazione */
#define MAX_VECTOR_SIZE 1000        /* Dimensione massima vettore */
#define MAX_CLIENTS_PER_PORT 10     /* Client simultanei per porta */
#define LISTEN_BACKLOG 5            /* Coda listen per socket */
#define BUFFER_SIZE 64              /* Buffer per ricezione dati */

/* Valore speciale per elementi liberi */
#define EMPTY_SLOT -1

/* Struttura per vettore condiviso thread-safe */
typedef struct {
    int data[MAX_VECTOR_SIZE];      /* Vettore di interi */
    int size;                       /* Numero elementi attualmente nel vettore */
    pthread_mutex_t mutex;          /* Mutex per accesso esclusivo */
} shared_vector_t;

/* Struttura per parametri thread porta */
typedef struct {
    int port;                       /* Porta TCP da gestire */
    int thread_id;                  /* ID thread (0, 1, 2) */
    shared_vector_t *vector;        /* Puntatore al vettore condiviso */
} thread_params_t;

/* Variabili globali */
static shared_vector_t g_shared_vector;
static volatile int g_server_running = 1;
static pthread_t g_port_threads[3];

/* Prototipi funzioni */
static void init_shared_vector(shared_vector_t *vector);
static int add_to_vector(shared_vector_t *vector, int value, int thread_id);
static void print_vector_status(shared_vector_t *vector);
static int create_tcp_socket(int port);
static void* port_handler_thread(void* arg);
static void handle_client_connection(int client_fd, int thread_id, shared_vector_t *vector);
static void signal_handler(int sig);
static void setup_signal_handling(void);
static void print_usage(const char *progname);
static int parse_ports(int argc, char *argv[], int ports[3]);

/*
 * Inizializza il vettore condiviso
 */
static void init_shared_vector(shared_vector_t *vector)
{
    int i;
    
    /* Inizializza tutti gli elementi come liberi */
    for (i = 0; i < MAX_VECTOR_SIZE; i++) {
        vector->data[i] = EMPTY_SLOT;
    }
    
    vector->size = 0;
    
    /* Inizializza mutex */
    if (pthread_mutex_init(&vector->mutex, NULL) != 0) {
        err_sys("pthread_mutex_init failed");
    }
    
    printf("Shared vector initialized (capacity: %d)\n", MAX_VECTOR_SIZE);
}

/*
 * Aggiunge un valore al primo slot libero del vettore
 * Ritorna: 0 = successo, -1 = vettore pieno
 */
static int add_to_vector(shared_vector_t *vector, int value, int thread_id)
{
    int i;
    int slot_found = -1;
    
    /* Acquisisce lock esclusivo */
    pthread_mutex_lock(&vector->mutex);
    
    /* Cerca primo slot libero */
    for (i = 0; i < MAX_VECTOR_SIZE; i++) {
        if (vector->data[i] == EMPTY_SLOT) {
            slot_found = i;
            break;
        }
    }
    
    if (slot_found != -1) {
        /* Slot libero trovato - inserisce valore */
        vector->data[slot_found] = value;
        vector->size++;
        
        printf("Thread %d: Added value %d at position %d (total: %d/%d)\n",
               thread_id, value, slot_found, vector->size, MAX_VECTOR_SIZE);
        
        pthread_mutex_unlock(&vector->mutex);
        return 0;
    } else {
        /* Vettore pieno */
        printf("Thread %d: Vector full! Cannot add value %d\n", thread_id, value);
        pthread_mutex_unlock(&vector->mutex);
        return -1;
    }
}

/*
 * Stampa stato attuale del vettore (per debugging)
 */
static void print_vector_status(shared_vector_t *vector)
{
    int i;
    int count = 0;
    
    pthread_mutex_lock(&vector->mutex);
    
    printf("\n=== Vector Status ===\n");
    printf("Used slots: %d/%d\n", vector->size, MAX_VECTOR_SIZE);
    printf("Values: ");
    
    for (i = 0; i < MAX_VECTOR_SIZE && count < 20; i++) {
        if (vector->data[i] != EMPTY_SLOT) {
            printf("[%d]=%d ", i, vector->data[i]);
            count++;
        }
    }
    
    if (vector->size > 20) {
        printf("... (showing first 20)");
    }
    printf("\n=====================\n\n");
    
    pthread_mutex_unlock(&vector->mutex);
}

/*
 * Crea e configura socket TCP per una porta specifica
 */
static int create_tcp_socket(int port)
{
    int sockfd;
    struct sockaddr_in server_addr;
    int opt = 1;
    
    /* Crea socket TCP */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        err_sys("socket creation failed");
    }
    
    /* Abilita riuso indirizzo per evitare "Address already in use" */
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        err_sys("setsockopt SO_REUSEADDR failed");
    }
    
    /* Configura indirizzo server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  /* Accetta da qualsiasi interfaccia */
    server_addr.sin_port = htons(port);
    
    /* Bind socket alla porta */
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sockfd);
        err_sys("bind failed");
    }
    
    /* Mette socket in modalità listen */
    if (listen(sockfd, LISTEN_BACKLOG) < 0) {
        close(sockfd);
        err_sys("listen failed");
    }
    
    printf("TCP socket created and listening on port %d\n", port);
    return sockfd;
}

/*
 * Gestisce singola connessione client
 */
static void handle_client_connection(int client_fd, int thread_id, shared_vector_t *vector)
{
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    int received_value;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    /* Ottieni informazioni client */
    if (getpeername(client_fd, (struct sockaddr*)&client_addr, &addr_len) == 0) {
        printf("Thread %d: Client connected from %s:%d\n",
               thread_id, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    }
    
    /* Ricevi dati dal client */
    bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';  /* Termina stringa */
        
        /* Converte stringa ricevuta in intero */
        received_value = atoi(buffer);
        
        printf("Thread %d: Received value: %d\n", thread_id, received_value);
        
        /* Aggiunge valore al vettore condiviso */
        if (add_to_vector(vector, received_value, thread_id) == 0) {
            /* Successo - invia conferma al client */
            const char *success_msg = "OK: Value added to vector\n";
            send(client_fd, success_msg, strlen(success_msg), 0);
        } else {
            /* Errore - vettore pieno */
            const char *error_msg = "ERROR: Vector is full\n";
            send(client_fd, error_msg, strlen(error_msg), 0);
        }
        
    } else if (bytes_received == 0) {
        printf("Thread %d: Client disconnected\n", thread_id);
    } else {
        printf("Thread %d: recv() error\n", thread_id);
    }
    
    close(client_fd);
}

/*
 * Thread handler per gestione di una porta TCP
 */
static void* port_handler_thread(void* arg)
{
    thread_params_t *params = (thread_params_t*)arg;
    int server_fd, client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    
    printf("Thread %d starting on port %d\n", params->thread_id, params->port);
    
    /* Crea socket per questa porta */
    server_fd = create_tcp_socket(params->port);
    
    /* Loop principale per accettare connessioni */
    while (g_server_running) {
        client_addr_len = sizeof(client_addr);
        
        /* Accetta connessione client */
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
        
        if (client_fd < 0) {
            if (g_server_running) {
                /* Solo errore se server ancora attivo */
                printf("Thread %d: accept() failed\n", params->thread_id);
            }
            continue;
        }
        
        /* Gestisce connessione client */
        handle_client_connection(client_fd, params->thread_id, params->vector);
    }
    
    close(server_fd);
    printf("Thread %d: Stopped listening on port %d\n", params->thread_id, params->port);
    return NULL;
}

/*
 * Handler per segnali di terminazione
 */
static void signal_handler(int sig)
{
    printf("\nSignal %d received. Shutting down server...\n", sig);
    g_server_running = 0;
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
 * Stampa istruzioni d'uso
 */
static void print_usage(const char *progname)
{
    printf("Uso: %s <porta1> <porta2> <porta3>\n", progname);
    printf("\nParametri:\n");
    printf("  porta1, porta2, porta3 - Tre porte TCP diverse (1024-65535)\n");
    printf("\nEsempio:\n");
    printf("  %s 8001 8002 8003\n", progname);
    printf("\nNote:\n");
    printf("  - Ogni porta gestita da thread separato\n");
    printf("  - Vettore condiviso di %d elementi\n", MAX_VECTOR_SIZE);
    printf("  - Client inviano interi che vengono salvati nel vettore\n");
    printf("  - Usa Ctrl+C per terminare il server\n");
}

/*
 * Parsing e validazione porte dalla linea di comando
 */
static int parse_ports(int argc, char *argv[], int ports[3])
{
    int i;
    
    if (argc != 4) {
        print_usage(argv[0]);
        return -1;
    }
    
    for (i = 0; i < 3; i++) {
        ports[i] = atoi(argv[i + 1]);
        
        /* Validazione range porte */
        if (ports[i] < 1024 || ports[i] > 65535) {
            fprintf(stderr, "Errore: porta %d fuori range (1024-65535)\n", ports[i]);
            return -1;
        }
        
        /* Verifica porte non duplicate */
        {
            int j;
            for (j = 0; j < i; j++) {
                if (ports[j] == ports[i]) {
                    fprintf(stderr, "Errore: porta %d duplicata\n", ports[i]);
                    return -1;
                }
            }
        }
    }
    
    return 0;
}

/*
 * Funzione principale
 */
int main(int argc, char *argv[])
{
    int ports[3];
    thread_params_t thread_params[3];
    int i;
    
    printf("=== TCP Multi-Thread Server ===\n");
    printf("Server with 3 ports and shared vector\n\n");
    
    /* Parse e valida argomenti */
    if (parse_ports(argc, argv, ports) < 0) {
        exit(EXIT_FAILURE);
    }
    
    /* Setup gestione segnali */
    setup_signal_handling();
    
    /* Inizializza vettore condiviso */
    init_shared_vector(&g_shared_vector);
    
    printf("Starting server on ports: %d, %d, %d\n", ports[0], ports[1], ports[2]);
    printf("Shared vector capacity: %d integers\n", MAX_VECTOR_SIZE);
    printf("Press Ctrl+C to stop server\n\n");
    
    /* Crea thread per ogni porta */
    for (i = 0; i < 3; i++) {
        thread_params[i].port = ports[i];
        thread_params[i].thread_id = i;
        thread_params[i].vector = &g_shared_vector;
        
        if (pthread_create(&g_port_threads[i], NULL, port_handler_thread, 
                          &thread_params[i]) != 0) {
            err_sys("pthread_create failed");
        }
    }
    
    printf("All threads started successfully!\n\n");
    
    /* Loop principale - stampa stato periodicamente */
    while (g_server_running) {
        sleep(10);  /* Stampa stato ogni 10 secondi */
        if (g_server_running) {
            print_vector_status(&g_shared_vector);
        }
    }
    
    /* Terminazione - attendi chiusura thread */
    printf("\nWaiting for threads to terminate...\n");
    for (i = 0; i < 3; i++) {
        pthread_join(g_port_threads[i], NULL);
    }
    
    /* Stampa stato finale */
    printf("\nFinal vector status:\n");
    print_vector_status(&g_shared_vector);
    
    /* Cleanup mutex */
    pthread_mutex_destroy(&g_shared_vector.mutex);
    
    printf("Server shutdown complete.\n");
    return 0;
}