#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"

// Struttura per sincronizzazione thread
typedef struct {
    pthread_mutex_t mutex;      // Mutex per protezione dati
    pthread_cond_t condition;   // Condition variable per sincronizzazione
    int current_thread;         // Chi può accedere al server (0 o 1)
    int server_socket;          // Socket condiviso per comunicazione
} sync_data_t;

// Funzione thread - ogni thread esegue questa funzione
void* thread_function(void* arg) {
    int thread_id = *((int*)arg);
    extern sync_data_t sync_data;
    char received_char;
    char dummy = 'X';  // Carattere da inviare per richiedere risposta
    
    printf("Thread %d iniziato\n", thread_id);
    
    for (int i = 0; i < 5; i++) {  // Ogni thread fa 5 richieste
        
        // === SEZIONE CRITICA - INIZIO ===
        pthread_mutex_lock(&sync_data.mutex);
        
        // Aspetta il proprio turno
        while (sync_data.current_thread != thread_id) {
            printf("Thread %d aspetta il suo turno...\n", thread_id);
            pthread_cond_wait(&sync_data.condition, &sync_data.mutex);
        }
        
        printf("Thread %d: È il mio turno! Accedo al server...\n", thread_id);
        
        // Comunica con il server
        if (send(sync_data.server_socket, &dummy, 1, 0) < 0) {
            err_sys("send failed");
        }
        
        if (recv(sync_data.server_socket, &received_char, 1, 0) <= 0) {
            err_sys("recv failed");
        }
        
        printf("Thread %d ricevuto carattere: '%c'\n", thread_id, received_char);
        
        // Simula lavoro (evita che sia troppo veloce)
        sleep(1);
        
        // Passa il turno all'altro thread
        sync_data.current_thread = (thread_id == 0) ? 1 : 0;
        printf("Thread %d: Passo il turno al thread %d\n", thread_id, sync_data.current_thread);
        
        // Notifica l'altro thread che può procedere
        pthread_cond_signal(&sync_data.condition);
        
        pthread_mutex_unlock(&sync_data.mutex);
        // === SEZIONE CRITICA - FINE ===
        
        // Lavoro non critico (può essere in parallelo)
        printf("Thread %d: Elaboro il carattere ricevuto...\n", thread_id);
        sleep(1);  // Simula elaborazione
    }
    
    printf("Thread %d terminato\n", thread_id);
    return NULL;
}

// Variabile globale per sincronizzazione
sync_data_t sync_data;

int main(void) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    pthread_t thread1, thread2;
    int thread_id1 = 0, thread_id2 = 1;
    
    // Inizializza strutture di sincronizzazione
    if (pthread_mutex_init(&sync_data.mutex, NULL) != 0) {
        err_sys("mutex init failed");
    }
    
    if (pthread_cond_init(&sync_data.condition, NULL) != 0) {
        err_sys("condition variable init failed");
    }
    
    sync_data.current_thread = 0;  // Il thread 0 inizia per primo
    
    // Crea socket client
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        err_sys("socket creation error");
    }
    
    // Configura indirizzo server
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        err_sys("invalid address");
    }
    
    // Connetti al server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        err_sys("connection failed");
    }
    
    printf("Connesso al server %s:%d\n", SERVER_IP, PORT);
    sync_data.server_socket = sock;
    
    // Crea i due thread
    printf("Creazione thread...\n");
    
    if (pthread_create(&thread1, NULL, thread_function, &thread_id1) != 0) {
        err_sys("thread creation failed");
    }
    
    if (pthread_create(&thread2, NULL, thread_function, &thread_id2) != 0) {
        err_sys("thread creation failed");
    }
    
    // Aspetta che entrambi i thread finiscano
    printf("Aspetto che i thread finiscano...\n");
    
    if (pthread_join(thread1, NULL) != 0) {
        err_sys("thread join failed");
    }
    
    if (pthread_join(thread2, NULL) != 0) {
        err_sys("thread join failed");
    }
    
    printf("Tutti i thread sono terminati\n");
    
    // Pulisci risorse
    close(sock);
    pthread_mutex_destroy(&sync_data.mutex);
    pthread_cond_destroy(&sync_data.condition);
    
    return 0;
}