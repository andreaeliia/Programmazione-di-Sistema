# Daemon di Sistema - Riferimento Completo

## 🎯 Concetti Fondamentali

### **Cos'è un Daemon?**
- **Background process**: processo che gira in background senza interfaccia utente
- **System service**: fornisce servizi al sistema o ad altri processi
- **Independent**: non legato a sessioni utente o terminali
- **Persistent**: rimane attivo anche dopo logout dell'utente

### **Caratteristiche Daemon**
- **No controlling terminal**: scollegato da terminali
- **Session leader**: diventa leader di nuova sessione
- **Working directory**: di solito root (/) per sicurezza
- **File descriptors**: chiude stdin/stdout/stderr standard
- **PID file**: spesso salva il proprio PID in un file

---

## 🔧 Daemon Base - Template Completo

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#define DAEMON_NAME "mydaemon"
#define PID_FILE "/var/run/mydaemon.pid"
#define LOG_FILE "/var/log/mydaemon.log"

// Flag globale per terminazione pulita
volatile sig_atomic_t daemon_running = 1;

// Handler per segnali di terminazione
void signal_handler(int sig) {
    switch (sig) {
        case SIGTERM:
            syslog(LOG_INFO, "Ricevuto SIGTERM - terminazione daemon");
            daemon_running = 0;
            break;
        case SIGINT:
            syslog(LOG_INFO, "Ricevuto SIGINT - terminazione daemon");
            daemon_running = 0;
            break;
        case SIGHUP:
            syslog(LOG_INFO, "Ricevuto SIGHUP - ricarico configurazione");
            // Qui potresti ricaricare configurazione
            break;
        default:
            syslog(LOG_WARNING, "Ricevuto segnale inaspettato: %d", sig);
            break;
    }
}

// Crea file PID per prevenire istanze multiple
int create_pid_file() {
    int pid_fd;
    char pid_str[16];
    
    // Apri file PID in modalità esclusiva
    pid_fd = open(PID_FILE, O_CREAT | O_WRONLY | O_EXCL, 0644);
    if (pid_fd < 0) {
        if (errno == EEXIST) {
            syslog(LOG_ERR, "Daemon già in esecuzione (PID file exists)");
            return -1;
        } else {
            syslog(LOG_ERR, "Impossibile creare PID file: %s", strerror(errno));
            return -1;
        }
    }
    
    // Scrivi PID nel file
    snprintf(pid_str, sizeof(pid_str), "%d\n", getpid());
    if (write(pid_fd, pid_str, strlen(pid_str)) != strlen(pid_str)) {
        syslog(LOG_ERR, "Errore scrittura PID file");
        close(pid_fd);
        unlink(PID_FILE);
        return -1;
    }
    
    close(pid_fd);
    syslog(LOG_INFO, "PID file creato: %s (PID: %d)", PID_FILE, getpid());
    return 0;
}

// Rimuovi file PID al termine
void remove_pid_file() {
    if (unlink(PID_FILE) < 0) {
        syslog(LOG_WARNING, "Errore rimozione PID file: %s", strerror(errno));
    } else {
        syslog(LOG_INFO, "PID file rimosso");
    }
}

// Processo di daemonizzazione completo
int daemonize() {
    pid_t pid, sid;
    
    // 1. PRIMO FORK - Crea processo figlio
    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Errore primo fork: %s\n", strerror(errno));
        return -1;
    }
    
    if (pid > 0) {
        // Processo padre termina
        exit(EXIT_SUCCESS);
    }
    
    // Ora siamo nel processo figlio
    
    // 2. DIVENTA SESSION LEADER
    sid = setsid();
    if (sid < 0) {
        syslog(LOG_ERR, "Errore setsid: %s", strerror(errno));
        return -1;
    }
    
    // 3. SECONDO FORK - Previene acquisizione controlling terminal
    pid = fork();
    if (pid < 0) {
        syslog(LOG_ERR, "Errore secondo fork: %s", strerror(errno));
        return -1;
    }
    
    if (pid > 0) {
        // Primo figlio termina
        exit(EXIT_SUCCESS);
    }
    
    // Ora siamo nel processo nipote (vero daemon)
    
    // 4. CAMBIA WORKING DIRECTORY
    if (chdir("/") < 0) {
        syslog(LOG_ERR, "Errore chdir: %s", strerror(errno));
        return -1;
    }
    
    // 5. IMPOSTA UMASK per controllo permessi file
    umask(0);
    
    // 6. CHIUDI FILE DESCRIPTOR STANDARD
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    // 7. REINDIRIZZA STANDARD FD a /dev/null
    int null_fd = open("/dev/null", O_RDWR);
    if (null_fd >= 0) {
        dup2(null_fd, STDIN_FILENO);   // stdin -> /dev/null
        dup2(null_fd, STDOUT_FILENO);  // stdout -> /dev/null
        dup2(null_fd, STDERR_FILENO);  // stderr -> /dev/null
        
        if (null_fd > STDERR_FILENO) {
            close(null_fd);
        }
    }
    
    syslog(LOG_INFO, "Daemonizzazione completata - PID: %d", getpid());
    return 0;
}

// Inizializza logging di sistema
void init_logging() {
    // Apri connessione a syslog
    openlog(DAEMON_NAME, LOG_PID | LOG_CONS, LOG_DAEMON);
    
    syslog(LOG_INFO, "=== %s daemon avviato ===", DAEMON_NAME);
    syslog(LOG_INFO, "PID: %d", getpid());
    syslog(LOG_INFO, "PPID: %d", getppid());
}

// Cleanup risorse al termine
void cleanup_daemon() {
    syslog(LOG_INFO, "Cleanup daemon in corso...");
    
    // Rimuovi file PID
    remove_pid_file();
    
    // Chiudi logging
    syslog(LOG_INFO, "=== %s daemon terminato ===", DAEMON_NAME);
    closelog();
}

// Logica principale del daemon
void daemon_main_loop() {
    time_t last_heartbeat = time(NULL);
    int heartbeat_interval = 60;  // Heartbeat ogni 60 secondi
    
    syslog(LOG_INFO, "Daemon main loop avviato");
    
    while (daemon_running) {
        time_t current_time = time(NULL);
        
        // HEARTBEAT - log periodico per monitoraggio
        if (current_time - last_heartbeat >= heartbeat_interval) {
            syslog(LOG_INFO, "Daemon heartbeat - uptime: %ld secondi", 
                   current_time - last_heartbeat);
            last_heartbeat = current_time;
        }
        
        // QUI VA LA LOGICA SPECIFICA DEL TUO DAEMON
        // Esempi:
        // - Elaborazione file in una directory
        // - Controllo sistema
        // - Gestione richieste di rete
        // - Backup automatici
        
        // Per questo esempio, semplice sleep
        sleep(10);  // Controllo ogni 10 secondi
    }
    
    syslog(LOG_INFO, "Daemon main loop terminato");
}

int main(int argc, char* argv[]) {
    // Controllo parametri
    if (argc > 1 && strcmp(argv[1], "--foreground") == 0) {
        // Modalità foreground per debug
        printf("Modalità foreground - PID: %d\n", getpid());
        init_logging();
        
        if (create_pid_file() < 0) {
            return EXIT_FAILURE;
        }
        
    } else {
        // Modalità daemon normale
        printf("Avvio daemon %s...\n", DAEMON_NAME);
        
        // Inizializza logging prima della daemonizzazione
        init_logging();
        
        // Daemonizza processo
        if (daemonize() < 0) {
            syslog(LOG_ERR, "Errore daemonizzazione");
            return EXIT_FAILURE;
        }
        
        // Crea file PID dopo daemonizzazione
        if (create_pid_file() < 0) {
            return EXIT_FAILURE;
        }
    }
    
    // Installa handler per segnali
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGHUP, signal_handler);
    signal(SIGPIPE, SIG_IGN);  // Ignora SIGPIPE
    
    // Registra funzione cleanup per atexit
    atexit(cleanup_daemon);
    
    // Avvia logica principale
    daemon_main_loop();
    
    return EXIT_SUCCESS;
}
```

---

## 🎯 Daemon TCP con 10 Socket e Select (Per Traccia Esame)

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#include <pthread.h>

#define DAEMON_NAME "tcpserver_daemon"
#define BASE_PORT 8080
#define NUM_SOCKETS 10
#define MAX_CLIENTS_PER_SOCKET 5
#define BUFFER_SIZE 1024

// Struttura per gestire socket server
typedef struct {
    int socket_fd;              // File descriptor socket
    int port;                   // Porta di ascolto
    int active_clients;         // Numero client connessi
    struct sockaddr_in addr;    // Indirizzo socket
} ServerSocket;

// Struttura per gestire client
typedef struct {
    int socket_fd;              // FD client
    int server_index;           // Indice server di appartenenza
    time_t connect_time;        // Timestamp connessione
    char buffer[BUFFER_SIZE];   // Buffer dati parziali
} ClientConnection;

// Variabili globali
ServerSocket servers[NUM_SOCKETS];
ClientConnection clients[NUM_SOCKETS * MAX_CLIENTS_PER_SOCKET];
pthread_t network_thread;
volatile sig_atomic_t daemon_running = 1;

// Signal handler
void signal_handler(int sig) {
    if (sig == SIGTERM || sig == SIGINT) {
        syslog(LOG_INFO, "Ricevuto segnale terminazione %d", sig);
        daemon_running = 0;
    }
}

// Inizializza array client
void init_clients() {
    for (int i = 0; i < NUM_SOCKETS * MAX_CLIENTS_PER_SOCKET; i++) {
        clients[i].socket_fd = -1;  // -1 = slot libero
        clients[i].server_index = -1;
    }
}

// Trova slot libero per nuovo client
int find_free_client_slot() {
    for (int i = 0; i < NUM_SOCKETS * MAX_CLIENTS_PER_SOCKET; i++) {
        if (clients[i].socket_fd == -1) {
            return i;
        }
    }
    return -1;  // Nessun slot libero
}

// Rimuovi client dall'array
void remove_client(int client_index) {
    if (client_index >= 0 && client_index < NUM_SOCKETS * MAX_CLIENTS_PER_SOCKET) {
        if (clients[client_index].socket_fd != -1) {
            close(clients[client_index].socket_fd);
            
            // Decrementa contatore server
            int server_idx = clients[client_index].server_index;
            if (server_idx >= 0 && server_idx < NUM_SOCKETS) {
                servers[server_idx].active_clients--;
            }
            
            clients[client_index].socket_fd = -1;
            clients[client_index].server_index = -1;
        }
    }
}

// Crea e configura socket server
int create_server_socket(int port) {
    int sockfd;
    struct sockaddr_in addr;
    int opt = 1;
    
    // Crea socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        syslog(LOG_ERR, "Errore creazione socket porta %d: %s", port, strerror(errno));
        return -1;
    }
    
    // Opzioni socket
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        syslog(LOG_ERR, "Errore setsockopt porta %d: %s", port, strerror(errno));
        close(sockfd);
        return -1;
    }
    
    // Configura indirizzo
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    // Bind
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        syslog(LOG_ERR, "Errore bind porta %d: %s", port, strerror(errno));
        close(sockfd);
        return -1;
    }
    
    // Listen
    if (listen(sockfd, MAX_CLIENTS_PER_SOCKET) < 0) {
        syslog(LOG_ERR, "Errore listen porta %d: %s", port, strerror(errno));
        close(sockfd);
        return -1;
    }
    
    syslog(LOG_INFO, "Socket server creato su porta %d (FD: %d)", port, sockfd);
    return sockfd;
}

// Inizializza tutti i socket server
int init_server_sockets() {
    for (int i = 0; i < NUM_SOCKETS; i++) {
        int port = BASE_PORT + i;
        int sockfd = create_server_socket(port);
        
        if (sockfd < 0) {
            // Cleanup socket già creati
            for (int j = 0; j < i; j++) {
                close(servers[j].socket_fd);
            }
            return -1;
        }
        
        // Inizializza struttura server
        servers[i].socket_fd = sockfd;
        servers[i].port = port;
        servers[i].active_clients = 0;
        servers[i].addr.sin_family = AF_INET;
        servers[i].addr.sin_addr.s_addr = INADDR_ANY;
        servers[i].addr.sin_port = htons(port);
    }
    
    syslog(LOG_INFO, "Inizializzati %d socket server (porte %d-%d)", 
           NUM_SOCKETS, BASE_PORT, BASE_PORT + NUM_SOCKETS - 1);
    return 0;
}

// Thread per gestione rete con I/O multiplexing
void* network_thread_func(void* arg) {
    fd_set read_fds, master_fds;
    int max_fd = 0;
    struct timeval timeout;
    char buffer[BUFFER_SIZE];
    
    syslog(LOG_INFO, "Thread di rete avviato con select() I/O multiplexing");
    
    // Inizializza set file descriptor
    FD_ZERO(&master_fds);
    
    // Aggiungi tutti i socket server al set
    for (int i = 0; i < NUM_SOCKETS; i++) {
        FD_SET(servers[i].socket_fd, &master_fds);
        if (servers[i].socket_fd > max_fd) {
            max_fd = servers[i].socket_fd;
        }
    }
    
    while (daemon_running) {
        // Copia master set
        read_fds = master_fds;
        
        // Timeout per select (1 secondo)
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        // I/O MULTIPLEXING con select
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (activity < 0) {
            if (errno != EINTR) {
                syslog(LOG_ERR, "Errore select: %s", strerror(errno));
                break;
            }
            continue;
        }
        
        if (activity == 0) {
            // Timeout - nessuna attività
            continue;
        }
        
        // Controlla socket server per nuove connessioni
        for (int i = 0; i < NUM_SOCKETS && daemon_running; i++) {
            if (FD_ISSET(servers[i].socket_fd, &read_fds)) {
                // Nuova connessione su server i
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                
                int client_fd = accept(servers[i].socket_fd, 
                                     (struct sockaddr*)&client_addr, &client_len);
                
                if (client_fd < 0) {
                    syslog(LOG_ERR, "Errore accept server %d: %s", i, strerror(errno));
                    continue;
                }
                
                // Trova slot libero per client
                int client_slot = find_free_client_slot();
                if (client_slot == -1) {
                    syslog(LOG_WARNING, "Nessun slot libero per nuovo client server %d", i);
                    send(client_fd, "Server pieno\n", 13, 0);
                    close(client_fd);
                    continue;
                }
                
                // Configura nuovo client
                clients[client_slot].socket_fd = client_fd;
                clients[client_slot].server_index = i;
                clients[client_slot].connect_time = time(NULL);
                
                FD_SET(client_fd, &master_fds);
                if (client_fd > max_fd) {
                    max_fd = client_fd;
                }
                
                servers[i].active_clients++;
                
                syslog(LOG_INFO, "Nuovo client [%d] connesso a server %d (porta %d) - slot %d", 
                       client_fd, i, servers[i].port, client_slot);
                
                // Messaggio di benvenuto
                snprintf(buffer, BUFFER_SIZE, 
                        "🎉 Benvenuto su server daemon porta %d!\n"
                        "📊 Client attivi su questo server: %d/%d\n", 
                        servers[i].port, servers[i].active_clients, MAX_CLIENTS_PER_SOCKET);
                send(client_fd, buffer, strlen(buffer), 0);
            }
        }
        
        // Controlla client per dati in arrivo
        for (int i = 0; i < NUM_SOCKETS * MAX_CLIENTS_PER_SOCKET && daemon_running; i++) {
            int client_fd = clients[i].socket_fd;
            
            if (client_fd != -1 && FD_ISSET(client_fd, &read_fds)) {
                int bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
                
                if (bytes_read <= 0) {
                    // Client disconnesso
                    int server_idx = clients[i].server_index;
                    syslog(LOG_INFO, "Client [%d] disconnesso da server %d", 
                           client_fd, server_idx);
                    
                    FD_CLR(client_fd, &master_fds);
                    remove_client(i);
                    
                    // Aggiorna max_fd se necessario
                    if (client_fd == max_fd) {
                        max_fd = 0;
                        for (int j = 0; j < NUM_SOCKETS; j++) {
                            if (servers[j].socket_fd > max_fd) {
                                max_fd = servers[j].socket_fd;
                            }
                        }
                        for (int j = 0; j < NUM_SOCKETS * MAX_CLIENTS_PER_SOCKET; j++) {
                            if (clients[j].socket_fd > max_fd) {
                                max_fd = clients[j].socket_fd;
                            }
                        }
                    }
                } else {
                    // Elabora dati ricevuti
                    buffer[bytes_read] = '\0';
                    int server_idx = clients[i].server_index;
                    
                    syslog(LOG_INFO, "Dati da client [%d] server %d: '%s'", 
                           client_fd, server_idx, buffer);
                    
                    // Echo risposta al client
                    snprintf(buffer, BUFFER_SIZE, 
                            "✅ Server daemon %d ricevuto: %s\n"
                            "📊 Timestamp: %ld\n", 
                            server_idx, buffer, time(NULL));
                    send(client_fd, buffer, strlen(buffer), 0);
                }
            }
        }
    }
    
    syslog(LOG_INFO, "Thread di rete terminato");
    return NULL;
}

// Cleanup risorse di rete
void cleanup_network() {
    syslog(LOG_INFO, "Cleanup risorse di rete...");
    
    // Chiudi tutti i client
    for (int i = 0; i < NUM_SOCKETS * MAX_CLIENTS_PER_SOCKET; i++) {
        if (clients[i].socket_fd != -1) {
            close(clients[i].socket_fd);
        }
    }
    
    // Chiudi tutti i socket server
    for (int i = 0; i < NUM_SOCKETS; i++) {
        if (servers[i].socket_fd != -1) {
            close(servers[i].socket_fd);
            syslog(LOG_INFO, "Chiuso socket server porta %d", servers[i].port);
        }
    }
}

// Statistiche daemon
void print_daemon_stats() {
    int total_clients = 0;
    
    for (int i = 0; i < NUM_SOCKETS; i++) {
        total_clients += servers[i].active_clients;
        syslog(LOG_INFO, "Server %d (porta %d): %d client attivi", 
               i, servers[i].port, servers[i].active_clients);
    }
    
    syslog(LOG_INFO, "Statistiche: %d client totali su %d server", 
           total_clients, NUM_SOCKETS);
}

int main(int argc, char* argv[]) {
    // Inizializza logging
    openlog(DAEMON_NAME, LOG_PID | LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "=== Avvio daemon TCP multi-socket ===");
    
    // Daemonizza (commentare per debug)
    if (argc == 1 || strcmp(argv[1], "--foreground") != 0) {
        if (daemon(0, 0) < 0) {
            syslog(LOG_ERR, "Errore daemonizzazione");
            return EXIT_FAILURE;
        }
    }
    
    // Handler segnali
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGPIPE, SIG_IGN);
    
    // Inizializza strutture dati
    init_clients();
    
    // Crea socket server
    if (init_server_sockets() < 0) {
        syslog(LOG_ERR, "Errore inizializzazione socket server");
        return EXIT_FAILURE;
    }
    
    // Avvia thread di rete
    if (pthread_create(&network_thread, NULL, network_thread_func, NULL) != 0) {
        syslog(LOG_ERR, "Errore creazione thread di rete");
        cleanup_network();
        return EXIT_FAILURE;
    }
    
    syslog(LOG_INFO, "Daemon TCP avviato con successo");
    syslog(LOG_INFO, "PID: %d, Socket server: %d (porte %d-%d)", 
           getpid(), NUM_SOCKETS, BASE_PORT, BASE_PORT + NUM_SOCKETS - 1);
    
    // Main loop daemon
    time_t last_stats = time(NULL);
    while (daemon_running) {
        sleep(30);  // Heartbeat ogni 30 secondi
        
        time_t now = time(NULL);
        if (now - last_stats >= 300) {  // Statistiche ogni 5 minuti
            print_daemon_stats();
            last_stats = now;
        }
    }
    
    syslog(LOG_INFO, "Terminazione daemon in corso...");
    
    // Termina thread di rete
    pthread_cancel(network_thread);
    pthread_join(network_thread, NULL);
    
    // Cleanup
    cleanup_network();
    
    syslog(LOG_INFO, "=== Daemon TCP terminato ===");
    closelog();
    
    return EXIT_SUCCESS;
}
```

---

## 🛠️ Utility per Gestione Daemon

```c
// Controlla se daemon è già in esecuzione
int check_daemon_running(const char* pid_file) {
    FILE* file = fopen(pid_file, "r");
    if (!file) {
        return 0;  // PID file non esiste
    }
    
    int pid;
    if (fscanf(file, "%d", &pid) == 1) {
        fclose(file);
        
        // Controlla se processo esiste
        if (kill(pid, 0) == 0) {
            return pid;  // Daemon running
        } else {
            // PID file orfano - rimuovi
            unlink(pid_file);
            return 0;
        }
    }
    
    fclose(file);
    return 0;
}

// Stop daemon inviando SIGTERM
int stop_daemon(const char* pid_file) {
    int pid = check_daemon_running(pid_file);
    if (pid <= 0) {
        printf("Daemon non in esecuzione\n");
        return -1;
    }
    
    printf("Stopping daemon PID %d...\n", pid);
    if (kill(pid, SIGTERM) < 0) {
        perror("kill");
        return -1;
    }
    
    // Aspetta terminazione (max 10 secondi)
    for (int i = 0; i < 10; i++) {
        if (kill(pid, 0) < 0) {
            printf("Daemon stopped\n");
            return 0;
        }
        sleep(1);
    }
    
    // Force kill se non risponde
    printf("Daemon non risponde, force kill...\n");
    kill(pid, SIGKILL);
    return 0;
}

// Ricarica configurazione daemon
int reload_daemon(const char* pid_file) {
    int pid = check_daemon_running(pid_file);
    if (pid <= 0) {
        printf("Daemon non in esecuzione\n");
        return -1;
    }
    
    printf("Reloading daemon configuration PID %d...\n", pid);
    return kill(pid, SIGHUP);
}

// Status daemon
void daemon_status(const char* pid_file) {
    int pid = check_daemon_running(pid_file);
    if (pid > 0) {
        printf("Daemon running (PID: %d)\n", pid);
    } else {
        printf("Daemon not running\n");
    }
}
```

---

## 📋 Checklist Daemon

### ✅ **Daemonizzazione**
- [ ] Fork twice per scollegare da parent
- [ ] `setsid()` per diventare session leader
- [ ] `chdir("/")` per working directory sicura
- [ ] `umask(0)` per controllo permessi
- [ ] Chiudi stdin/stdout/stderr
- [ ] Reindirizza a /dev/null

### ✅ **Gestione PID File**
- [ ] Crea PID file in /var/run/
- [ ] Lock esclusivo per prevenire istanze multiple
- [ ] Rimuovi PID file alla terminazione

### ✅ **Logging**
- [ ] Usa syslog per log di sistema
- [ ] Log eventi importanti (start/stop/errori)
- [ ] Configura facility e priority appropriate

### ✅ **Signal Handling**
- [ ] Handler per SIGTERM (terminazione)
- [ ] Handler per SIGHUP (reload config)
- [ ] Ignora SIGPIPE per socket
- [ ] Terminazione pulita con cleanup

---

## 🎯 Compilazione e Gestione

```bash
# Compila daemon
gcc -o tcp_daemon daemon.c -lpthread

# Avvia in foreground (debug)
./tcp_daemon --foreground

# Avvia come daemon
sudo ./tcp_daemon

# Controlla se running
ps aux | grep tcp_daemon

# Termina daemon
sudo kill -TERM $(cat /var/run/mydaemon.pid)

# Test connessioni multiple
for i in {0..9}; do 
    echo "test" | nc localhost $((8080 + i)) &
done
```

## 🚀 Vantaggi I/O Multiplexing nel Daemon

| **Vantaggio** | **Descrizione** |
|---------------|-----------------|
| **Scalabilità** | Gestisce migliaia di connessioni con un thread |
| **Efficienza** | Basso overhead rispetto a thread multipli |
| **Robustezza** | Nessun race condition, gestione errori centralizzata |
| **Monitoraggio** | Facile controllare tutte le connessioni |
| **Resource control** | Controllo preciso di memoria e CPU |