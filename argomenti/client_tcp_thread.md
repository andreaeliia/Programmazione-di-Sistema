# Client TCP con Thread - Riferimento Completo

## 🎯 Concetti Fondamentali

### **Cos'è un Thread?**
- **Lightweight process**: processo leggero che condivide memoria
- **Concorrenza**: più thread eseguono contemporaneamente  
- **Condivisione risorse**: memoria, file descriptor, variabili globali
- **Sincronizzazione**: mutex, condition variables per coordinare

### **Perché Thread nei Client TCP?**
- **Separazione compiti**: un thread per invio, uno per ricezione
- **Non-blocking**: UI non si blocca durante operazioni di rete
- **Parallelismo**: gestire più connessioni simultanee
- **Responsiveness**: migliore esperienza utente

---

## 🔧 Client TCP Base con Thread

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

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
    
    printf("🎧 Thread ricezione [%d] avviato\n", data->thread_id);
    
    while (data->running && client_running) {
        // Ricevi dati dal server
        bytes_received = recv(data->socket_fd, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            
            // Stampa sincronizzata (evita output mescolato)
            pthread_mutex_lock(&print_mutex);
            printf("📩 Server: %s", buffer);
            fflush(stdout);
            pthread_mutex_unlock(&print_mutex);
            
        } else if (bytes_received == 0) {
            // Server ha chiuso la connessione
            pthread_mutex_lock(&print_mutex);
            printf("🔌 Server ha chiuso la connessione\n");
            pthread_mutex_unlock(&print_mutex);
            client_running = 0;
            break;
            
        } else {
            // Errore di rete
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                pthread_mutex_lock(&print_mutex);
                printf("❌ Errore ricezione: %s\n", strerror(errno));
                pthread_mutex_unlock(&print_mutex);
                client_running = 0;
                break;
            }
        }
        
        usleep(10000);  // Pausa 10ms per non saturare CPU
    }
    
    printf("🛑 Thread ricezione [%d] terminato\n", data->thread_id);
    return NULL;
}

// Thread per inviare messaggi al server
void* thread_invio(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE];
    
    printf("⌨️  Thread invio [%d] avviato\n", data->thread_id);
    printf("💡 Digita messaggi (quit per uscire):\n");
    
    while (data->running && client_running) {
        // Input utente (non bloccante sarebbe meglio, ma per semplicità...)
        printf("%s> ", data->username);
        fflush(stdout);
        
        if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
            // Rimuovi newline
            buffer[strcspn(buffer, "\n")] = 0;
            
            // Controlla comando di uscita
            if (strcmp(buffer, "quit") == 0 || strcmp(buffer, "exit") == 0) {
                printf("👋 Disconnessione richiesta...\n");
                client_running = 0;
                break;
            }
            
            // Prepara messaggio con username
            snprintf(message, BUFFER_SIZE, "[%s]: %s\n", data->username, buffer);
            
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
```

---

## 🔒 Client con Condition Variables (Per Traccia Esame)

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

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

// Struttura per sincronizzazione tra thread
typedef struct {
    pthread_mutex_t mutex;          // Mutex per protezione
    pthread_cond_t condition;       // Condition variable
    int server_available;           // Flag: server libero?
    int current_thread;             // ID thread corrente che usa server
    int access_count;               // Contatore accessi
} ServerAccess;

// Dati condivisi tra thread
typedef struct {
    int socket_fd;
    int thread_id;
    ServerAccess* access_control;
    int running;
} ThreadData;

// Inizializza struttura di controllo accesso
ServerAccess* init_server_access() {
    ServerAccess* access = malloc(sizeof(ServerAccess));
    
    pthread_mutex_init(&access->mutex, NULL);
    pthread_cond_init(&access->condition, NULL);
    access->server_available = 1;      // Server inizialmente libero
    access->current_thread = -1;       // Nessun thread sta usando server
    access->access_count = 0;
    
    return access;
}

// Thread che accede al server
void* thread_access_server(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    ServerAccess* access = data->access_control;
    char buffer[BUFFER_SIZE];
    
    printf("🚀 Thread %d avviato\n", data->thread_id);
    
    for (int i = 0; i < 5 && data->running; i++) {  // 5 accessi per test
        
        // 1. RICHIEDI ACCESSO ESCLUSIVO AL SERVER
        pthread_mutex_lock(&access->mutex);
        
        printf("🔄 Thread %d: richiede accesso al server (tentativo %d)\n", 
               data->thread_id, i + 1);
        
        // Aspetta che il server sia disponibile
        while (!access->server_available) {
            printf("⏳ Thread %d: server occupato da thread %d, aspetto...\n", 
                   data->thread_id, access->current_thread);
            
            // CONDITION WAIT: rilascia mutex e aspetta segnale
            pthread_cond_wait(&access->condition, &access->mutex);
        }
        
        // Server ora disponibile - prendiamo controllo
        access->server_available = 0;      // Segna server come occupato
        access->current_thread = data->thread_id;
        access->access_count++;
        
        printf("✅ Thread %d: ottenuto accesso esclusivo (#%d)\n", 
               data->thread_id, access->access_count);
        
        pthread_mutex_unlock(&access->mutex);
        
        // 2. USA IL SERVER (sezione critica)
        printf("🌐 Thread %d: invio richiesta al server...\n", data->thread_id);
        
        snprintf(buffer, BUFFER_SIZE, "Richiesta da Thread %d - accesso #%d", 
                data->thread_id, i + 1);
        
        send(data->socket_fd, buffer, strlen(buffer), 0);
        
        // Ricevi risposta
        int bytes_received = recv(data->socket_fd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            printf("📩 Thread %d ricevuto: %s\n", data->thread_id, buffer);
        }
        
        // Simula elaborazione
        printf("⚙️  Thread %d: elaborazione dati...\n", data->thread_id);
        sleep(2 + (rand() % 3));  // 2-4 secondi
        
        // 3. RILASCIA ACCESSO AL SERVER
        pthread_mutex_lock(&access->mutex);
        
        access->server_available = 1;       // Libera server
        access->current_thread = -1;
        
        printf("🔓 Thread %d: rilasciato accesso al server\n", data->thread_id);
        
        // CONDITION SIGNAL: sveglia thread in attesa
        pthread_cond_signal(&access->condition);
        
        pthread_mutex_unlock(&access->mutex);
        
        // Pausa tra accessi
        sleep(1 + (rand() % 2));
    }
    
    printf("🏁 Thread %d terminato\n", data->thread_id);
    return NULL;
}

int main() {
    int socket_fd;
    struct sockaddr_in server_addr;
    pthread_t thread1, thread2;
    ThreadData data1, data2;
    ServerAccess* access_control;
    
    srand(time(NULL));
    
    // 1. CONNESSIONE AL SERVER
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
    
    if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connessione fallita");
        return -1;
    }
    
    printf("✅ Connesso al server\n");
    
    // 2. INIZIALIZZA CONTROLLO ACCESSO
    access_control = init_server_access();
    
    // 3. CONFIGURA DATI THREAD
    data1.socket_fd = socket_fd;
    data1.thread_id = 1;
    data1.access_control = access_control;
    data1.running = 1;
    
    data2.socket_fd = socket_fd;
    data2.thread_id = 2;
    data2.access_control = access_control;
    data2.running = 1;
    
    // 4. CREA THREAD
    printf("🎯 Creazione 2 thread con accesso sincronizzato\n");
    printf("📋 Regola: NON possono accedere al server contemporaneamente\n\n");
    
    pthread_create(&thread1, NULL, thread_access_server, &data1);
    pthread_create(&thread2, NULL, thread_access_server, &data2);
    
    // 5. ATTESA COMPLETAMENTO
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    // 6. CLEANUP
    close(socket_fd);
    pthread_mutex_destroy(&access_control->mutex);
    pthread_cond_destroy(&access_control->condition);
    free(access_control);
    
    printf("\n🎉 Test completato - nessun accesso sovrapposto!\n");
    return 0;
}
```

---

## 🎯 Client per Controllo Velocità Stream (Traccia Esame)

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <termios.h>
#include <fcntl.h>
#include <time.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

// Struttura per controllo velocità
typedef struct {
    pthread_mutex_t mutex;
    int velocita_ms;                // Intervallo in millisecondi
    int caratteri_ricevuti;         // Contatore caratteri
    time_t inizio_misurazione;      // Timestamp inizio
    FILE* output_file;              // File per salvare dati
} ControlloVelocita;

// Dati globali
ControlloVelocita* controllo;
int client_running = 1;

// Imposta terminale in modalità non-bloccante
void setup_terminal() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ICANON | ECHO);  // Disabilita buffering e echo
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);  // Non-bloccante
}

// Ripristina terminale
void restore_terminal() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

// Thread per gestire input utente (controllo velocità)
void* thread_controllo_velocita(void* arg) {
    char key;
    
    printf("🎮 Thread controllo velocità avviato\n");
    printf("💡 Comandi: 'u' = aumenta velocità, 'd' = diminuisce, 'q' = quit\n");
    
    while (client_running) {
        if (read(STDIN_FILENO, &key, 1) > 0) {
            pthread_mutex_lock(&controllo->mutex);
            
            switch (key) {
                case 'u':
                case 'U':
                    // Aumenta velocità (diminuisce intervallo)
                    if (controllo->velocita_ms > 10) {
                        controllo->velocita_ms -= 50;
                        if (controllo->velocita_ms < 10) controllo->velocita_ms = 10;
                    }
                    printf("📈 Velocità aumentata: %d caratteri/sec\n", 
                           1000 / controllo->velocita_ms);
                    break;
                    
                case 'd':
                case 'D':
                    // Diminuisce velocità (aumenta intervallo)
                    controllo->velocita_ms += 50;
                    if (controllo->velocita_ms > 2000) controllo->velocita_ms = 2000;
                    printf("📉 Velocità diminuita: %d caratteri/sec\n", 
                           1000 / controllo->velocita_ms);
                    break;
                    
                case 'q':
                case 'Q':
                    printf("🛑 Uscita richiesta\n");
                    client_running = 0;
                    break;
            }
            
            pthread_mutex_unlock(&controllo->mutex);
        }
        
        usleep(50000);  // 50ms
    }
    
    return NULL;
}

// Thread per ricevere stream di caratteri
void* thread_ricezione_stream(void* arg) {
    int socket_fd = *(int*)arg;
    char buffer[BUFFER_SIZE];
    int bytes_received;
    time_t ultimo_aggiornamento = time(NULL);
    
    printf("📡 Thread ricezione stream avviato\n");
    
    while (client_running) {
        bytes_received = recv(socket_fd, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            
            pthread_mutex_lock(&controllo->mutex);
            
            // Salva su file
            fprintf(controllo->output_file, "%s", buffer);
            fflush(controllo->output_file);
            
            // Aggiorna contatori
            controllo->caratteri_ricevuti += bytes_received;
            
            // Mostra caratteri ricevuti
            printf("📥 Ricevuto: %s", buffer);
            
            // Calcola e mostra velocità ogni secondo
            time_t now = time(NULL);
            if (now - ultimo_aggiornamento >= 1) {
                time_t elapsed = now - controllo->inizio_misurazione;
                if (elapsed > 0) {
                    float velocita_effettiva = (float)controllo->caratteri_ricevuti / elapsed;
                    printf("📊 Velocità effettiva: %.1f caratteri/sec | Impostata: %d char/sec\n",
                           velocita_effettiva, 1000 / controllo->velocita_ms);
                }
                ultimo_aggiornamento = now;
            }
            
            pthread_mutex_unlock(&controllo->mutex);
            
        } else if (bytes_received == 0) {
            printf("🔌 Server ha chiuso la connessione\n");
            client_running = 0;
            break;
        } else {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                printf("❌ Errore ricezione: %s\n", strerror(errno));
                client_running = 0;
                break;
            }
        }
        
        usleep(10000);  // 10ms
    }
    
    return NULL;
}

// Thread per inviare richieste di velocità al server
void* thread_controllo_server(void* arg) {
    int socket_fd = *(int*)arg;
    char comando[64];
    int velocita_precedente = -1;
    
    printf("⚙️  Thread controllo server avviato\n");
    
    while (client_running) {
        pthread_mutex_lock(&controllo->mutex);
        int velocita_corrente = controllo->velocita_ms;
        pthread_mutex_unlock(&controllo->mutex);
        
        // Invia comando solo se velocità è cambiata
        if (velocita_corrente != velocita_precedente) {
            snprintf(comando, sizeof(comando), "VELOCITA:%d\n", 1000 / velocita_corrente);
            send(socket_fd, comando, strlen(comando), 0);
            
            printf("📤 Inviato comando velocità: %d char/sec\n", 1000 / velocita_corrente);
            velocita_precedente = velocita_corrente;
        }
        
        sleep(1);  // Controlla ogni secondo
    }
    
    return NULL;
}

int main() {
    int socket_fd;
    struct sockaddr_in server_addr;
    pthread_t thread_velocita, thread_stream, thread_server;
    
    // Inizializza controllo velocità
    controllo = malloc(sizeof(ControlloVelocita));
    pthread_mutex_init(&controllo->mutex, NULL);
    controllo->velocita_ms = 500;  // Iniziale: 2 caratteri/sec
    controllo->caratteri_ricevuti = 0;
    controllo->inizio_misurazione = time(NULL);
    
    // Apri file di output
    controllo->output_file = fopen("stream_data.txt", "w");
    if (!controllo->output_file) {
        perror("Errore apertura file output");
        return -1;
    }
    
    // Setup terminale
    setup_terminal();
    
    // Connessione al server
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
    
    if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connessione fallita");
        restore_terminal();
        return -1;
    }
    
    printf("✅ Connesso al server per stream controllo velocità\n");
    
    // Crea thread
    pthread_create(&thread_velocita, NULL, thread_controllo_velocita, NULL);
    pthread_create(&thread_stream, NULL, thread_ricezione_stream, &socket_fd);
    pthread_create(&thread_server, NULL, thread_controllo_server, &socket_fd);
    
    // Attesa terminazione
    pthread_join(thread_velocita, NULL);
    pthread_join(thread_stream, NULL);
    pthread_join(thread_server, NULL);
    
    // Cleanup
    close(socket_fd);
    fclose(controllo->output_file);
    restore_terminal();
    pthread_mutex_destroy(&controllo->mutex);
    free(controllo);
    
    printf("👋 Client terminato\n");
    return 0;
}
```

---

## 🛠️ Funzioni Utility per Thread

```c
// Crea thread con gestione errori
int crea_thread_sicuro(pthread_t* thread, void* (*funzione)(void*), void* arg) {
    if (pthread_create(thread, NULL, funzione, arg) != 0) {
        perror("pthread_create");
        return -1;
    }
    return 0;
}

// Termina thread con timeout
int termina_thread_timeout(pthread_t thread, int timeout_sec) {
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    timeout.tv_sec += timeout_sec;
    
    return pthread_timedjoin_np(thread, NULL, &timeout);
}

// Thread-safe printf
void thread_safe_printf(const char* format, ...) {
    static pthread_mutex_t printf_mutex = PTHREAD_MUTEX_INITIALIZER;
    va_list args;
    
    pthread_mutex_lock(&printf_mutex);
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    fflush(stdout);
    pthread_mutex_unlock(&printf_mutex);
}

// Barrier semplice per sincronizzare N thread
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    int thread_count;
    int waiting_threads;
} SimpleBarrier;

SimpleBarrier* barrier_create(int num_threads) {
    SimpleBarrier* barrier = malloc(sizeof(SimpleBarrier));
    pthread_mutex_init(&barrier->mutex, NULL);
    pthread_cond_init(&barrier->condition, NULL);
    barrier->thread_count = num_threads;
    barrier->waiting_threads = 0;
    return barrier;
}

void barrier_wait(SimpleBarrier* barrier) {
    pthread_mutex_lock(&barrier->mutex);
    barrier->waiting_threads++;
    
    if (barrier->waiting_threads == barrier->thread_count) {
        // Ultimo thread - sveglia tutti
        barrier->waiting_threads = 0;
        pthread_cond_broadcast(&barrier->condition);
    } else {
        // Aspetta che tutti arrivino
        pthread_cond_wait(&barrier->condition, &barrier->mutex);
    }
    
    pthread_mutex_unlock(&barrier->mutex);
}
```

---

## 📋 Checklist Thread Programming

### ✅ **Setup Thread**
- [ ] Include `#include <pthread.h>`
- [ ] Definisci strutture dati condivise
- [ ] Inizializza mutex e condition variables
- [ ] Crea funzioni thread con signature `void* func(void* arg)`

### ✅ **Sincronizzazione**
- [ ] Usa `pthread_mutex_lock/unlock` per sezioni critiche
- [ ] Usa `pthread_cond_wait/signal` per coordinazione
- [ ] Evita deadlock (sempre stesso ordine di lock)
- [ ] Controlla race conditions

### ✅ **Gestione Thread**
- [ ] `pthread_create()` per creare
- [ ] `pthread_join()` per aspettare terminazione
- [ ] `pthread_cancel()` per terminazione forzata
- [ ] Cleanup con `pthread_mutex_destroy()`

### ✅ **Best Practices**
- [ ] Thread-safe printf per output pulito
- [ ] Gestione errori su tutte le operazioni
- [ ] Flag globali per terminazione coordinata
- [ ] Timeout su operazioni bloccanti

---

## 🎯 Compilazione

```bash
# Compila con libreria pthread
gcc -o client_thread client_thread.c -lpthread

# Con debug
gcc -g -Wall -o client_thread client_thread.c -lpthread

# Con ottimizzazioni
gcc -O2 -o client_thread client_thread.c -lpthread
```

## 🚀 Patterns Thread Comuni

### **Producer-Consumer**
```c
// Thread producer scrive in buffer
// Thread consumer legge da buffer
// Mutex protegge buffer
// Condition variables segnalano dati pronti/spazio libero
```

### **Worker Pool**
```c
// Thread master distribuisce lavoro
// Pool di worker thread elaborano
// Coda di lavori protetta da mutex
```

### **Event-Driven**
```c
// Thread aspettano eventi con condition variables
// Eventi scatenano elaborazione
// Coordinazione tramite flag condivisi
```