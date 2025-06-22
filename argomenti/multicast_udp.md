# Multicast UDP - Riferimento Completo

## 🎯 Concetti Fondamentali

### **Cos'è il Multicast?**
- **One-to-Many**: un sender invia a più receiver contemporaneamente
- **Efficiente**: un solo pacchetto per tutti i destinatari
- **Group-based**: basato su gruppi multicast (indirizzi speciali)
- **UDP-based**: utilizza protocollo UDP (connectionless)

### **Indirizzi Multicast**
- **Range**: 224.0.0.0 - 239.255.255.255 (Classe D)
- **Locali**: 224.0.0.0/24 (non routing, solo LAN)
- **Globali**: 224.0.1.0 - 238.255.255.255
- **Privati**: 239.0.0.0 - 239.255.255.255 (uso locale)

### **Componenti Chiave**
- **Sender**: invia dati al gruppo multicast
- **Receiver**: si unisce al gruppo e riceve dati
- **Group**: indirizzo multicast + porta
- **TTL**: Time To Live (quanti router attraversare)

---

## 🔧 Multicast Sender Base

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>

#define MULTICAST_GROUP "239.1.1.1"    // Indirizzo multicast privato
#define MULTICAST_PORT 12345            // Porta multicast
#define DEFAULT_TTL 1                   // TTL locale (solo LAN)
#define BUFFER_SIZE 1024

typedef struct {
    int socket_fd;                      // File descriptor socket
    struct sockaddr_in multicast_addr;  // Indirizzo gruppo multicast
    int ttl;                            // Time To Live
    char interface_ip[16];              // IP interfaccia di uscita
    int sent_packets;                   // Contatore pacchetti inviati
    time_t start_time;                  // Timestamp avvio
} MulticastSender;

// Crea e configura socket multicast sender
MulticastSender* create_multicast_sender(const char* group_ip, int port, int ttl) {
    MulticastSender* sender = malloc(sizeof(MulticastSender));
    if (!sender) {
        perror("malloc sender");
        return NULL;
    }
    
    // 1. CREA SOCKET UDP
    sender->socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sender->socket_fd < 0) {
        perror("socket");
        free(sender);
        return NULL;
    }
    
    // 2. CONFIGURA INDIRIZZO MULTICAST
    memset(&sender->multicast_addr, 0, sizeof(sender->multicast_addr));
    sender->multicast_addr.sin_family = AF_INET;
    sender->multicast_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, group_ip, &sender->multicast_addr.sin_addr) <= 0) {
        printf("❌ Indirizzo multicast non valido: %s\n", group_ip);
        close(sender->socket_fd);
        free(sender);
        return NULL;
    }
    
    // 3. IMPOSTA TTL MULTICAST
    sender->ttl = ttl;
    if (setsockopt(sender->socket_fd, IPPROTO_IP, IP_MULTICAST_TTL, 
                   &sender->ttl, sizeof(sender->ttl)) < 0) {
        perror("setsockopt TTL");
        close(sender->socket_fd);
        free(sender);
        return NULL;
    }
    
    // 4. IMPOSTA INTERFACCIA DI USCITA (opzionale)
    struct in_addr interface_addr;
    interface_addr.s_addr = INADDR_ANY;  // Usa interfaccia predefinita
    
    if (setsockopt(sender->socket_fd, IPPROTO_IP, IP_MULTICAST_IF,
                   &interface_addr, sizeof(interface_addr)) < 0) {
        perror("setsockopt interface");
        close(sender->socket_fd);
        free(sender);
        return NULL;
    }
    
    // 5. INIZIALIZZA STATISTICHE
    sender->sent_packets = 0;
    sender->start_time = time(NULL);
    strcpy(sender->interface_ip, "0.0.0.0");
    
    printf("✅ Multicast sender creato\n");
    printf("📡 Gruppo: %s:%d\n", group_ip, port);
    printf("🔄 TTL: %d\n", ttl);
    printf("🆔 Socket FD: %d\n", sender->socket_fd);
    
    return sender;
}

// Invia messaggio multicast
int send_multicast_message(MulticastSender* sender, const char* message) {
    if (!sender || !message) {
        return -1;
    }
    
    // Aggiungi timestamp al messaggio
    char timestamped_message[BUFFER_SIZE];
    time_t now = time(NULL);
    snprintf(timestamped_message, BUFFER_SIZE, 
             "[%ld] %s", now, message);
    
    // Invia pacchetto multicast
    int bytes_sent = sendto(sender->socket_fd, 
                           timestamped_message, strlen(timestamped_message), 0,
                           (struct sockaddr*)&sender->multicast_addr, 
                           sizeof(sender->multicast_addr));
    
    if (bytes_sent < 0) {
        perror("sendto multicast");
        return -1;
    }
    
    sender->sent_packets++;
    
    printf("📤 Multicast [#%d]: '%s' (%d bytes)\n", 
           sender->sent_packets, message, bytes_sent);
    
    return bytes_sent;
}

// Invia dati binari multicast
int send_multicast_data(MulticastSender* sender, const void* data, size_t data_size) {
    if (!sender || !data || data_size == 0) {
        return -1;
    }
    
    int bytes_sent = sendto(sender->socket_fd, data, data_size, 0,
                           (struct sockaddr*)&sender->multicast_addr,
                           sizeof(sender->multicast_addr));
    
    if (bytes_sent < 0) {
        perror("sendto multicast data");
        return -1;
    }
    
    sender->sent_packets++;
    printf("📤 Multicast binario [#%d]: %zu bytes\n", sender->sent_packets, data_size);
    
    return bytes_sent;
}

// Statistiche sender
void print_sender_stats(MulticastSender* sender) {
    if (!sender) return;
    
    time_t uptime = time(NULL) - sender->start_time;
    double rate = uptime > 0 ? (double)sender->sent_packets / uptime : 0;
    
    printf("\n📊 === STATISTICHE MULTICAST SENDER ===\n");
    printf("📡 Gruppo: %s:%d\n", 
           inet_ntoa(sender->multicast_addr.sin_addr), 
           ntohs(sender->multicast_addr.sin_port));
    printf("📦 Pacchetti inviati: %d\n", sender->sent_packets);
    printf("⏱️  Uptime: %ld secondi\n", uptime);
    printf("📈 Rate: %.2f pacchetti/sec\n", rate);
    printf("🔄 TTL: %d\n", sender->ttl);
    printf("=====================================\n\n");
}

// Cleanup sender
void destroy_multicast_sender(MulticastSender* sender) {
    if (sender) {
        close(sender->socket_fd);
        printf("🗑️  Multicast sender distrutto\n");
        free(sender);
    }
}

// Esempio sender interattivo
void interactive_sender_example() {
    MulticastSender* sender = create_multicast_sender(MULTICAST_GROUP, MULTICAST_PORT, DEFAULT_TTL);
    if (!sender) {
        return;
    }
    
    char buffer[BUFFER_SIZE];
    printf("\n🎯 Multicast Sender Interattivo\n");
    printf("💡 Comandi:\n");
    printf("   - Scrivi messaggio per inviarlo\n");
    printf("   - 'stats' per statistiche\n");
    printf("   - 'quit' per uscire\n\n");
    
    while (1) {
        printf("Messaggio> ");
        fflush(stdout);
        
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            break;
        }
        
        // Rimuovi newline
        buffer[strcspn(buffer, "\n")] = 0;
        
        if (strlen(buffer) == 0) {
            continue;
        }
        
        if (strcmp(buffer, "quit") == 0 || strcmp(buffer, "exit") == 0) {
            break;
        }
        
        if (strcmp(buffer, "stats") == 0) {
            print_sender_stats(sender);
            continue;
        }
        
        // Invia messaggio
        send_multicast_message(sender, buffer);
    }
    
    print_sender_stats(sender);
    destroy_multicast_sender(sender);
}
```

---

## 📡 Multicast Receiver Base

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>

typedef struct {
    int socket_fd;                      // File descriptor socket
    struct sockaddr_in local_addr;      // Indirizzo locale bind
    struct ip_mreq multicast_req;       // Richiesta join gruppo
    char group_ip[16];                  // IP gruppo multicast
    int port;                           // Porta multicast
    int received_packets;               // Contatore pacchetti ricevuti
    time_t start_time;                  // Timestamp avvio
    time_t last_packet_time;            // Timestamp ultimo pacchetto
} MulticastReceiver;

// Crea e configura socket multicast receiver
MulticastReceiver* create_multicast_receiver(const char* group_ip, int port) {
    MulticastReceiver* receiver = malloc(sizeof(MulticastReceiver));
    if (!receiver) {
        perror("malloc receiver");
        return NULL;
    }
    
    // 1. CREA SOCKET UDP
    receiver->socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (receiver->socket_fd < 0) {
        perror("socket");
        free(receiver);
        return NULL;
    }
    
    // 2. ABILITA RIUSO INDIRIZZO (importante per multicast)
    int reuse = 1;
    if (setsockopt(receiver->socket_fd, SOL_SOCKET, SO_REUSEADDR, 
                   &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt SO_REUSEADDR");
        close(receiver->socket_fd);
        free(receiver);
        return NULL;
    }
    
    // 3. CONFIGURA INDIRIZZO LOCALE PER BIND
    memset(&receiver->local_addr, 0, sizeof(receiver->local_addr));
    receiver->local_addr.sin_family = AF_INET;
    receiver->local_addr.sin_addr.s_addr = INADDR_ANY;  // Qualsiasi interfaccia
    receiver->local_addr.sin_port = htons(port);
    
    // 4. BIND SOCKET ALLA PORTA
    if (bind(receiver->socket_fd, (struct sockaddr*)&receiver->local_addr, 
             sizeof(receiver->local_addr)) < 0) {
        perror("bind");
        close(receiver->socket_fd);
        free(receiver);
        return NULL;
    }
    
    // 5. UNISCITI AL GRUPPO MULTICAST
    receiver->multicast_req.imr_multiaddr.s_addr = inet_addr(group_ip);
    receiver->multicast_req.imr_interface.s_addr = INADDR_ANY;  // Qualsiasi interfaccia
    
    if (setsockopt(receiver->socket_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                   &receiver->multicast_req, sizeof(receiver->multicast_req)) < 0) {
        perror("setsockopt IP_ADD_MEMBERSHIP");
        close(receiver->socket_fd);
        free(receiver);
        return NULL;
    }
    
    // 6. INIZIALIZZA DATI
    strcpy(receiver->group_ip, group_ip);
    receiver->port = port;
    receiver->received_packets = 0;
    receiver->start_time = time(NULL);
    receiver->last_packet_time = 0;
    
    printf("✅ Multicast receiver creato\n");
    printf("📡 Gruppo: %s:%d\n", group_ip, port);
    printf("🆔 Socket FD: %d\n", receiver->socket_fd);
    printf("👂 In ascolto per pacchetti multicast...\n");
    
    return receiver;
}

// Ricevi messaggio multicast
int receive_multicast_message(MulticastReceiver* receiver, char* buffer, 
                              size_t buffer_size, struct sockaddr_in* sender_addr) {
    if (!receiver || !buffer) {
        return -1;
    }
    
    socklen_t sender_len = sizeof(*sender_addr);
    
    // Ricevi pacchetto
    int bytes_received = recvfrom(receiver->socket_fd, buffer, buffer_size - 1, 0,
                                 (struct sockaddr*)sender_addr, &sender_len);
    
    if (bytes_received < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("recvfrom multicast");
        }
        return -1;
    }
    
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';  // Null-terminate per stringhe
        receiver->received_packets++;
        receiver->last_packet_time = time(NULL);
        
        printf("📥 Multicast [#%d] da %s:%d: '%s' (%d bytes)\n",
               receiver->received_packets,
               inet_ntoa(sender_addr->sin_addr),
               ntohs(sender_addr->sin_port),
               buffer, bytes_received);
    }
    
    return bytes_received;
}

// Ricevi dati binari multicast
int receive_multicast_data(MulticastReceiver* receiver, void* buffer, 
                          size_t buffer_size, struct sockaddr_in* sender_addr) {
    if (!receiver || !buffer) {
        return -1;
    }
    
    socklen_t sender_len = sizeof(*sender_addr);
    
    int bytes_received = recvfrom(receiver->socket_fd, buffer, buffer_size, 0,
                                 (struct sockaddr*)sender_addr, &sender_len);
    
    if (bytes_received < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("recvfrom multicast data");
        }
        return -1;
    }
    
    if (bytes_received > 0) {
        receiver->received_packets++;
        receiver->last_packet_time = time(NULL);
        
        printf("📥 Multicast binario [#%d] da %s:%d: %d bytes\n",
               receiver->received_packets,
               inet_ntoa(sender_addr->sin_addr),
               ntohs(sender_addr->sin_port),
               bytes_received);
    }
    
    return bytes_received;
}

// Loop ricezione continua
void receive_multicast_loop(MulticastReceiver* receiver, int timeout_seconds) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in sender_addr;
    time_t start_time = time(NULL);
    
    printf("🔄 Avvio loop ricezione (timeout: %d sec)\n", timeout_seconds);
    
    while (1) {
        // Controlla timeout
        if (timeout_seconds > 0 && 
            (time(NULL) - start_time) > timeout_seconds) {
            printf("⏰ Timeout raggiunto\n");
            break;
        }
        
        // Ricevi messaggio (non-blocking con timeout)
        fd_set read_fds;
        struct timeval tv;
        
        FD_ZERO(&read_fds);
        FD_SET(receiver->socket_fd, &read_fds);
        
        tv.tv_sec = 1;   // Timeout 1 secondo
        tv.tv_usec = 0;
        
        int activity = select(receiver->socket_fd + 1, &read_fds, NULL, NULL, &tv);
        
        if (activity < 0) {
            perror("select");
            break;
        }
        
        if (activity == 0) {
            // Timeout select - nessun dato
            continue;
        }
        
        if (FD_ISSET(receiver->socket_fd, &read_fds)) {
            receive_multicast_message(receiver, buffer, BUFFER_SIZE, &sender_addr);
        }
    }
}

// Statistiche receiver
void print_receiver_stats(MulticastReceiver* receiver) {
    if (!receiver) return;
    
    time_t uptime = time(NULL) - receiver->start_time;
    double rate = uptime > 0 ? (double)receiver->received_packets / uptime : 0;
    time_t last_activity = receiver->last_packet_time > 0 ? 
                          time(NULL) - receiver->last_packet_time : -1;
    
    printf("\n📊 === STATISTICHE MULTICAST RECEIVER ===\n");
    printf("📡 Gruppo: %s:%d\n", receiver->group_ip, receiver->port);
    printf("📦 Pacchetti ricevuti: %d\n", receiver->received_packets);
    printf("⏱️  Uptime: %ld secondi\n", uptime);
    printf("📈 Rate: %.2f pacchetti/sec\n", rate);
    if (last_activity >= 0) {
        printf("🕐 Ultimo pacchetto: %ld secondi fa\n", last_activity);
    } else {
        printf("🕐 Nessun pacchetto ricevuto\n");
    }
    printf("==========================================\n\n");
}

// Abbandona gruppo multicast
void leave_multicast_group(MulticastReceiver* receiver) {
    if (receiver) {
        if (setsockopt(receiver->socket_fd, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                       &receiver->multicast_req, sizeof(receiver->multicast_req)) < 0) {
            perror("setsockopt IP_DROP_MEMBERSHIP");
        } else {
            printf("👋 Abbandonato gruppo multicast %s\n", receiver->group_ip);
        }
    }
}

// Cleanup receiver
void destroy_multicast_receiver(MulticastReceiver* receiver) {
    if (receiver) {
        leave_multicast_group(receiver);
        close(receiver->socket_fd);
        printf("🗑️  Multicast receiver distrutto\n");
        free(receiver);
    }
}

// Esempio receiver con gestione segnali
volatile sig_atomic_t receiver_running = 1;

void receiver_signal_handler(int sig) {
    receiver_running = 0;
    printf("\n🛑 Ricevuto segnale %d - terminazione receiver\n", sig);
}

void interactive_receiver_example() {
    signal(SIGINT, receiver_signal_handler);
    signal(SIGTERM, receiver_signal_handler);
    
    MulticastReceiver* receiver = create_multicast_receiver(MULTICAST_GROUP, MULTICAST_PORT);
    if (!receiver) {
        return;
    }
    
    printf("\n🎧 Multicast Receiver Interattivo\n");
    printf("💡 Premi Ctrl+C per terminare\n\n");
    
    char buffer[BUFFER_SIZE];
    struct sockaddr_in sender_addr;
    
    while (receiver_running) {
        fd_set read_fds;
        struct timeval tv;
        
        FD_ZERO(&read_fds);
        FD_SET(receiver->socket_fd, &read_fds);
        
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        
        int activity = select(receiver->socket_fd + 1, &read_fds, NULL, NULL, &tv);
        
        if (activity > 0 && FD_ISSET(receiver->socket_fd, &read_fds)) {
            receive_multicast_message(receiver, buffer, BUFFER_SIZE, &sender_addr);
        }
    }
    
    print_receiver_stats(receiver);
    destroy_multicast_receiver(receiver);
}
```

---

## 🎯 Multicast per Traccia Esame (Da Pipe a Multicast)

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MULTICAST_GROUP "239.1.2.3"
#define MULTICAST_PORT 54321

// Struttura per processo multicast dell'esame
typedef struct {
    int multicast_socket;               // Socket multicast
    struct sockaddr_in multicast_addr;  // Indirizzo gruppo
    int pipe_fd;                        // File descriptor pipe di lettura
    int messages_sent;                  // Contatore messaggi inviati
    char process_id[32];                // Identificativo processo
} ExamMulticastProcess;

// Crea processo multicast per esame
ExamMulticastProcess* create_exam_multicast_process(int pipe_read_fd) {
    ExamMulticastProcess* proc = malloc(sizeof(ExamMulticastProcess));
    if (!proc) {
        perror("malloc multicast process");
        return NULL;
    }
    
    // 1. CREA SOCKET MULTICAST
    proc->multicast_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (proc->multicast_socket < 0) {
        perror("socket multicast");
        free(proc);
        return NULL;
    }
    
    // 2. CONFIGURA INDIRIZZO MULTICAST
    memset(&proc->multicast_addr, 0, sizeof(proc->multicast_addr));
    proc->multicast_addr.sin_family = AF_INET;
    proc->multicast_addr.sin_port = htons(MULTICAST_PORT);
    inet_pton(AF_INET, MULTICAST_GROUP, &proc->multicast_addr.sin_addr);
    
    // 3. IMPOSTA TTL MULTICAST
    int ttl = 1;  // Solo rete locale
    if (setsockopt(proc->multicast_socket, IPPROTO_IP, IP_MULTICAST_TTL,
                   &ttl, sizeof(ttl)) < 0) {
        perror("setsockopt TTL");
        close(proc->multicast_socket);
        free(proc);
        return NULL;
    }
    
    // 4. INIZIALIZZA DATI
    proc->pipe_fd = pipe_read_fd;
    proc->messages_sent = 0;
    snprintf(proc->process_id, sizeof(proc->process_id), "MULTICAST_PID_%d", getpid());
    
    printf("✅ Processo multicast esame creato\n");
    printf("📡 Gruppo: %s:%d\n", MULTICAST_GROUP, MULTICAST_PORT);
    printf("📥 Pipe FD: %d\n", pipe_read_fd);
    printf("🆔 Process ID: %s\n", proc->process_id);
    
    return proc;
}

// Funzione principale processo multicast (da usare come processo figlio)
void exam_multicast_main_loop(int pipe_read_fd) {
    ExamMulticastProcess* proc = create_exam_multicast_process(pipe_read_fd);
    if (!proc) {
        return;
    }
    
    char buffer[1024];
    char multicast_message[1200];
    
    printf("🔄 Loop multicast avviato - leggo da pipe e invio multicast\n");
    
    while (1) {
        // 1. LEGGI DALLA PIPE (bloccante)
        int bytes_read = read(proc->pipe_fd, buffer, sizeof(buffer) - 1);
        
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            
            printf("📥 Ricevuto dalla pipe: '%s'\n", buffer);
            
            // 2. PREPARA MESSAGGIO MULTICAST
            snprintf(multicast_message, sizeof(multicast_message),
                    "[%s] [MSG_%d] [TIME_%ld] %s",
                    proc->process_id,
                    ++proc->messages_sent,
                    time(NULL),
                    buffer);
            
            // 3. INVIA MULTICAST
            int bytes_sent = sendto(proc->multicast_socket,
                                   multicast_message, strlen(multicast_message), 0,
                                   (struct sockaddr*)&proc->multicast_addr,
                                   sizeof(proc->multicast_addr));
            
            if (bytes_sent < 0) {
                perror("sendto multicast");
            } else {
                printf("📡 Multicast inviato [#%d]: '%s' (%d bytes)\n",
                       proc->messages_sent, buffer, bytes_sent);
            }
            
            // 4. CANCELLA DALLA PIPE (già fatto con read)
            printf("🗑️  Messaggio cancellato dalla pipe\n");
            
        } else if (bytes_read == 0) {
            // Pipe chiusa - termina processo
            printf("🔚 Pipe chiusa - termino processo multicast\n");
            break;
        } else {
            perror("read pipe");
            break;
        }
    }
    
    // Cleanup
    close(proc->multicast_socket);
    close(proc->pipe_fd);
    printf("🛑 Processo multicast terminato (%d messaggi inviati)\n", proc->messages_sent);
    free(proc);
}

// Esempio completo pipe + multicast per esame
void exam_pipe_multicast_example() {
    int pipefd[2];
    pid_t pid;
    
    // Crea pipe
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return;
    }
    
    printf("🎯 Esempio completo Pipe → Multicast per esame\n");
    
    pid = fork();
    
    if (pid == 0) {
        // PROCESSO FIGLIO - Multicast
        close(pipefd[1]);  // Chiudi scrittura
        
        printf("👶 Processo figlio multicast avviato\n");
        exam_multicast_main_loop(pipefd[0]);
        
        exit(0);
        
    } else if (pid > 0) {
        // PROCESSO PADRE - Simula client TCP che scrive in pipe
        close(pipefd[0]);  // Chiudi lettura
        
        printf("👨 Processo padre (client TCP simulato) avviato\n");
        
        // Simula ricezione righe dal server TCP
        char* sample_lines[] = {
            "Riga 1: Prima riga dal server TCP",
            "Riga 2: Seconda riga importante",
            "Riga 3: Dati dal file di testo",
            "Riga 4: Ultima riga del test"
        };
        
        int num_lines = sizeof(sample_lines) / sizeof(sample_lines[0]);
        
        for (int i = 0; i < num_lines; i++) {
            printf("🌐 Simulo ricezione dal server TCP: '%s'\n", sample_lines[i]);
            
            // Scrivi in pipe
            write(pipefd[1], sample_lines[i], strlen(sample_lines[i]));
            
            printf("📝 Scritto in pipe: '%s'\n", sample_lines[i]);
            
            sleep(3);  // Pausa tra le righe
        }
        
        printf("👨 Padre: chiudo pipe e aspetto figlio\n");
        close(pipefd[1]);
        
        wait(NULL);
        printf("👨 Processo padre terminato\n");
        
    } else {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
    }
}
```

---

## 🛠️ Utility Multicast Avanzate

```c
// Multicast con heartbeat
typedef struct {
    time_t last_heartbeat;
    int heartbeat_interval;
    char heartbeat_message[64];
} MulticastHeartbeat;

void send_heartbeat(MulticastSender* sender, MulticastHeartbeat* hb) {
    time_t now = time(NULL);
    if (now - hb->last_heartbeat >= hb->heartbeat_interval) {
        snprintf(hb->heartbeat_message, sizeof(hb->heartbeat_message),
                "HEARTBEAT_PID_%d_TIME_%ld", getpid(), now);
        
        send_multicast_message(sender, hb->heartbeat_message);
        hb->last_heartbeat = now;
    }
}

// Multicast con buffering
typedef struct {
    char* buffer;
    size_t buffer_size;
    size_t current_pos;
    pthread_mutex_t mutex;
} MulticastBuffer;

MulticastBuffer* create_multicast_buffer(size_t size) {
    MulticastBuffer* buf = malloc(sizeof(MulticastBuffer));
    buf->buffer = malloc(size);
    buf->buffer_size = size;
    buf->current_pos = 0;
    pthread_mutex_init(&buf->mutex, NULL);
    return buf;
}

int add_to_multicast_buffer(MulticastBuffer* buf, const char* data) {
    pthread_mutex_lock(&buf->mutex);
    
    size_t data_len = strlen(data);
    if (buf->current_pos + data_len + 1 < buf->buffer_size) {
        strcpy(buf->buffer + buf->current_pos, data);
        buf->current_pos += data_len;
        buf->buffer[buf->current_pos++] = '\n';
        pthread_mutex_unlock(&buf->mutex);
        return 0;
    }
    
    pthread_mutex_unlock(&buf->mutex);
    return -1;  // Buffer pieno
}

// Multicast reliability con ACK
typedef struct {
    uint32_t sequence_number;
    time_t timestamp;
    char data[256];
} MulticastPacket;

void send_reliable_multicast(MulticastSender* sender, const char* data) {
    static uint32_t seq_num = 0;
    MulticastPacket packet;
    
    packet.sequence_number = htonl(++seq_num);
    packet.timestamp = htonl(time(NULL));
    strncpy(packet.data, data, sizeof(packet.data) - 1);
    packet.data[sizeof(packet.data) - 1] = '\0';
    
    send_multicast_data(sender, &packet, sizeof(packet));
}

// Test connettività multicast
int test_multicast_connectivity(const char* group_ip, int port) {
    printf("🔍 Test connettività multicast %s:%d\n", group_ip, port);
    
    // Verifica indirizzo multicast valido
    struct in_addr addr;
    if (inet_pton(AF_INET, group_ip, &addr) <= 0) {
        printf("❌ Indirizzo IP non valido\n");
        return -1;
    }
    
    uint32_t ip = ntohl(addr.s_addr);
    if ((ip & 0xF0000000) != 0xE0000000) {
        printf("❌ Non è un indirizzo multicast (classe D)\n");
        return -1;
    }
    
    // Test creazione socket
    int test_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (test_socket < 0) {
        printf("❌ Errore creazione socket test\n");
        return -1;
    }
    
    // Test bind porta
    struct sockaddr_in test_addr;
    memset(&test_addr, 0, sizeof(test_addr));
    test_addr.sin_family = AF_INET;
    test_addr.sin_addr.s_addr = INADDR_ANY;
    test_addr.sin_port = htons(port);
    
    if (bind(test_socket, (struct sockaddr*)&test_addr, sizeof(test_addr)) < 0) {
        printf("❌ Porta %d non disponibile\n", port);
        close(test_socket);
        return -1;
    }
    
    close(test_socket);
    printf("✅ Test connettività superato\n");
    return 0;
}
```

---

## 📋 Checklist Multicast

### ✅ **Sender Setup**
- [ ] Crea socket UDP con `socket(AF_INET, SOCK_DGRAM, 0)`
- [ ] Configura indirizzo multicast di destinazione
- [ ] Imposta TTL con `IP_MULTICAST_TTL`
- [ ] Opzionale: imposta interfaccia con `IP_MULTICAST_IF`
- [ ] Usa `sendto()` per inviare pacchetti

### ✅ **Receiver Setup**
- [ ] Crea socket UDP
- [ ] Abilita `SO_REUSEADDR` per condivisione porta
- [ ] Bind su `INADDR_ANY` con porta multicast
- [ ] Join gruppo con `IP_ADD_MEMBERSHIP`
- [ ] Usa `recvfrom()` per ricevere pacchetti
- [ ] Leave gruppo con `IP_DROP_MEMBERSHIP`

### ✅ **Best Practices**
- [ ] Usa indirizzi 239.x.x.x per reti private
- [ ] TTL=1 per reti locali, maggiore per WAN
- [ ] Gestisci errori su tutte le operazioni socket
- [ ] Implementa timeout sui receiver
- [ ] Monitora statistiche traffico

---

## 🎯 Test e Compilazione

```bash
# Compila sender
gcc -o multicast_sender sender.c

# Compila receiver  
gcc -o multicast_receiver receiver.c

# Test sender (terminale 1)
./multicast_sender

# Test receiver (terminale 2)
./multicast_receiver

# Test multiple receivers
./multicast_receiver &
./multicast_receiver &
./multicast_receiver &

# Monitor traffico multicast
tcpdump -i any host 239.1.1.1

# Test con netcat (se supporta multicast)
echo "test message" | nc -u 239.1.1.1 12345
```

## 🚀 Vantaggi Multicast

| **Vantaggio** | **Descrizione** |
|---------------|-----------------|
| **Efficienza** | Un pacchetto per N destinatari |
| **Scalabilità** | Performance costante al crescere dei receiver |
| **Bandwidth** | Risparmio notevole di banda di rete |
| **Semplicità** | API UDP standard, nessuna gestione connessioni |
| **Real-time** | Ideale per streaming e notifiche live |