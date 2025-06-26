#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <time.h>  

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

// Macro per pulizia stringhe
#define REMOVE_NEWLINE(str) do { \
    (str)[strcspn((str), "\n\r")] = '\0'; \
} while(0)

// VARIABILI GLOBALI
int A;
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t A_mutex = PTHREAD_MUTEX_INITIALIZER;
int client_running = 1;
int A_changed = 0;

// Struttura per passare dati ai thread
typedef struct {
    int socket_fd;
    char username[50];
    int thread_id;
    int running;
} ThreadData;

// Verifica se una stringa contiene solo cifre
int is_digit_string(const char* str) {
    if (str == NULL || *str == '\0') return 0;
    
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) return 0;
    }
    return 1;
}

int safe_string_to_int(const char* str, int* result) {
    if (str == NULL || result == NULL) return -1;
    
    errno = 0;
    char* endptr;
    long val = strtol(str, &endptr, 10);
    
    if (errno == ERANGE) {
        printf("Errore: numero fuori range\n");
        return -1;
    }
    
    if (endptr == str || *endptr != '\0') {
        printf("Errore: formato numero non valido\n");
        return -1;
    }
    
    if (val < INT_MIN || val > INT_MAX) {
        printf("Errore: numero troppo grande\n");
        return -1;
    }
    
    *result = (int)val;
    return 0;
}

// Funzione per aggiornare A
void update_A(int new_value, int thread_id){
    pthread_mutex_lock(&A_mutex);

    A = new_value;

    // Aggiorna variabile di ambiente
    char A_string[20];
    sprintf(A_string,"%d",A);
    setenv("A",A_string,1);

    A_changed = 1;

    printf("[Thread %d] A aggiornata: %d (ambiente: %s)\n", 
           thread_id, A, getenv("A"));
    
    pthread_mutex_unlock(&A_mutex);
}

// Thread per ricevere messaggi dal server
void* thread_ricezione(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    char buffer[BUFFER_SIZE];
    int bytes_received;
    int first_message = 1;
    
    printf("Thread ricezione [%d] avviato\n", data->thread_id);
    
    while (data->running && client_running) {
        bytes_received = recv(data->socket_fd, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            
            pthread_mutex_lock(&print_mutex);
            printf("Server: %s", buffer);
            
            if (first_message) {
                first_message = 0;
            } else {
                int temp_A;
                if (safe_string_to_int(buffer, &temp_A) == 0) {
                    printf("Variabile A aggiornata: %d\n", temp_A);
                } else {
                    printf("Errore conversione numero\n");
                }
            }
            
            fflush(stdout);
            pthread_mutex_unlock(&print_mutex);
            
        } else if (bytes_received == 0) {
            pthread_mutex_lock(&print_mutex);
            printf("Server ha chiuso la connessione\n");
            pthread_mutex_unlock(&print_mutex);
            client_running = 0;
            break;
            
        } else {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                pthread_mutex_lock(&print_mutex);
                printf("Errore ricezione: %s\n", strerror(errno));
                pthread_mutex_unlock(&print_mutex);
                client_running = 0;
                break;
            }
        }
        
        usleep(10000);
    }
    
    printf("Thread ricezione [%d] terminato\n", data->thread_id);
    return NULL;
}

void* thread_modificatore1(void *arg){
    ThreadData* data = (ThreadData*)arg;

    printf("Thread modificatore 1 [%d] avviato\n", data->thread_id);

    srand(time(NULL) + data->thread_id);

    while(data->running && client_running){
        int random_value = rand() % 1000 + 1;
        update_A(random_value, data->thread_id);
        
        int sleep_ms = (rand() % 3300) + 700;
        usleep(sleep_ms * 1000);
    }
    printf("Thread modificatore 1 [%d] terminato\n", data->thread_id);
    return NULL;
}

void* thread_modificatore2(void *arg){
    ThreadData* data = (ThreadData*)arg;

    printf("Thread modificatore 2 [%d] avviato\n", data->thread_id);

    srand(time(NULL) + data->thread_id);

    while(data->running && client_running){
        int random_value = rand() % 1000 + 1;
        update_A(random_value, data->thread_id);
        
        int sleep_ms = (rand() % 3300) + 700;
        usleep(sleep_ms * 1000);
    }
    printf("Thread modificatore 2 [%d] terminato\n", data->thread_id);
    return NULL;
}

// Thread che invia A al server quando cambia
void* thread_tcp_sender(void* arg){
    ThreadData* data = (ThreadData*)arg;
    char buffer[BUFFER_SIZE];
    int last_A_send = -1;

    printf("Thread TCP sender [%d] avviato\n", data->thread_id);

    while (data->running && client_running) {
        pthread_mutex_lock(&A_mutex);

        if(A_changed && A != last_A_send){
            int value_to_send = A;
            A_changed = 0;
            pthread_mutex_unlock(&A_mutex);

            snprintf(buffer, BUFFER_SIZE, "%d\n", value_to_send);
            int bytes_send = send(data->socket_fd, buffer, strlen(buffer), 0);
            if(bytes_send > 0){
                last_A_send = value_to_send;
                printf("[TCP] Inviato al server: %d\n", value_to_send);
            } else {
                printf("[TCP] Errore invio: %s\n", strerror(errno));
                client_running = 0;
                break;
            }
        } else {
            pthread_mutex_unlock(&A_mutex);
        }

        usleep(100000);
    }

    printf("Thread TCP sender [%d] terminato\n", data->thread_id);
    return NULL;
}

int main() {
    int socket_fd;
    struct sockaddr_in server_addr;
    pthread_t thread_recv, thread_mod1, thread_mod2, thread_tcp;
    ThreadData recv_data, mod1_data, mod2_data, tcp_data;
    char username[50];
    
    printf("Inserisci username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0;
    
    // Creazione socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Configurazione server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        printf("Indirizzo server non valido: %s\n", SERVER_IP);
        return -1;
    }
    
    // Connessione al server
    printf("Connessione a %s:%d...\n", SERVER_IP, SERVER_PORT);
    
    if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Connessione fallita: %s\n", strerror(errno));
        return -1;
    }
    
    printf("Connesso al server\n");
    
    // Configurazione dati thread
    recv_data.socket_fd = socket_fd;
    recv_data.thread_id = 1;
    recv_data.running = 1;
    strcpy(recv_data.username, username);
    
    mod1_data.socket_fd = socket_fd;
    mod1_data.thread_id = 2;
    mod1_data.running = 1;
    strcpy(mod1_data.username, username);
    
    mod2_data.socket_fd = socket_fd;
    mod2_data.thread_id = 3;
    mod2_data.running = 1;
    strcpy(mod2_data.username, username);

    tcp_data.socket_fd = socket_fd;
    tcp_data.thread_id = 4;
    tcp_data.running = 1;
    strcpy(tcp_data.username, username);

    // Creazione thread
    if (pthread_create(&thread_recv, NULL, thread_ricezione, &recv_data) != 0) {
        printf("Errore creazione thread ricezione\n");
        close(socket_fd);
        return -1;
    }

    if (pthread_create(&thread_mod1, NULL, thread_modificatore1, &mod1_data) != 0) {
        printf("Errore creazione thread modificatore 1\n");
        close(socket_fd);
        return -1;
    }

    if (pthread_create(&thread_mod2, NULL, thread_modificatore2, &mod2_data) != 0) {
        printf("Errore creazione thread modificatore 2\n");
        close(socket_fd);
        return -1;
    }

    if (pthread_create(&thread_tcp, NULL, thread_tcp_sender, &tcp_data) != 0) {
        printf("Errore creazione thread TCP\n");
        close(socket_fd);
        return -1;
    }
    
    printf("Client avviato con 4 thread\n");
    
    // Attesa terminazione thread
    pthread_join(thread_mod1, NULL);
    pthread_join(thread_mod2, NULL);
    pthread_join(thread_tcp, NULL);
    
    // Termina thread ricezione
    recv_data.running = 0;
    pthread_cancel(thread_recv);
    pthread_join(thread_recv, NULL);
    
    // Cleanup
    close(socket_fd);
    pthread_mutex_destroy(&print_mutex);
    pthread_mutex_destroy(&A_mutex);
    
    printf("Client terminato\n");
    return 0;
}