/*
 * Server concorrente con thread pool e coda FIFO
 * Implementazione in C90 compatibile con Linux e macOS
 */

#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#define MAX_CLIENTS 10
#define WORKER_THREADS 5
#define PORT 8080
#define BUFFER_SIZE 256

/* Struttura per rappresentare una richiesta nella coda */
typedef struct request {
    int client_fd;
    char data[BUFFER_SIZE];
    struct request *next;
} request_t;

/* Struttura per la coda FIFO thread-safe */
typedef struct {
    request_t *head;
    request_t *tail;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    int count;
} request_queue_t;

/* Variabili globali */
static request_queue_t request_queue;
static int server_running = 1;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Funzioni di utilita per la coda FIFO */
void init_queue(request_queue_t *queue);
void enqueue_request(request_queue_t *queue, int client_fd, const char *data);
request_t* dequeue_request(request_queue_t *queue);
void destroy_queue(request_queue_t *queue);

/* Funzioni del server */
void* worker_thread(void* arg);
void* client_handler(void* arg);
void log_message(const char* message);
int create_server_socket(void);
double random_time(void);

/*
 * Inizializza la coda delle richieste
 */
void init_queue(request_queue_t *queue) {
    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->not_empty, NULL);
}

/*
 * Aggiunge una richiesta alla coda (thread-safe)
 */
void enqueue_request(request_queue_t *queue, int client_fd, const char *data) {
    request_t *new_request;
    
    new_request = (request_t*)malloc(sizeof(request_t));
    if (new_request == NULL) {
        err_sys("malloc failed");
    }
    
    new_request->client_fd = client_fd;
    strncpy(new_request->data, data, BUFFER_SIZE - 1);
    new_request->data[BUFFER_SIZE - 1] = '\0';
    new_request->next = NULL;
    
    pthread_mutex_lock(&queue->mutex);
    
    if (queue->tail == NULL) {
        queue->head = queue->tail = new_request;
    } else {
        queue->tail->next = new_request;
        queue->tail = new_request;
    }
    
    queue->count++;
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
    
    log_message("Richiesta aggiunta alla coda");
}

/*
 * Rimuove una richiesta dalla coda (thread-safe)
 */
request_t* dequeue_request(request_queue_t *queue) {
    request_t *request;
    
    pthread_mutex_lock(&queue->mutex);
    
    while (queue->head == NULL && server_running) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }
    
    if (!server_running) {
        pthread_mutex_unlock(&queue->mutex);
        return NULL;
    }
    
    request = queue->head;
    queue->head = queue->head->next;
    
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    
    queue->count--;
    pthread_mutex_unlock(&queue->mutex);
    
    return request;
}

/*
 * Distrugge la coda e libera la memoria
 */
void destroy_queue(request_queue_t *queue) {
    request_t *current, *next;
    
    pthread_mutex_lock(&queue->mutex);
    current = queue->head;
    
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    
    queue->head = queue->tail = NULL;
    queue->count = 0;
    pthread_mutex_unlock(&queue->mutex);
    
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_empty);
}

/*
 * Funzione per generare tempo casuale tra 0 e 1 secondo
 */
double random_time(void) {
    return (double)rand() / RAND_MAX;
}

/*
 * Funzione per il logging thread-safe
 */
void log_message(const char* message) {
    time_t now;
    char time_str[64];
    
    pthread_mutex_lock(&log_mutex);
    
    time(&now);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    printf("[%s] %s (Coda: %d elementi)\n", time_str, message, request_queue.count);
    fflush(stdout);
    
    pthread_mutex_unlock(&log_mutex);
}

/*
 * Thread worker che elabora le richieste dalla coda
 */
void* worker_thread(void* arg) {
    int worker_id = *(int*)arg;
    request_t *request;
    double processing_time, service_time;
    char log_msg[256];
    char response[] = "Richiesta elaborata con successo\n";
    
    sprintf(log_msg, "Worker thread %d avviato", worker_id);
    log_message(log_msg);
    
    while (server_running) {
        /* Preleva richiesta dalla coda */
        request = dequeue_request(&request_queue);
        if (request == NULL) {
            break; /* Server in chiusura */
        }
        
        sprintf(log_msg, "Worker %d: elaborando richiesta da client %d", 
                worker_id, request->client_fd);
        log_message(log_msg);
        
        /* Tempo di elaborazione casuale (0-1 sec) */
        processing_time = random_time();
        usleep((unsigned int)(processing_time * 1000000));
        
        sprintf(log_msg, "Worker %d: elaborazione completata in %.3f sec", 
                worker_id, processing_time);
        log_message(log_msg);
        
        /* Tempo di servizio casuale (0-1 sec) per invio risposta */
        service_time = random_time();
        usleep((unsigned int)(service_time * 1000000));
        
        /* Invio risposta al client */
        if (write(request->client_fd, response, strlen(response)) < 0) {
            sprintf(log_msg, "Worker %d: errore invio risposta", worker_id);
            log_message(log_msg);
        } else {
            sprintf(log_msg, "Worker %d: risposta inviata in %.3f sec", 
                    worker_id, service_time);
            log_message(log_msg);
        }
        
        /* Chiude connessione client */
        close(request->client_fd);
        free(request);
        
        sprintf(log_msg, "Worker %d: connessione client chiusa", worker_id);
        log_message(log_msg);
    }
    
    sprintf(log_msg, "Worker thread %d terminato", worker_id);
    log_message(log_msg);
    
    return NULL;
}

/*
 * Thread per gestire singolo client
 */
void* client_handler(void* arg) {
    int client_fd = *(int*)arg;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    char log_msg[256];
    
    free(arg); /* Libera memoria allocata per l'argomento */
    
    sprintf(log_msg, "Nuovo client connesso (fd: %d)", client_fd);
    log_message(log_msg);
    
    /* Legge richiesta dal client */
    bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
    if (bytes_read < 0) {
        sprintf(log_msg, "Errore lettura da client %d", client_fd);
        log_message(log_msg);
        close(client_fd);
        return NULL;
    }
    
    buffer[bytes_read] = '\0';
    
    sprintf(log_msg, "Richiesta ricevuta da client %d: %s", client_fd, buffer);
    log_message(log_msg);
    
    /* Aggiunge richiesta alla coda per elaborazione */
    enqueue_request(&request_queue, client_fd, buffer);
    
    return NULL;
}

/*
 * Crea socket del server
 */
int create_server_socket(void) {
    int server_fd;
    struct sockaddr_in server_addr;
    int opt = 1;
    
    /* Crea socket */
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        err_sys("socket failed");
    }
    
    /* Opzioni socket */
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        err_sys("setsockopt failed");
    }
    
    /* Configurazione indirizzo */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    /* Bind */
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        err_sys("bind failed");
    }
    
    /* Listen */
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        err_sys("listen failed");
    }
    
    return server_fd;
}

/*
 * Funzione principale
 */
int main(void) {
    int server_fd, client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_len;
    pthread_t worker_threads[WORKER_THREADS];
    pthread_t client_thread;
    int worker_ids[WORKER_THREADS];
    int *client_fd_ptr;
    int i;
    char log_msg[256];
    
    /* Inizializzazione */
    srand((unsigned int)time(NULL));
    init_queue(&request_queue);
    
    log_message("=== SERVER AVVIATO ===");
    sprintf(log_msg, "Server in ascolto sulla porta %d", PORT);
    log_message(log_msg);
    
    /* Crea socket server */
    server_fd = create_server_socket();
    
    /* Avvia thread worker */
    log_message("Avvio thread worker...");
    for (i = 0; i < WORKER_THREADS; i++) {
        worker_ids[i] = i + 1;
        if (pthread_create(&worker_threads[i], NULL, worker_thread, &worker_ids[i]) != 0) {
            err_sys("pthread_create failed per worker thread");
        }
    }
    
    /* Ciclo principale - accetta connessioni client */
    log_message("Server pronto per accettare connessioni");
    while (server_running) {
        client_len = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_fd < 0) {
            if (server_running) {
                err_msg("accept failed");
            }
            continue;
        }
        
        /* Alloca memoria per passare fd al thread */
        client_fd_ptr = (int*)malloc(sizeof(int));
        if (client_fd_ptr == NULL) {
            err_sys("malloc failed");
        }
        *client_fd_ptr = client_fd;
        
        /* Crea thread per gestire client */
        if (pthread_create(&client_thread, NULL, client_handler, client_fd_ptr) != 0) {
            err_msg("pthread_create failed per client handler");
            close(client_fd);
            free(client_fd_ptr);
            continue;
        }
        
        /* Thread detached - gestisce la propria pulizia */
        pthread_detach(client_thread);
    }
    
    /* Pulizia e chiusura */
    log_message("Avvio procedura di chiusura server...");
    server_running = 0;
    
    /* Sveglia tutti i worker thread */
    pthread_cond_broadcast(&request_queue.not_empty);
    
    /* Attende terminazione worker threads */
    for (i = 0; i < WORKER_THREADS; i++) {
        pthread_join(worker_threads[i], NULL);
    }
    
    close(server_fd);
    destroy_queue(&request_queue);
    
    log_message("=== SERVER TERMINATO ===");
    
    return 0;
}