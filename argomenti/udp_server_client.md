# UDP Client/Server - Riferimento Completo

## 🎯 Concetti Fondamentali

### **UDP vs TCP**
- **Connectionless**: nessuna connessione stabilita
- **Unreliable**: nessuna garanzia di consegna
- **Fast**: basso overhead, veloce
- **Stateless**: ogni pacchetto indipendente
- **Message-oriented**: preserva confini dei messaggi

### **Caratteristiche UDP**
- **No handshaking**: invio immediato
- **No ordering**: pacchetti possono arrivare disordinati
- **No duplicate protection**: possibili duplicati
- **Fixed header**: header di 8 byte
- **Broadcasting/Multicasting**: supporto nativo

---

## 🔧 Server UDP Base

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
#include <signal.h>

#define SERVER_PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100

// Struttura per tracciare client UDP
typedef struct {
    struct sockaddr_in address;     // Indirizzo client
    time_t last_seen;              // Timestamp ultimo pacchetto
    unsigned int packet_count;      // Contatore pacchetti da questo client
    char client_id[32];            // ID identificativo
} UDPClient;

// Struttura server UDP
typedef struct {
    int socket_fd;                 // Socket server
    struct sockaddr_in address;    // Indirizzo server
    UDPClient clients[MAX_CLIENTS]; // Array client tracciati
    int client_count;              // Numero client attivi
    unsigned long packets_received; // Contatore totale pacchetti
    unsigned long packets_sent;     // Contatore pacchetti inviati
    time_t start_time;             // Timestamp avvio server
    volatile sig_atomic_t running; // Flag per terminazione pulita
} UDPServer;

UDPServer* global_server = NULL;

// Signal handler per terminazione pulita
void signal_handler(int sig) {
    if (global_server) {
        global_server->running = 0;
        printf("\n🛑 Ricevuto segnale %d - terminazione server\n", sig);
    }
}

// Crea server UDP
UDPServer* create_udp_server(int port) {
    UDPServer* server = malloc(sizeof(UDPServer));
    if (!server) {
        perror("malloc server");
        return NULL;
    }
    
    // Inizializza struttura
    memset(server, 0, sizeof(UDPServer));
    server->running = 1;
    server->start_time = time(NULL);
    
    // 1. CREA SOCKET UDP
    server->socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server->socket_fd < 0) {
        perror("socket");
        free(server);
        return NULL;
    }
    
    // 2. CONFIGURA OPZIONI SOCKET
    int opt = 1;
    if (setsockopt(server->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_REUSEADDR");
        close(server->socket_fd);
        free(server);
        return NULL;
    }
    
    // 3. CONFIGURA INDIRIZZO SERVER
    memset(&server->address, 0, sizeof(server->address));
    server->address.sin_family = AF_INET;
    server->address.sin_addr.s_addr = INADDR_ANY;
    server->address.sin_port = htons(port);
    
    // 4. BIND SOCKET
    if (bind(server->socket_fd, (struct sockaddr*)&server->address, 
             sizeof(server->address)) < 0) {
        perror("bind");
        close(server->socket_fd);
        free(server);
        return NULL;
    }
    
    printf("✅ Server UDP creato e bindato su porta %d\n", port);
    printf("🆔 Socket FD: %d\n", server->socket_fd);
    printf("📡 In ascolto per pacchetti UDP...\n");
    
    return server;
}

// Trova o crea entry per client
UDPClient* find_or_create_client(UDPServer* server, struct sockaddr_in* client_addr) {
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr->sin_addr, client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(client_addr->sin_port);
    
    // Cerca client esistente
    for (int i = 0; i < server->client_count; i++) {
        if (server->clients[i].address.sin_addr.s_addr == client_addr->sin_addr.s_addr &&
            server->clients[i].address.sin_port == client_addr->sin_port) {
            
            // Client trovato - aggiorna timestamp
            server->clients[i].last_seen = time(NULL);
            server->clients[i].packet_count++;
            return &server->clients[i];
        }
    }
    
    // Client nuovo - crea entry se c'è spazio
    if (server->client_count < MAX_CLIENTS) {
        UDPClient* new_client = &server->clients[server->client_count];
        
        new_client->address = *client_addr;
        new_client->last_seen = time(NULL);
        new_client->packet_count = 1;
        snprintf(new_client->client_id, sizeof(new_client->client_id), 
                "%s:%d", client_ip, client_port);
        
        server->client_count++;
        
        printf("🆕 Nuovo client UDP: %s (totale: %d)\n", 
               new_client->client_id, server->client_count);
        
        return new_client;
    }
    
    printf("⚠️  Troppi client - ignoro %s:%d\n", client_ip, client_port);
    return NULL;
}

// Processa messaggio ricevuto
void process_udp_message(UDPServer* server, const char* message, size_t message_len,
                        struct sockaddr_in* client_addr) {
    UDPClient* client = find_or_create_client(server, client_addr);
    if (!client) return;
    
    server->packets_received++;
    
    printf("📥 [%s] Pacchetto #%u: '%.*s' (%zu bytes)\n",
           client->client_id, client->packet_count, (int)message_len, message, message_len);
    
    // ELABORAZIONE MESSAGGI SPECIFICI
    char response[BUFFER_SIZE];
    int response_len = 0;
    
    if (strncmp(message, "PING", 4) == 0) {
        // Comando PING
        response_len = snprintf(response, BUFFER_SIZE, 
                               "PONG from server (your packet #%u)", client->packet_count);
        
    } else if (strncmp(message, "TIME", 4) == 0) {
        // Comando TIME
        time_t now = time(NULL);
        char* time_str = ctime(&now);
        time_str[strlen(time_str) - 1] = '\0';  // Rimuovi newline
        
        response_len = snprintf(response, BUFFER_SIZE, 
                               "Server time: %s", time_str);
        
    } else if (strncmp(message, "STATS", 5) == 0) {
        // Comando STATS
        time_t uptime = time(NULL) - server->start_time;
        response_len = snprintf(response, BUFFER_SIZE,
                               "Server stats: %lu packets received, %lu sent, %d clients, %ld sec uptime",
                               server->packets_received, server->packets_sent, 
                               server->client_count, uptime);
        
    } else if (strncmp(message, "ECHO:", 5) == 0) {
        // Comando ECHO
        response_len = snprintf(response, BUFFER_SIZE, 
                               "Echo: %.*s", (int)(message_len - 5), message + 5);
        
    } else {
        // Messaggio generico - echo con timestamp
        response_len = snprintf(response, BUFFER_SIZE,
                               "[%ld] Echo: %.*s", time(NULL), (int)message_len, message);
    }
    
    // INVIA RISPOSTA
    if (response_len > 0) {
        int bytes_sent = sendto(server->socket_fd, response, response_len, 0,
                               (struct sockaddr*)client_addr, sizeof(*client_addr));
        
        if (bytes_sent > 0) {
            server->packets_sent++;
            printf("📤 [%s] Risposta: '%s' (%d bytes)\n", 
                   client->client_id, response, bytes_sent);
        } else {
            printf("❌ Errore invio risposta a %s: %s\n", 
                   client->client_id, strerror(errno));
        }
    }
}

// Rimuovi client inattivi
void cleanup_inactive_clients(UDPServer* server, int timeout_seconds) {
    time_t now = time(NULL);
    int removed = 0;
    
    for (int i = 0; i < server->client_count; i++) {
        if (now - server->clients[i].last_seen > timeout_seconds) {
            printf("🗑️  Rimuovo client inattivo: %s (last seen %ld sec ago)\n",
                   server->clients[i].client_id, now - server->clients[i].last_seen);
            
            // Sposta ultimo client in questa posizione
            server->clients[i] = server->clients[server->client_count - 1];
            server->client_count--;
            removed++;
            i--;  // Ricontrolla questa posizione
        }
    }
    
    if (removed > 0) {
        printf("🧹 Rimossi %d client inattivi (attivi: %d)\n", removed, server->client_count);
    }
}

// Stampa statistiche server
void print_server_stats(UDPServer* server) {
    time_t uptime = time(NULL) - server->start_time;
    double recv_rate = uptime > 0 ? (double)server->packets_received / uptime : 0;
    double sent_rate = uptime > 0 ? (double)server->packets_sent / uptime : 0;
    
    printf("\n📊 === STATISTICHE SERVER UDP ===\n");
    printf("⏱️  Uptime: %ld secondi\n", uptime);
    printf("📥 Pacchetti ricevuti: %lu (%.2f/sec)\n", server->packets_received, recv_rate);
    printf("📤 Pacchetti inviati: %lu (%.2f/sec)\n", server->packets_sent, sent_rate);
    printf("👥 Client attivi: %d/%d\n", server->client_count, MAX_CLIENTS);
    
    if (server->client_count > 0) {
        printf("📋 Lista client:\n");
        for (int i = 0; i < server->client_count; i++) {
            UDPClient* c = &server->clients[i];
            time_t inactive = time(NULL) - c->last_seen;
            printf("   %d. %s - %u pacchetti, last seen %ld sec ago\n",
                   i + 1, c->client_id, c->packet_count, inactive);
        }
    }
    printf("================================\n\n");
}

// Main loop server
void udp_server_main_loop(UDPServer* server) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    time_t last_cleanup = time(NULL);
    time_t last_stats = time(NULL);
    
    printf("🔄 Server UDP main loop avviato\n");
    printf("💡 Comandi supportati: PING, TIME, STATS, ECHO:<message>\n\n");
    
    while (server->running) {
        // Configura timeout per recvfrom
        fd_set read_fds;
        struct timeval timeout;
        
        FD_ZERO(&read_fds);
        FD_SET(server->socket_fd, &read_fds);
        
        timeout.tv_sec = 1;   // Timeout 1 secondo
        timeout.tv_usec = 0;
        
        // Aspetta pacchetti con timeout
        int activity = select(server->socket_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (activity < 0) {
            if (errno != EINTR) {
                perror("select");
                break;
            }
            continue;
        }
        
        if (activity > 0 && FD_ISSET(server->socket_fd, &read_fds)) {
            // Ricevi pacchetto UDP
            int bytes_received = recvfrom(server->socket_fd, buffer, BUFFER_SIZE - 1, 0,
                                         (struct sockaddr*)&client_addr, &client_addr_len);
            
            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';  // Null-terminate
                process_udp_message(server, buffer, bytes_received, &client_addr);
                
            } else if (bytes_received < 0) {
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    perror("recvfrom");
                }
            }
        }
        
        // Cleanup periodico client inattivi (ogni 30 secondi)
        time_t now = time(NULL);
        if (now - last_cleanup >= 30) {
            cleanup_inactive_clients(server, 120);  // Timeout 2 minuti
            last_cleanup = now;
        }
        
        // Statistiche periodiche (ogni 60 secondi)
        if (now - last_stats >= 60) {
            print_server_stats(server);
            last_stats = now;
        }
    }
    
    printf("🛑 Server UDP main loop terminato\n");
}

// Cleanup server
void destroy_udp_server(UDPServer* server) {
    if (server) {
        print_server_stats(server);
        close(server->socket_fd);
        printf("🗑️  Server UDP distrutto\n");
        free(server);
    }
}

// Main server
int main() {
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN);
    
    printf("🚀 Avvio Server UDP\n");
    
    // Crea server
    global_server = create_udp_server(SERVER_PORT);
    if (!global_server) {
        exit(EXIT_FAILURE);
    }
    
    printf("📡 Server UDP pronto su porta %d\n", SERVER_PORT);
    printf("💡 Test con: echo 'PING' | nc -u localhost %d\n\n", SERVER_PORT);
    
    // Main loop
    udp_server_main_loop(global_server);
    
    // Cleanup
    destroy_udp_server(global_server);
    
    printf("👋 Server UDP terminato\n");
    return 0;
}
```

---

## 📱 Client UDP Base

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

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

// Struttura client UDP
typedef struct {
    int socket_fd;                  // Socket client
    struct sockaddr_in server_addr; // Indirizzo server
    unsigned int packets_sent;      // Contatore pacchetti inviati
    unsigned int packets_received;  // Contatore pacchetti ricevuti
    time_t start_time;              // Timestamp avvio
    char client_id[32];             // ID identificativo client
} UDPClient;

// Crea client UDP
UDPClient* create_udp_client(const char* server_ip, int server_port) {
    UDPClient* client = malloc(sizeof(UDPClient));
    if (!client) {
        perror("malloc client");
        return NULL;
    }
    
    // Inizializza struttura
    memset(client, 0, sizeof(UDPClient));
    client->start_time = time(NULL);
    snprintf(client->client_id, sizeof(client->client_id), "CLIENT_%d", getpid());
    
    // 1. CREA SOCKET UDP
    client->socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client->socket_fd < 0) {
        perror("socket");
        free(client);
        return NULL;
    }
    
    // 2. CONFIGURA INDIRIZZO SERVER
    memset(&client->server_addr, 0, sizeof(client->server_addr));
    client->server_addr.sin_family = AF_INET;
    client->server_addr.sin_port = htons(server_port);
    
    if (inet_pton(AF_INET, server_ip, &client->server_addr.sin_addr) <= 0) {
        printf("❌ Indirizzo IP server non valido: %s\n", server_ip);
        close(client->socket_fd);
        free(client);
        return NULL;
    }
    
    printf("✅ Client UDP creato\n");
    printf("🎯 Server target: %s:%d\n", server_ip, server_port);
    printf("🆔 Client ID: %s\n", client->client_id);
    
    return client;
}

// Invia messaggio al server con timeout
int send_udp_message(UDPClient* client, const char* message, char* response, 
                     size_t response_size, int timeout_seconds) {
    if (!client || !message) {
        return -1;
    }
    
    // Invia messaggio
    int bytes_sent = sendto(client->socket_fd, message, strlen(message), 0,
                           (struct sockaddr*)&client->server_addr, 
                           sizeof(client->server_addr));
    
    if (bytes_sent < 0) {
        printf("❌ Errore invio messaggio: %s\n", strerror(errno));
        return -1;
    }
    
    client->packets_sent++;
    printf("📤 Inviato [#%u]: '%s' (%d bytes)\n", 
           client->packets_sent, message, bytes_sent);
    
    // Ricevi risposta con timeout
    if (response && response_size > 0) {
        fd_set read_fds;
        struct timeval timeout;
        
        FD_ZERO(&read_fds);
        FD_SET(client->socket_fd, &read_fds);
        
        timeout.tv_sec = timeout_seconds;
        timeout.tv_usec = 0;
        
        int activity = select(client->socket_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (activity > 0 && FD_ISSET(client->socket_fd, &read_fds)) {
            struct sockaddr_in from_addr;
            socklen_t from_len = sizeof(from_addr);
            
            int bytes_received = recvfrom(client->socket_fd, response, response_size - 1, 0,
                                        (struct sockaddr*)&from_addr, &from_len);
            
            if (bytes_received > 0) {
                response[bytes_received] = '\0';
                client->packets_received++;
                
                printf("📥 Risposta [#%u]: '%s' (%d bytes)\n",
                       client->packets_received, response, bytes_received);
                
                return bytes_received;
            } else {
                printf("❌ Errore ricezione risposta: %s\n", strerror(errno));
                return -1;
            }
        } else if (activity == 0) {
            printf("⏰ Timeout ricezione risposta (%d sec)\n", timeout_seconds);
            return 0;
        } else {
            printf("❌ Errore select: %s\n", strerror(errno));
            return -1;
        }
    }
    
    return bytes_sent;
}

// Test connettività con server
int test_server_connectivity(UDPClient* client) {
    char response[BUFFER_SIZE];
    
    printf("🔍 Test connettività server...\n");
    
    // Test PING
    int result = send_udp_message(client, "PING", response, sizeof(response), 3);
    if (result > 0) {
        printf("✅ Server risponde: %s\n", response);
        return 0;
    } else if (result == 0) {
        printf("⏰ Server non risponde (timeout)\n");
        return -1;
    } else {
        printf("❌ Errore comunicazione con server\n");
        return -1;
    }
}

// Client interattivo
void interactive_client_mode(UDPClient* client) {
    char input[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    
    printf("\n🎮 Modalità Client Interattivo\n");
    printf("💡 Comandi speciali:\n");
    printf("   - 'ping' = test connettività\n");
    printf("   - 'time' = richiedi ora server\n");
    printf("   - 'stats' = statistiche server\n");
    printf("   - 'echo:<msg>' = echo messaggio\n");
    printf("   - 'quit' = esci\n\n");
    
    while (1) {
        printf("%s> ", client->client_id);
        fflush(stdout);
        
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        
        // Rimuovi newline
        input[strcspn(input, "\n")] = 0;
        
        if (strlen(input) == 0) {
            continue;
        }
        
        if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0) {
            break;
        }
        
        // Converti comandi speciali in formato server
        if (strcmp(input, "ping") == 0) {
            strcpy(input, "PING");
        } else if (strcmp(input, "time") == 0) {
            strcpy(input, "TIME");
        } else if (strcmp(input, "stats") == 0) {
            strcpy(input, "STATS");
        } else if (strncmp(input, "echo:", 5) == 0) {
            memmove(input, input + 5, strlen(input) - 4);  // Rimuovi "echo:"
            char temp[BUFFER_SIZE];
            snprintf(temp, sizeof(temp), "ECHO:%s", input);
            strcpy(input, temp);
        }
        
        // Invia messaggio e ricevi risposta
        int result = send_udp_message(client, input, response, sizeof(response), 5);
        
        if (result == 0) {
            printf("⏰ Nessuna risposta dal server\n");
        } else if (result < 0) {
            printf("❌ Errore comunicazione\n");
        }
        
        printf("\n");
    }
}

// Test automatico
void automatic_test_mode(UDPClient* client, int num_tests) {
    char message[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    int success_count = 0;
    
    printf("\n🤖 Modalità Test Automatico (%d test)\n", num_tests);
    
    for (int i = 1; i <= num_tests; i++) {
        snprintf(message, sizeof(message), "Auto test message #%d from %s", i, client->client_id);
        
        int result = send_udp_message(client, message, response, sizeof(response), 3);
        
        if (result > 0) {
            success_count++;
            printf("✅ Test %d/%d: SUCCESS\n", i, num_tests);
        } else {
            printf("❌ Test %d/%d: FAILED\n", i, num_tests);
        }
        
        if (i < num_tests) {
            sleep(1);  // Pausa tra test
        }
    }
    
    printf("\n📊 Risultati test: %d/%d successi (%.1f%%)\n", 
           success_count, num_tests, (float)success_count * 100 / num_tests);
}

// Stampa statistiche client
void print_client_stats(UDPClient* client) {
    time_t uptime = time(NULL) - client->start_time;
    double sent_rate = uptime > 0 ? (double)client->packets_sent / uptime : 0;
    double recv_rate = uptime > 0 ? (double)client->packets_received / uptime : 0;
    double loss_rate = client->packets_sent > 0 ? 
                      (double)(client->packets_sent - client->packets_received) * 100 / client->packets_sent : 0;
    
    printf("\n📊 === STATISTICHE CLIENT UDP ===\n");
    printf("🆔 Client ID: %s\n", client->client_id);
    printf("⏱️  Uptime: %ld secondi\n", uptime);
    printf("📤 Pacchetti inviati: %u (%.2f/sec)\n", client->packets_sent, sent_rate);
    printf("📥 Pacchetti ricevuti: %u (%.2f/sec)\n", client->packets_received, recv_rate);
    printf("📉 Packet loss: %.1f%%\n", loss_rate);
    printf("===============================\n\n");
}

// Cleanup client
void destroy_udp_client(UDPClient* client) {
    if (client) {
        print_client_stats(client);
        close(client->socket_fd);
        printf("🗑️  Client UDP distrutto\n");
        free(client);
    }
}

// Main client
int main(int argc, char* argv[]) {
    printf("🚀 Client UDP\n");
    
    // Parsing argomenti
    const char* server_ip = SERVER_IP;
    int server_port = SERVER_PORT;
    int test_mode = 0;
    int num_tests = 10;
    
    if (argc > 1) {
        if (strcmp(argv[1], "--auto") == 0) {
            test_mode = 1;
            if (argc > 2) {
                num_tests = atoi(argv[2]);
            }
        } else if (strcmp(argv[1], "--help") == 0) {
            printf("Uso:\n");
            printf("  %s                    # Modalità interattiva\n", argv[0]);
            printf("  %s --auto [num]       # Test automatico (default: 10)\n", argv[0]);
            printf("  %s --help             # Mostra questo aiuto\n", argv[0]);
            return 0;
        }
    }
    
    // Crea client
    UDPClient* client = create_udp_client(server_ip, server_port);
    if (!client) {
        exit(EXIT_FAILURE);
    }
    
    // Test connettività
    if (test_server_connectivity(client) < 0) {
        printf("⚠️  Server potrebbe non essere disponibile\n");
    }
    
    // Modalità operativa
    if (test_mode) {
        automatic_test_mode(client, num_tests);
    } else {
        interactive_client_mode(client);
    }
    
    // Cleanup
    destroy_udp_client(client);
    
    printf("👋 Client UDP terminato\n");
    return 0;
}
```

---

## 🎯 UDP Broadcast

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BROADCAST_PORT 9999
#define BROADCAST_IP "255.255.255.255"

// Sender broadcast
int udp_broadcast_sender() {
    int sockfd;
    struct sockaddr_in broadcast_addr;
    int broadcast_enable = 1;
    char message[256];
    
    // Crea socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket broadcast");
        return -1;
    }
    
    // Abilita broadcast
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, 
                   &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        perror("setsockopt SO_BROADCAST");
        close(sockfd);
        return -1;
    }
    
    // Configura indirizzo broadcast
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(BROADCAST_PORT);
    inet_pton(AF_INET, BROADCAST_IP, &broadcast_addr.sin_addr);
    
    printf("📡 UDP Broadcast Sender avviato\n");
    printf("🎯 Target: %s:%d\n", BROADCAST_IP, BROADCAST_PORT);
    
    // Invia messaggi broadcast
    for (int i = 1; i <= 5; i++) {
        snprintf(message, sizeof(message), 
                "Broadcast message #%d from PID %d at %ld", 
                i, getpid(), time(NULL));
        
        int bytes_sent = sendto(sockfd, message, strlen(message), 0,
                               (struct sockaddr*)&broadcast_addr, 
                               sizeof(broadcast_addr));
        
        if (bytes_sent > 0) {
            printf("📤 Broadcast [#%d]: '%s' (%d bytes)\n", i, message, bytes_sent);
        } else {
            printf("❌ Errore broadcast #%d: %s\n", i, strerror(errno));
        }
        
        sleep(2);
    }
    
    close(sockfd);
    return 0;
}

// Receiver broadcast
int udp_broadcast_receiver() {
    int sockfd;
    struct sockaddr_in local_addr, sender_addr;
    socklen_t sender_len = sizeof(sender_addr);
    char buffer[1024];
    int reuse = 1;
    
    // Crea socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket broadcast receiver");
        return -1;
    }
    
    // Abilita riuso indirizzo
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt SO_REUSEADDR");
        close(sockfd);
        return -1;
    }
    
    // Bind su tutte le interfacce
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(BROADCAST_PORT);
    
    if (bind(sockfd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        perror("bind broadcast receiver");
        close(sockfd);
        return -1;
    }
    
    printf("👂 UDP Broadcast Receiver avviato su porta %d\n", BROADCAST_PORT);
    printf("⏳ In ascolto per messaggi broadcast...\n");
    
    // Loop ricezione
    while (1) {
        int bytes_received = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                                     (struct sockaddr*)&sender_addr, &sender_len);
        
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            printf("📥 Broadcast da %s:%d: '%s' (%d bytes)\n",
                   inet_ntoa(sender_addr.sin_addr), 
                   ntohs(sender_addr.sin_port),
                   buffer, bytes_received);
        } else {
            printf("❌ Errore ricezione broadcast: %s\n", strerror(errno));
            break;
        }
    }
    
    close(sockfd);
    return 0;
}
```

---

## 🛠️ Utility UDP Avanzate

```c
// UDP con checksum custom
typedef struct {
    uint32_t sequence;
    uint32_t timestamp;
    uint16_t checksum;
    uint16_t data_length;
    char data[];
} UDPPacket;

uint16_t calculate_checksum(const void* data, size_t length) {
    const uint16_t* ptr = (const uint16_t*)data;
    uint32_t sum = 0;
    
    while (length > 1) {
        sum += *ptr++;
        length -= 2;
    }
    
    if (length == 1) {
        sum += *(const uint8_t*)ptr;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

// UDP reliable con retry
int udp_send_reliable(int sockfd, const void* data, size_t data_len,
                     const struct sockaddr* dest_addr, socklen_t addr_len,
                     int max_retries, int timeout_ms) {
    
    for (int retry = 0; retry < max_retries; retry++) {
        // Invia pacchetto
        int sent = sendto(sockfd, data, data_len, 0, dest_addr, addr_len);
        if (sent < 0) {
            continue;
        }
        
        // Aspetta ACK con timeout
        fd_set read_fds;
        struct timeval timeout;
        
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);
        
        timeout.tv_sec = timeout_ms / 1000;
        timeout.tv_usec = (timeout_ms % 1000) * 1000;
        
        int activity = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (activity > 0) {
            char ack[16];
            struct sockaddr_in from;
            socklen_t from_len = sizeof(from);
            
            int received = recvfrom(sockfd, ack, sizeof(ack), 0,
                                  (struct sockaddr*)&from, &from_len);
            
            if (received > 0 && strncmp(ack, "ACK", 3) == 0) {
                return sent;  // Success
            }
        }
        
        printf("⚠️  Retry %d/%d...\n", retry + 1, max_retries);
    }
    
    return -1;  // Failed after all retries
}

// UDP con buffering
typedef struct {
    char* buffer;
    size_t size;
    size_t used;
    pthread_mutex_t mutex;
} UDPBuffer;

UDPBuffer* create_udp_buffer(size_t size) {
    UDPBuffer* buf = malloc(sizeof(UDPBuffer));
    buf->buffer = malloc(size);
    buf->size = size;
    buf->used = 0;
    pthread_mutex_init(&buf->mutex, NULL);
    return buf;
}

int udp_buffer_add(UDPBuffer* buf, const void* data, size_t data_len) {
    pthread_mutex_lock(&buf->mutex);
    
    if (buf->used + data_len <= buf->size) {
        memcpy(buf->buffer + buf->used, data, data_len);
        buf->used += data_len;
        pthread_mutex_unlock(&buf->mutex);
        return 0;
    }
    
    pthread_mutex_unlock(&buf->mutex);
    return -1;  // Buffer full
}

int udp_buffer_flush(UDPBuffer* buf, int sockfd, const struct sockaddr* dest) {
    pthread_mutex_lock(&buf->mutex);
    
    if (buf->used > 0) {
        int sent = sendto(sockfd, buf->buffer, buf->used, 0, dest, sizeof(struct sockaddr_in));
        if (sent > 0) {
            buf->used = 0;  // Clear buffer
        }
        pthread_mutex_unlock(&buf->mutex);
        return sent;
    }
    
    pthread_mutex_unlock(&buf->mutex);
    return 0;
}

// UDP connection tracking
typedef struct {
    struct sockaddr_in address;
    time_t last_seen;
    uint32_t packets_sent;
    uint32_t packets_received;
    uint32_t bytes_sent;
    uint32_t bytes_received;
} UDPConnection;

typedef struct {
    UDPConnection* connections;
    int max_connections;
    int connection_count;
    pthread_mutex_t mutex;
} UDPConnectionTracker;

UDPConnectionTracker* create_udp_tracker(int max_connections) {
    UDPConnectionTracker* tracker = malloc(sizeof(UDPConnectionTracker));
    tracker->connections = calloc(max_connections, sizeof(UDPConnection));
    tracker->max_connections = max_connections;
    tracker->connection_count = 0;
    pthread_mutex_init(&tracker->mutex, NULL);
    return tracker;
}

UDPConnection* udp_tracker_find_or_create(UDPConnectionTracker* tracker,
                                         const struct sockaddr_in* addr) {
    pthread_mutex_lock(&tracker->mutex);
    
    // Cerca connessione esistente
    for (int i = 0; i < tracker->connection_count; i++) {
        UDPConnection* conn = &tracker->connections[i];
        if (conn->address.sin_addr.s_addr == addr->sin_addr.s_addr &&
            conn->address.sin_port == addr->sin_port) {
            conn->last_seen = time(NULL);
            pthread_mutex_unlock(&tracker->mutex);
            return conn;
        }
    }
    
    // Crea nuova connessione se c'è spazio
    if (tracker->connection_count < tracker->max_connections) {
        UDPConnection* conn = &tracker->connections[tracker->connection_count];
        conn->address = *addr;
        conn->last_seen = time(NULL);
        conn->packets_sent = conn->packets_received = 0;
        conn->bytes_sent = conn->bytes_received = 0;
        tracker->connection_count++;
        pthread_mutex_unlock(&tracker->mutex);
        return conn;
    }
    
    pthread_mutex_unlock(&tracker->mutex);
    return NULL;  // No space
}
```

---

## 📋 Checklist UDP Programming

### ✅ **Server UDP**
- [ ] `socket(AF_INET, SOCK_DGRAM, 0)` per creare socket
- [ ] `setsockopt(SO_REUSEADDR)` per riuso indirizzo
- [ ] `bind()` su INADDR_ANY per accettare da tutti
- [ ] `recvfrom()` per ricevere con indirizzo mittente
- [ ] `sendto()` per rispondere al mittente
- [ ] Timeout con `select()` per non bloccare

### ✅ **Client UDP**
- [ ] Socket UDP con indirizzo server configurato
- [ ] `sendto()` per inviare al server
- [ ] `recvfrom()` per ricevere risposta (opzionale)
- [ ] Gestione timeout su ricezione
- [ ] Retry logic per reliability

### ✅ **Best Practices**
- [ ] Gestisci packet loss (UDP è unreliable)
- [ ] Implementa timeout su tutte le operazioni
- [ ] Traccia connessioni per statistiche
- [ ] Cleanup di connessioni inattive
- [ ] Validazione dati ricevuti

---

## 🎯 Compilazione e Test

```bash
# Compila server
gcc -o udp_server server.c -lpthread

# Compila client
gcc -o udp_client client.c

# Test base
./udp_server &
./udp_client

# Test con netcat
echo "PING" | nc -u localhost 8080

# Test broadcast
gcc -o udp_broadcast broadcast.c
./udp_broadcast receiver &
./udp_broadcast sender

# Test multiple client
for i in {1..10}; do ./udp_client --auto 5 & done
```

## 🚀 UDP vs TCP - Quando Usare

| **Scenario** | **Protocollo** | **Motivo** |
|--------------|----------------|------------|
| **File transfer** | TCP | Reliability necessaria |
| **Gaming real-time** | UDP | Velocità più importante |
| **Streaming video** | UDP | Latenza bassa |
| **Chat messages** | TCP | Nessun messaggio perso |
| **DNS queries** | UDP | Semplice richiesta/risposta |
| **HTTP/Web** | TCP | Reliability e ordering |