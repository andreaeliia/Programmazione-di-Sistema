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


#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024




// Macro per pulizia stringhe
#define REMOVE_NEWLINE(str) do { \
    (str)[strcspn((str), "\n\r")] = '\0'; \
} while(0)
// Macro per casting buffer
#define BUFFER_TO_INT(buf) (*((int*)(buf)))


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
    
    // Controllo errori di conversione
    if (errno == ERANGE) {
        printf("❌ Errore: numero fuori range per int!\n");
        return -1;
    }
    
    // Controlla che tutta la stringa sia stata convertita
    if (endptr == str || *endptr != '\0') {
        printf("❌ Errore: formato numero non valido!\n");
        return -1;
    }
    
    // Controllo range specifico per int
    if (val < INT_MIN || val > INT_MAX) {
        printf("❌ Errore: numero troppo grande per int!\n");
        return -1;
    }
    
    *result = (int)val;
    return 0;  // Successo
}





int A;

// Struttura per passare dati ai thread
typedef struct {
    int socket_fd;              // Socket di connessione
    char username[50];          // Nome utente
    int thread_id;              // ID identificativo thread
    int running;                // Flag per terminazione
} ThreadData;

// Variabili globali per sincronizzazione
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex per printf
int client_running = 1;                                   // Flag globale

// Thread per ricevere messaggi dal server
void* thread_ricezione(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    char buffer[BUFFER_SIZE];
    int bytes_received;
    int first_message = 1;  // Flag per saltare messaggio benvenuto
    
    printf("Thread ricezione [%d] avviato\n", data->thread_id);
    
    while (data->running && client_running) {
        bytes_received = recv(data->socket_fd, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            
            pthread_mutex_lock(&print_mutex);
            printf("Server: %s", buffer);
            
            if (first_message) {
                // Primo messaggio = benvenuto, solo stampa
                first_message = 0;
            } else {
                // Messaggi successivi = echo, estrai numero
                
                if (strcmp(buffer, "quit") == 0 || strcmp(buffer, "exit") == 0) {
                    printf("👋 Disconnessione richiesta...\n");
                    client_running = 0;
                    break;
            }
                    
                    if (safe_string_to_int(buffer, &A) == 0) {
                        printf("Variabile A aggiornata: %d\n", A);
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


// Thread per inviare messaggi al server
void* thread_invio(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE];
    A = 0;
    
    printf("⌨️  Thread invio [%d] avviato\n", data->thread_id);
    printf("💡 Digita messaggi (quit per uscire):\n");
    
    while (data->running && client_running) {
        // Input utente (non bloccante sarebbe meglio, ma per semplicità...)
        printf("%s> ", data->username);
        fflush(stdout);
        
        if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {

            //Controllo che la variabile sia un integer
            REMOVE_NEWLINE(buffer);
            if(is_digit_string(buffer)!= 1){
                printf("Hai inserito una stringa non valida riprovare\n");
                continue;
            }
            
            // Controlla comando di uscita
            if (strcmp(buffer, "quit") == 0 || strcmp(buffer, "exit") == 0) {
                printf("👋 Disconnessione richiesta...\n");
                client_running = 0;
                break;
            }
            
            // Prepara messaggio con username
            snprintf(message, BUFFER_SIZE, "%s\n", buffer);
            
            // Invia al server
            int bytes_sent = send(data->socket_fd, message, strlen(message), 0);
            
            if (bytes_sent < 0) {
                pthread_mutex_lock(&print_mutex);
                printf("❌ Errore invio: %s\n", strerror(errno));
                pthread_mutex_unlock(&print_mutex);
                client_running = 0;
                break;
            }
            
            pthread_mutex_lock(&print_mutex);
            printf("✅ Inviato: %s\n", buffer);
            pthread_mutex_unlock(&print_mutex);
        }
    }
    
    printf("🛑 Thread invio [%d] terminato\n", data->thread_id);
    return NULL;
}

int main() {
    int socket_fd;
    struct sockaddr_in server_addr;
    pthread_t thread_recv, thread_send;
    ThreadData recv_data, send_data;
    char username[50];
    
    // Input username
    printf("👤 Inserisci username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0;  // Rimuovi newline
    
    // 1. CREAZIONE SOCKET
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // 2. CONFIGURAZIONE SERVER
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        printf("❌ Indirizzo server non valido: %s\n", SERVER_IP);
        return -1;
    }
    
    // 3. CONNESSIONE AL SERVER
    printf("🔗 Connessione a %s:%d...\n", SERVER_IP, SERVER_PORT);
    
    if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("❌ Connessione fallita: %s\n", strerror(errno));
        return -1;
    }
    
    printf("✅ Connesso al server!\n");
    
    // 4. CONFIGURAZIONE DATI THREAD
    recv_data.socket_fd = socket_fd;
    recv_data.thread_id = 1;
    recv_data.running = 1;
    strcpy(recv_data.username, username);
    
    send_data.socket_fd = socket_fd;
    send_data.thread_id = 2;
    send_data.running = 1;
    strcpy(send_data.username, username);
    
    // 5. CREAZIONE THREAD
    if (pthread_create(&thread_recv, NULL, thread_ricezione, &recv_data) != 0) {
        printf("❌ Errore creazione thread ricezione\n");
        close(socket_fd);
        return -1;
    }
    
    if (pthread_create(&thread_send, NULL, thread_invio, &send_data) != 0) {
        printf("❌ Errore creazione thread invio\n");
        pthread_cancel(thread_recv);
        close(socket_fd);
        return -1;
    }
    
    // 6. ATTESA TERMINAZIONE THREAD
    printf("🚀 Client avviato con 2 thread\n");
    
    pthread_join(thread_send, NULL);   // Aspetta thread invio (controllato da utente)
    
    // Termina thread ricezione
    recv_data.running = 0;
    pthread_cancel(thread_recv);       // Forza terminazione se bloccato
    pthread_join(thread_recv, NULL);
    
    // 7. CLEANUP
    close(socket_fd);
    pthread_mutex_destroy(&print_mutex);
    
    printf("👋 Client terminato\n");
    return 0;
}