# Stream Control e Velocità - Riferimento Completo

## 🎯 Concetti Fondamentali

### **Stream di Dati**
- **Continuous flow**: flusso continuo di dati senza interruzioni
- **Rate control**: controllo della velocità di trasmissione
- **Buffer management**: gestione buffer per evitare overflow/underflow
- **Flow control**: meccanismi per regolare il flusso dati

### **Controllo Velocità**
- **Dynamic rate**: modifica velocità in tempo reale
- **User interaction**: controllo tramite input utente
- **Feedback loop**: sistema di controllo con feedback
- **Throttling**: limitazione velocità per evitare sovraccarico

---

## 🔧 Server Stream con Controllo Velocità

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

#define SERVER_PORT 8080
#define BUFFER_SIZE 1024
#define DEFAULT_STREAM_RATE 2   // Caratteri per secondo iniziali

// Struttura per controllo stream
typedef struct {
    int client_socket;              // Socket client
    pthread_mutex_t rate_mutex;     // Mutex per proteggere rate
    int chars_per_second;           // Velocità corrente (char/sec)
    int stream_active;              // Flag: stream attivo?
    unsigned long chars_sent;       // Contatore caratteri totali
    time_t stream_start_time;       // Timestamp inizio stream
    char stream_buffer[BUFFER_SIZE]; // Buffer caratteri da inviare
    int buffer_position;            // Posizione corrente nel buffer
    char client_id[32];             // Identificativo client
} StreamControl;

// Struttura per gestire client
typedef struct {
    int socket_fd;                  // Socket del client
    struct sockaddr_in address;     // Indirizzo client
    StreamControl* stream_ctrl;     // Controllo stream
    pthread_t stream_thread;        // Thread per gestione stream
    pthread_t command_thread;       // Thread per comandi client
    int active;                     // Flag: client attivo?
} ClientHandler;

// Genera carattere alfanumerico casuale
char generate_random_char() {
    int type = rand() % 3;
    switch (type) {
        case 0: return 'A' + (rand() % 26);    // Maiuscola A-Z
        case 1: return 'a' + (rand() % 26);    // Minuscola a-z
        case 2: return '0' + (rand() % 10);    // Numero 0-9
        default: return 'X';
    }
}

// Riempi buffer con caratteri casuali
void fill_stream_buffer(StreamControl* ctrl) {
    for (int i = 0; i < BUFFER_SIZE - 1; i++) {
        ctrl->stream_buffer[i] = generate_random_char();
    }
    ctrl->stream_buffer[BUFFER_SIZE - 1] = '\0';
    ctrl->buffer_position = 0;
    
    printf("🔄 Buffer stream riempito per client %s\n", ctrl->client_id);
}

// Calcola delay in microsecondi per velocità richiesta
int calculate_delay_microseconds(int chars_per_second) {
    if (chars_per_second <= 0) chars_per_second = 1;
    if (chars_per_second > 1000) chars_per_second = 1000;  // Max 1000 char/sec
    
    return 1000000 / chars_per_second;  // 1 secondo / chars_per_second
}

// Thread per gestione stream continuo
void* stream_thread_func(void* arg) {
    ClientHandler* client = (ClientHandler*)arg;
    StreamControl* ctrl = client->stream_ctrl;
    char current_char;
    int delay_us;
    
    printf("📡 Thread stream avviato per client %s\n", ctrl->client_id);
    
    // Riempi buffer iniziale
    fill_stream_buffer(ctrl);
    
    while (ctrl->stream_active && client->active) {
        // Ottieni carattere corrente
        if (ctrl->buffer_position >= BUFFER_SIZE - 1) {
            // Buffer esaurito - riempi di nuovo
            fill_stream_buffer(ctrl);
        }
        
        current_char = ctrl->stream_buffer[ctrl->buffer_position++];
        
        // Invia carattere al client
        if (send(client->socket_fd, &current_char, 1, MSG_NOSIGNAL) <= 0) {
            printf("❌ Errore invio carattere a client %s: %s\n", 
                   ctrl->client_id, strerror(errno));
            break;
        }
        
        // Aggiorna statistiche
        ctrl->chars_sent++;
        
        // Log periodico ogni 100 caratteri
        if (ctrl->chars_sent % 100 == 0) {
            time_t elapsed = time(NULL) - ctrl->stream_start_time;
            double actual_rate = elapsed > 0 ? (double)ctrl->chars_sent / elapsed : 0;
            
            printf("📊 Client %s: %lu caratteri inviati, rate effettivo: %.1f char/sec\n",
                   ctrl->client_id, ctrl->chars_sent, actual_rate);
        }
        
        // Calcola delay basato sulla velocità corrente
        pthread_mutex_lock(&ctrl->rate_mutex);
        delay_us = calculate_delay_microseconds(ctrl->chars_per_second);
        pthread_mutex_unlock(&ctrl->rate_mutex);
        
        // Pausa per controllo velocità
        usleep(delay_us);
    }
    
    printf("🛑 Thread stream terminato per client %s\n", ctrl->client_id);
    return NULL;
}

// Thread per gestione comandi controllo velocità
void* command_thread_func(void* arg) {
    ClientHandler* client = (ClientHandler*)arg;
    StreamControl* ctrl = client->stream_ctrl;
    char command_buffer[256];
    int bytes_received;
    
    printf("⌨️  Thread comandi avviato per client %s\n", ctrl->client_id);
    
    while (ctrl->stream_active && client->active) {
        // Ricevi comando dal client
        bytes_received = recv(client->socket_fd, command_buffer, 
                             sizeof(command_buffer) - 1, MSG_DONTWAIT);
        
        if (bytes_received > 0) {
            command_buffer[bytes_received] = '\0';
            
            // Rimuovi newline
            command_buffer[strcspn(command_buffer, "\r\n")] = 0;
            
            printf("📥 Comando da client %s: '%s'\n", ctrl->client_id, command_buffer);
            
            // Processa comando
            if (strncmp(command_buffer, "VELOCITA:", 9) == 0) {
                // Comando velocità diretta: VELOCITA:10
                int new_rate = atoi(command_buffer + 9);
                
                pthread_mutex_lock(&ctrl->rate_mutex);
                int old_rate = ctrl->chars_per_second;
                ctrl->chars_per_second = new_rate;
                pthread_mutex_unlock(&ctrl->rate_mutex);
                
                printf("⚡ Client %s: velocità cambiata da %d a %d char/sec\n",
                       ctrl->client_id, old_rate, new_rate);
                
                // Invia conferma al client
                char response[128];
                snprintf(response, sizeof(response), 
                        "RATE_CHANGED:%d\n", new_rate);
                send(client->socket_fd, response, strlen(response), MSG_NOSIGNAL);
                
            } else if (strcmp(command_buffer, "STOP") == 0) {
                // Comando stop stream
                ctrl->stream_active = 0;
                printf("🛑 Stop stream richiesto da client %s\n", ctrl->client_id);
                
            } else if (strcmp(command_buffer, "STATUS") == 0) {
                // Comando status
                time_t elapsed = time(NULL) - ctrl->stream_start_time;
                double actual_rate = elapsed > 0 ? (double)ctrl->chars_sent / elapsed : 0;
                
                char status[256];
                snprintf(status, sizeof(status),
                        "STATUS: Rate=%d char/sec, Sent=%lu, Elapsed=%lds, Actual=%.1f char/sec\n",
                        ctrl->chars_per_second, ctrl->chars_sent, elapsed, actual_rate);
                
                send(client->socket_fd, status, strlen(status), MSG_NOSIGNAL);
                
            } else {
                // Comando non riconosciuto
                char error_msg[] = "ERROR: Unknown command\n";
                send(client->socket_fd, error_msg, strlen(error_msg), MSG_NOSIGNAL);
            }
        } else if (bytes_received == 0) {
            // Client ha chiuso connessione
            printf("🔌 Client %s ha chiuso la connessione\n", ctrl->client_id);
            client->active = 0;
            break;
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            // Errore di rete
            printf("❌ Errore ricezione da client %s: %s\n", 
                   ctrl->client_id, strerror(errno));
            client->active = 0;
            break;
        }
        
        usleep(100000);  // 100ms tra controlli
    }
    
    printf("🛑 Thread comandi terminato per client %s\n", ctrl->client_id);
    return NULL;
}

// Crea struttura controllo stream
StreamControl* create_stream_control(int client_socket, const char* client_id) {
    StreamControl* ctrl = malloc(sizeof(StreamControl));
    if (!ctrl) {
        perror("malloc StreamControl");
        return NULL;
    }
    
    ctrl->client_socket = client_socket;
    ctrl->chars_per_second = DEFAULT_STREAM_RATE;
    ctrl->stream_active = 1;
    ctrl->chars_sent = 0;
    ctrl->stream_start_time = time(NULL);
    ctrl->buffer_position = 0;
    
    strncpy(ctrl->client_id, client_id, sizeof(ctrl->client_id) - 1);
    ctrl->client_id[sizeof(ctrl->client_id) - 1] = '\0';
    
    if (pthread_mutex_init(&ctrl->rate_mutex, NULL) != 0) {
        perror("pthread_mutex_init");
        free(ctrl);
        return NULL;
    }
    
    printf("✅ StreamControl creato per client %s (rate iniziale: %d char/sec)\n",
           client_id, DEFAULT_STREAM_RATE);
    
    return ctrl;
}

// Distruggi controllo stream
void destroy_stream_control(StreamControl* ctrl) {
    if (ctrl) {
        ctrl->stream_active = 0;
        pthread_mutex_destroy(&ctrl->rate_mutex);
        
        time_t total_time = time(NULL) - ctrl->stream_start_time;
        double avg_rate = total_time > 0 ? (double)ctrl->chars_sent / total_time : 0;
        
        printf("📊 Statistiche finali client %s:\n", ctrl->client_id);
        printf("   📈 Caratteri inviati: %lu\n", ctrl->chars_sent);
        printf("   ⏱️  Durata: %ld secondi\n", total_time);
        printf("   📊 Rate medio: %.2f char/sec\n", avg_rate);
        
        free(ctrl);
    }
}

// Gestisci nuovo client
void handle_new_client(int client_socket, struct sockaddr_in client_addr) {
    ClientHandler* client = malloc(sizeof(ClientHandler));
    if (!client) {
        perror("malloc ClientHandler");
        close(client_socket);
        return;
    }
    
    // Inizializza client handler
    client->socket_fd = client_socket;
    client->address = client_addr;
    client->active = 1;
    
    // Crea ID client
    char client_id[32];
    snprintf(client_id, sizeof(client_id), "%s:%d", 
             inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    
    // Crea controllo stream
    client->stream_ctrl = create_stream_control(client_socket, client_id);
    if (!client->stream_ctrl) {
        close(client_socket);
        free(client);
        return;
    }
    
    printf("🎯 Nuovo client connesso: %s\n", client_id);
    
    // Invia messaggio di benvenuto
    char welcome[256];
    snprintf(welcome, sizeof(welcome),
             "🎉 Benvenuto al Server Stream!\n"
             "📡 Stream caratteri avviato a %d char/sec\n"
             "💡 Comandi: VELOCITA:<num>, STATUS, STOP\n"
             "📊 Inizia ricezione stream...\n\n",
             DEFAULT_STREAM_RATE);
    
    send(client_socket, welcome, strlen(welcome), 0);
    
    // Avvia thread di gestione
    if (pthread_create(&client->stream_thread, NULL, stream_thread_func, client) != 0) {
        perror("pthread_create stream");
        destroy_stream_control(client->stream_ctrl);
        close(client_socket);
        free(client);
        return;
    }
    
    if (pthread_create(&client->command_thread, NULL, command_thread_func, client) != 0) {
        perror("pthread_create command");
        client->stream_ctrl->stream_active = 0;
        pthread_join(client->stream_thread, NULL);
        destroy_stream_control(client->stream_ctrl);
        close(client_socket);
        free(client);
        return;
    }
    
    // Aspetta terminazione thread (in thread separato per gestire altri client)
    pthread_detach(client->stream_thread);
    pthread_detach(client->command_thread);
    
    // Nota: in un server reale, dovresti mantenere una lista di client attivi
    // e gestire la loro terminazione in modo più robusto
}

// Main server
int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    int opt = 1;
    
    srand(time(NULL));  // Inizializza random per caratteri
    
    // Crea socket server
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // Opzioni socket
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    // Configura indirizzo server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);
    
    // Bind e listen
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_socket, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    printf("🚀 Server Stream avviato su porta %d\n", SERVER_PORT);
    printf("📡 Invia stream continuo di caratteri alfanumerici\n");
    printf("⚡ Velocità controllabile dinamicamente dai client\n");
    printf("💡 Usa netcat per test: nc localhost %d\n\n", SERVER_PORT);
    
    // Loop accettazione client
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_socket < 0) {
            perror("accept");
            continue;
        }
        
        // Gestisci client in thread separato
        // Nota: per semplicità, qui gestiamo direttamente
        // In un server reale, avresti un pool di thread o fork()
        handle_new_client(client_socket, client_addr);
    }
    
    close(server_socket);
    return 0;
}
```

---

## 🎮 Client con Controllo Velocità Interattivo

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

// Struttura per controllo client
typedef struct {
    int socket_fd;                  // Socket connessione server
    int current_speed;              // Velocità corrente
    int received_chars;             // Caratteri ricevuti
    time_t start_time;              // Timestamp inizio
    FILE* output_file;              // File per salvare dati
    pthread_mutex_t speed_mutex;    // Mutex per proteggere velocità
    int client_running;             // Flag: client attivo?
    struct termios original_termios; // Impostazioni terminale originali
} ClientControl;

ClientControl* global_client = NULL;

// Ripristina terminale al termine
void restore_terminal() {
    if (global_client) {
        tcsetattr(STDIN_FILENO, TCSANOW, &global_client->original_termios);
    }
}

// Handler per terminazione pulita
void cleanup_handler(int sig) {
    if (global_client) {
        global_client->client_running = 0;
    }
    restore_terminal();
    printf("\n🛑 Terminazione client richiesta\n");
}

// Configura terminale per input non-bloccante
int setup_terminal() {
    struct termios new_termios;
    
    // Salva impostazioni originali
    if (tcgetattr(STDIN_FILENO, &global_client->original_termios) < 0) {
        perror("tcgetattr");
        return -1;
    }
    
    // Configura nuovo terminale
    new_termios = global_client->original_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);  // Disabilita buffering e echo
    new_termios.c_cc[VMIN] = 0;               // Non-blocking read
    new_termios.c_cc[VTIME] = 0;
    
    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_termios) < 0) {
        perror("tcsetattr");
        return -1;
    }
    
    // Imposta stdin come non-bloccante
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    
    return 0;
}

// Thread per ricezione stream dal server
void* receive_thread_func(void* arg) {
    ClientControl* client = (ClientControl*)arg;
    char buffer[BUFFER_SIZE];
    int bytes_received;
    time_t last_stats_time = time(NULL);
    
    printf("📡 Thread ricezione stream avviato\n");
    
    while (client->client_running) {
        bytes_received = recv(client->socket_fd, buffer, BUFFER_SIZE - 1, MSG_DONTWAIT);
        
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            
            // Salva su file
            if (client->output_file) {
                fwrite(buffer, 1, bytes_received, client->output_file);
                fflush(client->output_file);
            }
            
            // Aggiorna contatori
            client->received_chars += bytes_received;
            
            // Mostra caratteri ricevuti (limitato per non intasare console)
            if (bytes_received <= 10) {
                printf("📥 Ricevuto: '%s'", buffer);
            } else {
                printf("📥 Ricevuto %d caratteri: '%.10s...'", bytes_received, buffer);
            }
            
            // Statistiche ogni 5 secondi
            time_t now = time(NULL);
            if (now - last_stats_time >= 5) {
                time_t elapsed = now - client->start_time;
                double actual_rate = elapsed > 0 ? (double)client->received_chars / elapsed : 0;
                
                pthread_mutex_lock(&client->speed_mutex);
                int current_speed = client->current_speed;
                pthread_mutex_unlock(&client->speed_mutex);
                
                printf("\n📊 Statistiche: %d caratteri in %lds, rate: %.1f char/sec (impostato: %d)\n",
                       client->received_chars, elapsed, actual_rate, current_speed);
                
                last_stats_time = now;
            }
            
        } else if (bytes_received == 0) {
            printf("\n🔌 Server ha chiuso la connessione\n");
            client->client_running = 0;
            break;
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            printf("\n❌ Errore ricezione: %s\n", strerror(errno));
            client->client_running = 0;
            break;
        }
        
        usleep(10000);  // 10ms
    }
    
    printf("🛑 Thread ricezione terminato\n");
    return NULL;
}

// Thread per controllo velocità tramite tastiera
void* control_thread_func(void* arg) {
    ClientControl* client = (ClientControl*)arg;
    char key;
    char command[64];
    
    printf("🎮 Thread controllo velocità avviato\n");
    printf("💡 Comandi:\n");
    printf("   'u' = aumenta velocità\n");
    printf("   'd' = diminuisce velocità\n");
    printf("   's' = mostra statistiche\n");
    printf("   'q' = quit\n\n");
    
    while (client->client_running) {
        if (read(STDIN_FILENO, &key, 1) > 0) {
            pthread_mutex_lock(&client->speed_mutex);
            
            switch (key) {
                case 'u':
                case 'U':
                    // Aumenta velocità
                    client->current_speed += 5;
                    if (client->current_speed > 100) {
                        client->current_speed = 100;
                    }
                    
                    snprintf(command, sizeof(command), "VELOCITA:%d\n", client->current_speed);
                    send(client->socket_fd, command, strlen(command), 0);
                    
                    printf("\n📈 Velocità aumentata a %d char/sec\n", client->current_speed);
                    break;
                    
                case 'd':
                case 'D':
                    // Diminuisce velocità
                    client->current_speed -= 5;
                    if (client->current_speed < 1) {
                        client->current_speed = 1;
                    }
                    
                    snprintf(command, sizeof(command), "VELOCITA:%d\n", client->current_speed);
                    send(client->socket_fd, command, strlen(command), 0);
                    
                    printf("\n📉 Velocità diminuita a %d char/sec\n", client->current_speed);
                    break;
                    
                case 's':
                case 'S':
                    // Mostra statistiche
                    send(client->socket_fd, "STATUS\n", 7, 0);
                    printf("\n📊 Richieste statistiche server...\n");
                    break;
                    
                case 'q':
                case 'Q':
                    // Quit
                    printf("\n👋 Uscita richiesta dall'utente\n");
                    client->client_running = 0;
                    break;
                    
                default:
                    printf("\n❓ Comando non riconosciuto: '%c'\n", key);
                    break;
            }
            
            pthread_mutex_unlock(&client->speed_mutex);
        }
        
        usleep(50000);  // 50ms
    }
    
    printf("🛑 Thread controllo terminato\n");
    return NULL;
}

// Crea controllo client
ClientControl* create_client_control(int socket_fd) {
    ClientControl* client = malloc(sizeof(ClientControl));
    if (!client) {
        perror("malloc ClientControl");
        return NULL;
    }
    
    client->socket_fd = socket_fd;
    client->current_speed = 2;  // Velocità iniziale
    client->received_chars = 0;
    client->start_time = time(NULL);
    client->client_running = 1;
    
    // Apri file di output
    client->output_file = fopen("stream_data.txt", "w");
    if (!client->output_file) {
        perror("fopen output file");
        free(client);
        return NULL;
    }
    
    if (pthread_mutex_init(&client->speed_mutex, NULL) != 0) {
        perror("pthread_mutex_init");
        fclose(client->output_file);
        free(client);
        return NULL;
    }
    
    printf("✅ Client control creato\n");
    printf("📁 Dati salvati in: stream_data.txt\n");
    
    return client;
}

// Distruggi controllo client
void destroy_client_control(ClientControl* client) {
    if (client) {
        client->client_running = 0;
        
        if (client->output_file) {
            fclose(client->output_file);
        }
        
        pthread_mutex_destroy(&client->speed_mutex);
        
        time_t total_time = time(NULL) - client->start_time;
        double avg_rate = total_time > 0 ? (double)client->received_chars / total_time : 0;
        
        printf("\n📊 Statistiche finali:\n");
        printf("   📈 Caratteri ricevuti: %d\n", client->received_chars);
        printf("   ⏱️  Durata connessione: %ld secondi\n", total_time);
        printf("   📊 Rate medio: %.2f char/sec\n", avg_rate);
        printf("   📁 Dati salvati in: stream_data.txt\n");
        
        free(client);
    }
}

int main() {
    int socket_fd;
    struct sockaddr_in server_addr;
    pthread_t receive_thread, control_thread;
    
    // Setup signal handlers
    signal(SIGINT, cleanup_handler);
    signal(SIGTERM, cleanup_handler);
    atexit(restore_terminal);
    
    printf("🚀 Client Stream con Controllo Velocità\n");
    printf("🎯 Connessione a %s:%d...\n", SERVER_IP, SERVER_PORT);
    
    // Crea socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // Configura indirizzo server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        printf("❌ Indirizzo IP non valido\n");
        exit(EXIT_FAILURE);
    }
    
    // Connetti al server
    if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }
    
    printf("✅ Connesso al server!\n");
    
    // Crea controllo client
    global_client = create_client_control(socket_fd);
    if (!global_client) {
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
    
    // Setup terminale
    if (setup_terminal() < 0) {
        destroy_client_control(global_client);
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
    
    // Avvia thread
    if (pthread_create(&receive_thread, NULL, receive_thread_func, global_client) != 0) {
        perror("pthread_create receive");
        destroy_client_control(global_client);
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
    
    if (pthread_create(&control_thread, NULL, control_thread_func, global_client) != 0) {
        perror("pthread_create control");
        global_client->client_running = 0;
        pthread_join(receive_thread, NULL);
        destroy_client_control(global_client);
        close(socket_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("🎮 Client pronto - usa 'u'/'d' per controllare velocità\n\n");
    
    // Aspetta terminazione thread
    pthread_join(control_thread, NULL);
    global_client->client_running = 0;
    pthread_join(receive_thread, NULL);
    
    // Cleanup
    close(socket_fd);
    destroy_client_control(global_client);
    
    printf("👋 Client terminato\n");
    return 0;
}
```

---

## 🛠️ Utility per Gestione Stream

```c
// Buffer circolare per stream
typedef struct {
    char* buffer;
    size_t size;
    size_t head;
    size_t tail;
    size_t count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} CircularBuffer;

CircularBuffer* create_circular_buffer(size_t size) {
    CircularBuffer* cb = malloc(sizeof(CircularBuffer));
    cb->buffer = malloc(size);
    cb->size = size;
    cb->head = cb->tail = cb->count = 0;
    
    pthread_mutex_init(&cb->mutex, NULL);
    pthread_cond_init(&cb->not_empty, NULL);
    pthread_cond_init(&cb->not_full, NULL);
    
    return cb;
}

int circular_buffer_put(CircularBuffer* cb, char data) {
    pthread_mutex_lock(&cb->mutex);
    
    while (cb->count == cb->size) {
        pthread_cond_wait(&cb->not_full, &cb->mutex);
    }
    
    cb->buffer[cb->head] = data;
    cb->head = (cb->head + 1) % cb->size;
    cb->count++;
    
    pthread_cond_signal(&cb->not_empty);
    pthread_mutex_unlock(&cb->mutex);
    
    return 0;
}

int circular_buffer_get(CircularBuffer* cb, char* data) {
    pthread_mutex_lock(&cb->mutex);
    
    while (cb->count == 0) {
        pthread_cond_wait(&cb->not_empty, &cb->mutex);
    }
    
    *data = cb->buffer[cb->tail];
    cb->tail = (cb->tail + 1) % cb->size;
    cb->count--;
    
    pthread_cond_signal(&cb->not_full);
    pthread_mutex_unlock(&cb->mutex);
    
    return 0;
}

// Rate limiter con token bucket
typedef struct {
    int tokens;
    int max_tokens;
    int refill_rate;
    time_t last_refill;
    pthread_mutex_t mutex;
} TokenBucket;

TokenBucket* create_token_bucket(int max_tokens, int refill_rate) {
    TokenBucket* tb = malloc(sizeof(TokenBucket));
    tb->tokens = max_tokens;
    tb->max_tokens = max_tokens;
    tb->refill_rate = refill_rate;
    tb->last_refill = time(NULL);
    pthread_mutex_init(&tb->mutex, NULL);
    return tb;
}

int token_bucket_consume(TokenBucket* tb, int tokens_needed) {
    pthread_mutex_lock(&tb->mutex);
    
    // Refill tokens
    time_t now = time(NULL);
    time_t elapsed = now - tb->last_refill;
    int new_tokens = elapsed * tb->refill_rate;
    
    tb->tokens += new_tokens;
    if (tb->tokens > tb->max_tokens) {
        tb->tokens = tb->max_tokens;
    }
    tb->last_refill = now;
    
    // Consuma tokens
    if (tb->tokens >= tokens_needed) {
        tb->tokens -= tokens_needed;
        pthread_mutex_unlock(&tb->mutex);
        return 1;  // Success
    }
    
    pthread_mutex_unlock(&tb->mutex);
    return 0;  // Not enough tokens
}

// Misurazione bandwidth
typedef struct {
    unsigned long bytes_transferred;
    time_t window_start;
    int window_seconds;
    double current_rate;
    pthread_mutex_t mutex;
} BandwidthMeter;

BandwidthMeter* create_bandwidth_meter(int window_seconds) {
    BandwidthMeter* bm = malloc(sizeof(BandwidthMeter));
    bm->bytes_transferred = 0;
    bm->window_start = time(NULL);
    bm->window_seconds = window_seconds;
    bm->current_rate = 0.0;
    pthread_mutex_init(&bm->mutex, NULL);
    return bm;
}

void bandwidth_meter_update(BandwidthMeter* bm, size_t bytes) {
    pthread_mutex_lock(&bm->mutex);
    
    time_t now = time(NULL);
    bm->bytes_transferred += bytes;
    
    // Calcola rate se finestra completata
    if (now - bm->window_start >= bm->window_seconds) {
        bm->current_rate = (double)bm->bytes_transferred / (now - bm->window_start);
        bm->bytes_transferred = 0;
        bm->window_start = now;
    }
    
    pthread_mutex_unlock(&bm->mutex);
}

double bandwidth_meter_get_rate(BandwidthMeter* bm) {
    pthread_mutex_lock(&bm->mutex);
    double rate = bm->current_rate;
    pthread_mutex_unlock(&bm->mutex);
    return rate;
}
```

---

## 📋 Checklist Stream Control

### ✅ **Server Setup**
- [ ] Thread separato per stream continuo
- [ ] Controllo velocità dinamico con mutex
- [ ] Buffer management per caratteri
- [ ] Thread per comandi client
- [ ] Statistiche in tempo reale

### ✅ **Client Setup**  
- [ ] Thread ricezione stream non-bloccante
- [ ] Input tastiera non-bloccante
- [ ] Salvataggio dati su file
- [ ] Controllo velocità 'u'/'d'
- [ ] Terminale configurato correttamente

### ✅ **Protocollo Comunicazione**
- [ ] Comandi standard (VELOCITA:, STATUS, STOP)
- [ ] Gestione errori e disconnessioni
- [ ] Feedback immediato su cambi velocità
- [ ] Misurazione velocità effettiva

---

## 🎯 Compilazione e Test

```bash
# Compila server
gcc -o stream_server server.c -lpthread

# Compila client  
gcc -o stream_client client.c -lpthread

# Test completo
./stream_server &
./stream_client

# Test con netcat (semplice)
nc localhost 8080
# Poi digita: VELOCITA:10

# Monitor file output
tail -f stream_data.txt

# Test carico
for i in {1..5}; do ./stream_client & done
```

## 🚀 Funzionalità Avanzate

| **Feature** | **Descrizione** |
|-------------|-----------------|
| **Dynamic Rate** | Modifica velocità in tempo reale |
| **File Logging** | Salvataggio automatico dati |
| **Statistics** | Monitoraggio rate effettivo vs richiesto |
| **Multi-Client** | Gestione client multipli indipendenti |
| **Flow Control** | Prevenzione overflow buffer |