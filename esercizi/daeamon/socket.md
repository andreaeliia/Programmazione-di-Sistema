# SOCKET TCP - Guida Completa per Esame

## LIBRERIE NECESSARIE

```c
#include <sys/socket.h>  // socket, bind, listen, accept, recv, send
#include <netinet/in.h>  // struct sockaddr_in, INADDR_ANY, htons
#include <arpa/inet.h>   // inet_pton, inet_ntoa, inet_addr
#include <unistd.h>      // close, read, write
#include <string.h>      // memset, strlen
#include <errno.h>       // errno, strerror
#include <stdio.h>       // printf, perror
#include <stdlib.h>      // exit
```

## COS'È UN SOCKET

### Definizione
Un **socket** è un endpoint di comunicazione che permette a processi di comunicare attraverso la rete o localmente. È come una "presa telefonica" per i programmi.

### Tipi di Socket
```c
SOCK_STREAM   // TCP - affidabile, ordinato, connection-oriented
SOCK_DGRAM    // UDP - veloce, non affidabile, connectionless
SOCK_RAW      // Accesso diretto ai protocolli di livello inferiore
```

### Famiglie di Indirizzi
```c
AF_INET       // IPv4 Internet protocols
AF_INET6      // IPv6 Internet protocols  
AF_UNIX       // Local communication (pipes)
```

## STRUTTURE DATI FONDAMENTALI

### struct sockaddr_in (IPv4)
```c
struct sockaddr_in {
    short sin_family;        // Famiglia indirizzi (AF_INET)
    u_short sin_port;        // Porta (network byte order)
    struct in_addr sin_addr; // Indirizzo IP  
    char sin_zero[8];        // Padding per compatibilità
};

struct in_addr {
    u_long s_addr;           // Indirizzo IPv4 (network byte order)
};
```

**SPIEGAZIONE CAMPI:**
- `sin_family`: Sempre `AF_INET` per IPv4
- `sin_port`: Porta in **network byte order** (usa `htons()`)
- `sin_addr.s_addr`: IP in **network byte order** (usa `inet_pton()`)
- `sin_zero`: Deve essere azzerato con `memset()`

### Costanti Utili
```c
INADDR_ANY      // 0.0.0.0 - bind su tutte le interfacce
INADDR_LOOPBACK // 127.0.0.1 - localhost
SOMAXCONN       // Massimo backlog per listen()
```

## CONVERSIONI BYTE ORDER

### Network vs Host Byte Order
- **Network Byte Order**: Big-endian (byte più significativo prima)
- **Host Byte Order**: Dipende dall'architettura (x86 è little-endian)

### Funzioni di Conversione
```c
#include <arpa/inet.h>

uint16_t htons(uint16_t hostshort);    // Host TO Network Short
uint16_t ntohs(uint16_t netshort);     // Network TO Host Short
uint32_t htonl(uint32_t hostlong);     // Host TO Network Long  
uint32_t ntohl(uint32_t netlong);      // Network TO Host Long
```

**QUANDO USARLE:**
```c
// SEMPRE quando imposti porta
server_addr.sin_port = htons(8080);

// SEMPRE quando leggi porta ricevuta
printf("Client porta: %d\n", ntohs(client_addr.sin_port));
```

### Conversioni Indirizzi IP
```c
// Stringa → Binario (MODERNO)
int inet_pton(int af, const char *src, void *dst);
// af: AF_INET o AF_INET6
// src: stringa IP ("192.168.1.1")
// dst: puntatore a struct in_addr
// RITORNA: 1 successo, 0 formato invalido, -1 errore

// Binario → Stringa (MODERNO)  
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);

// Binario → Stringa (DEPRECATO ma ancora usato)
char *inet_ntoa(struct in_addr in);

// Stringa → Binario (DEPRECATO)
in_addr_t inet_addr(const char *cp);
```

**ESEMPI:**
```c
// Moderno (preferito)
struct sockaddr_in addr;
inet_pton(AF_INET, "192.168.1.1", &addr.sin_addr);

// Per stampare IP ricevuto
printf("Client IP: %s\n", inet_ntoa(client_addr.sin_addr));
```

## FLUSSO TCP SERVER

### Schema Completo
```
1. socket()  → Crea socket
2. setsockopt() → Opzioni socket (opzionale)
3. bind()    → Associa indirizzo locale
4. listen()  → Mette in modalità ascolto
5. accept()  → Accetta connessione (BLOCCA)
6. recv/send → Scambia dati
7. close()   → Chiude connessione
```

### Dettagli di Ogni Passo

#### 1. socket() - Creazione
```c
int socket(int domain, int type, int protocol);

// PARAMETRI:
// domain: AF_INET (IPv4), AF_INET6 (IPv6)
// type: SOCK_STREAM (TCP), SOCK_DGRAM (UDP)  
// protocol: 0 (automatico)

// RITORNA:
// Successo: file descriptor (≥ 0)
// Errore: -1 (setta errno)

// ESEMPIO:
int server_fd = socket(AF_INET, SOCK_STREAM, 0);
if (server_fd < 0) {
    perror("socket failed");
    exit(1);
}
```

#### 2. setsockopt() - Opzioni Socket
```c
int setsockopt(int sockfd, int level, int optname, 
               const void *optval, socklen_t optlen);

// OPZIONE IMPORTANTE: SO_REUSEADDR
int opt = 1;
if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, 
               &opt, sizeof(opt)) < 0) {
    perror("setsockopt failed");
}
```

**PERCHÉ SO_REUSEADDR:**
- Evita errore "Address already in use"
- Permette restart immediato del server
- Riusa porta anche se in stato TIME_WAIT

#### 3. bind() - Associazione Indirizzo
```c
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

// ESEMPIO COMPLETO:
struct sockaddr_in server_addr;
memset(&server_addr, 0, sizeof(server_addr));  // IMPORTANTE: azzera
server_addr.sin_family = AF_INET;
server_addr.sin_addr.s_addr = INADDR_ANY;      // Tutte interfacce
server_addr.sin_port = htons(8080);            // Porta 8080

if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    perror("bind failed");
    close(server_fd);
    exit(1);
}
```

**ERRORI COMUNI BIND:**
- Porta già in uso (errno = EADDRINUSE)
- Permessi insufficienti (porte < 1024 richiedono root)
- Indirizzo non valido

#### 4. listen() - Modalità Ascolto
```c
int listen(int sockfd, int backlog);

// backlog: Massimo connessioni in coda
// Tipico: 5-10 per applicazioni normali

if (listen(server_fd, 5) < 0) {
    perror("listen failed");
    close(server_fd);
    exit(1);
}
```

**COSA FA:**
- Marca il socket come "passivo" (accetta connessioni)
- Crea coda per connessioni in arrivo
- Non blocca, solo configura

#### 5. accept() - Accettazione Connessioni
```c
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

// ESEMPIO COMPLETO:
struct sockaddr_in client_addr;
socklen_t client_len = sizeof(client_addr);

int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
if (client_fd < 0) {
    perror("accept failed");
    // NON uscire, continua ad accettare altre connessioni
} else {
    printf("Client connesso da %s:%d\n", 
           inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port));
}
```

**CARATTERISTICHE ACCEPT:**
- **BLOCCA** fino a nuova connessione
- Ritorna **nuovo file descriptor** per il client
- Il server socket rimane aperto per altre connessioni
- Può fallire senza compromettere il server

#### 6. recv() e send() - Scambio Dati
```c
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
ssize_t send(int sockfd, const void *buf, size_t len, int flags);

// flags: Generalmente 0, opzioni speciali:
MSG_DONTWAIT  // Non bloccare
MSG_PEEK      // Leggi senza rimuovere dal buffer
MSG_WAITALL   // Aspetta tutti i byte richiesti

// ESEMPIO RECV:
char buffer[1024];
ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer)-1, 0);
if (bytes_received > 0) {
    buffer[bytes_received] = '\0';  // Null-terminate
    printf("Ricevuto: %s\n", buffer);
} else if (bytes_received == 0) {
    printf("Client ha chiuso la connessione\n");
} else {
    perror("recv failed");
}

// ESEMPIO SEND:
const char *response = "Echo: ";
if (send(client_fd, response, strlen(response), 0) < 0) {
    perror("send failed");
}
```

**VALORI RITORNO RECV:**
- `> 0`: Numero byte ricevuti
- `= 0`: Connessione chiusa dal peer
- `< 0`: Errore (controllare errno)

#### 7. close() - Chiusura
```c
close(client_fd);  // Chiude connessione client
close(server_fd);  // Chiude server socket
```

## FLUSSO TCP CLIENT

### Schema Client
```
1. socket()   → Crea socket
2. connect()  → Connette al server
3. send/recv  → Scambia dati
4. close()    → Chiude connessione
```

### Implementazione Client
```c
int client_fd = socket(AF_INET, SOCK_STREAM, 0);

struct sockaddr_in server_addr;
memset(&server_addr, 0, sizeof(server_addr));
server_addr.sin_family = AF_INET;
server_addr.sin_port = htons(8080);

// Converte indirizzo IP
if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
    perror("inet_pton failed");
    exit(1);
}

// Connette al server
if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    perror("connect failed");
    exit(1);
}

// Invia dati
char *message = "Hello Server!";
send(client_fd, message, strlen(message), 0);

// Riceve risposta
char buffer[1024];
recv(client_fd, buffer, sizeof(buffer), 0);

close(client_fd);
```

## ESEMPI PRATICI

### Esempio 1: Echo Server Base
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    
    // 1. Crea socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(1);
    }
    
    // 2. Riusa indirizzo
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // 3. Configura indirizzo
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // 4. Bind
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(1);
    }
    
    // 5. Listen
    if (listen(server_fd, 5) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(1);
    }
    
    printf("Server in ascolto sulla porta %d...\n", PORT);
    
    // 6. Accept e gestione client
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }
        
        printf("Client connesso: %s:%d\n", 
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));
        
        // Echo loop
        while (1) {
            memset(buffer, 0, BUFFER_SIZE);
            ssize_t bytes = recv(client_fd, buffer, BUFFER_SIZE-1, 0);
            
            if (bytes <= 0) {
                printf("Client disconnesso\n");
                break;
            }
            
            printf("Ricevuto: %s", buffer);
            send(client_fd, buffer, bytes, 0);  // Echo back
        }
        
        close(client_fd);
    }
    
    close(server_fd);
    return 0;
}
```

### Esempio 2: Client di Test
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    
    // 1. Crea socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket failed");
        exit(1);
    }
    
    // 2. Configura indirizzo server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton failed");
        exit(1);
    }
    
    // 3. Connetti
    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        exit(1);
    }
    
    printf("Connesso al server. Digita messaggi:\n");
    
    // 4. Loop comunicazione
    while (1) {
        printf("> ");
        if (!fgets(buffer, BUFFER_SIZE, stdin)) break;
        
        if (strncmp(buffer, "quit", 4) == 0) break;
        
        // Invia messaggio
        send(sock_fd, buffer, strlen(buffer), 0);
        
        // Ricevi risposta
        memset(response, 0, BUFFER_SIZE);
        ssize_t bytes = recv(sock_fd, response, BUFFER_SIZE-1, 0);
        if (bytes > 0) {
            printf("Echo: %s", response);
        }
    }
    
    close(sock_fd);
    return 0;
}
```

### Esempio 3: Server Multi-Porta
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define NUM_PORTS 3
#define BASE_PORT 8080
#define BUFFER_SIZE 1024

int create_server_socket(int port) {
    int server_fd;
    struct sockaddr_in server_addr;
    int opt = 1;
    
    // Crea socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        return -1;
    }
    
    // Riusa indirizzo
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Configura indirizzo
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    // Bind
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server_fd);
        return -1;
    }
    
    // Listen
    if (listen(server_fd, 5) < 0) {
        perror("listen failed");
        close(server_fd);
        return -1;
    }
    
    printf("Server socket creato sulla porta %d\n", port);
    return server_fd;
}

int main() {
    int server_fds[NUM_PORTS];
    
    // Crea socket su porte multiple
    for (int i = 0; i < NUM_PORTS; i++) {
        server_fds[i] = create_server_socket(BASE_PORT + i);
        if (server_fds[i] < 0) {
            exit(1);
        }
    }
    
    printf("Server in ascolto su porte %d-%d\n", 
           BASE_PORT, BASE_PORT + NUM_PORTS - 1);
    
    // In un vero server useresti select() qui
    // Per ora gestiamo solo il primo socket
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    
    while (1) {
        client_fd = accept(server_fds[0], (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) continue;
        
        printf("Client connesso alla porta %d\n", BASE_PORT);
        
        while (1) {
            ssize_t bytes = recv(client_fd, buffer, BUFFER_SIZE-1, 0);
            if (bytes <= 0) break;
            
            buffer[bytes] = '\0';
            printf("Ricevuto: %s", buffer);
            send(client_fd, buffer, bytes, 0);
        }
        
        close(client_fd);
    }
    
    // Pulizia
    for (int i = 0; i < NUM_PORTS; i++) {
        close(server_fds[i]);
    }
    
    return 0;
}
```

## GESTIONE ERRORI SOCKET

### Errori Comuni e Significato
```c
EADDRINUSE     // Address already in use (bind)
ECONNREFUSED   // Connection refused (connect)
ECONNRESET     // Connection reset by peer (recv/send)
EPIPE          // Broken pipe (send su connessione chiusa)
ETIMEDOUT      // Connection timed out
ENOBUFS        // No buffer space available
```

### Pattern di Gestione Errori
```c
int server_fd = socket(AF_INET, SOCK_STREAM, 0);
if (server_fd < 0) {
    switch (errno) {
        case EAFNOSUPPORT:
            fprintf(stderr, "Famiglia indirizzi non supportata\n");
            break;
        case EMFILE:
            fprintf(stderr, "Troppi file descriptor aperti\n");
            break;
        default:
            perror("socket");
    }
    exit(1);
}
```

### Recupero da Errori Non Fatali
```c
// Accept che continua dopo errori
while (1) {
    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        if (errno == EINTR) {
            // Interrotto da segnale, riprova
            continue;
        }
        perror("accept");
        continue;  // Non uscire, continua ad accettare
    }
    
    // Gestisci client...
}
```

## DEBUGGING SOCKET

### Strumenti di Debug
```bash
# Verifica porte in ascolto
netstat -tulpn | grep :8080
ss -tulpn | grep :8080

# Test connessione
telnet localhost 8080
nc localhost 8080

# Monitora traffico di rete
tcpdump -i lo port 8080
wireshark (interfaccia grafica)
```

### Debug nel Codice
```c
// Abilita debug socket
int debug = 1;
setsockopt(sock_fd, SOL_SOCKET, SO_DEBUG, &debug, sizeof(debug));

// Controlla dimensione buffer
int bufsize;
socklen_t optlen = sizeof(bufsize);
getsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, &bufsize, &optlen);
printf("Buffer ricezione: %d bytes\n", bufsize);

// Verifica stato connessione
int error;
socklen_t len = sizeof(error);
getsockopt(sock_fd, SOL_SOCKET, SO_ERROR, &error, &len);
if (error != 0) {
    printf("Errore socket: %s\n", strerror(error));
}
```

## SOCKET NON-BLOCCANTI

### Modalità Non-Bloccante
```c
#include <fcntl.h>

// Rendi socket non-bloccante
int flags = fcntl(sock_fd, F_GETFL, 0);
fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);

// Accept non-bloccante
int client_fd = accept(server_fd, NULL, NULL);
if (client_fd < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // Nessuna connessione disponibile, continua
    } else {
        perror("accept");
    }
}
```

### Recv/Send Non-Bloccanti
```c
ssize_t bytes = recv(sock_fd, buffer, sizeof(buffer), MSG_DONTWAIT);
if (bytes < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // Nessun dato disponibile
    } else {
        perror("recv");
    }
}
```

## OPZIONI SOCKET AVANZATE

### Timeout su Operazioni
```c
// Timeout su recv
struct timeval timeout;
timeout.tv_sec = 5;   // 5 secondi
timeout.tv_usec = 0;
setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

// Timeout su send  
setsockopt(sock_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
```

### Keep-Alive
```c
// Abilita keep-alive
int keepalive = 1;
setsockopt(sock_fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));

// Configura parametri keep-alive (Linux)
int keepidle = 60;    // Inizia dopo 60 sec inattività
int keepintvl = 10;   // Intervallo tra probe: 10 sec  
int keepcnt = 3;      // Max 3 probe falliti

setsockopt(sock_fd, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(keepidle));
setsockopt(sock_fd, IPPROTO_TCP, TCP_KEEPINTVL, &keepintvl, sizeof(keepintvl));
setsockopt(sock_fd, IPPROTO_TCP, TCP_KEEPCNT, &keepcnt, sizeof(keepcnt));
```

### Dimensione Buffer
```c
// Aumenta buffer di ricezione
int bufsize = 65536;  // 64KB
setsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));

// Aumenta buffer di invio
setsockopt(sock_fd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));
```

## ERRORI COMUNI E SOLUZIONI

### 1. Address Already in Use
```c
// PROBLEMA: bind() fallisce con EADDRINUSE
// SOLUZIONE: Usa SO_REUSEADDR
int opt = 1;
setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
```

### 2. Broken Pipe
```c
// PROBLEMA: send() su connessione chiusa causa SIGPIPE
// SOLUZIONE: Ignora SIGPIPE e controlla EPIPE
signal(SIGPIPE, SIG_IGN);

if (send(sock_fd, buffer, len, 0) < 0) {
    if (errno == EPIPE) {
        printf("Connessione chiusa dal peer\n");
    }
}
```

### 3. Buffer Non Null-Terminated
```c
// PROBLEMA: recv() non null-termina la stringa
char buffer[1024];
ssize_t bytes = recv(sock_fd, buffer, sizeof(buffer)-1, 0);
if (bytes > 0) {
    buffer[bytes] = '\0';  // IMPORTANTE: null-terminate
    printf("Ricevuto: %s\n", buffer);
}
```

### 4. Non Controllare Valore Ritorno
```c
// SBAGLIATO
send(sock_fd, buffer, len, 0);

// GIUSTO  
if (send(sock_fd, buffer, len, 0) < 0) {
    perror("send failed");
    // Gestisci errore
}
```

### 5. Dimenticare Network Byte Order
```c
// SBAGLIATO
server_addr.sin_port = 8080;

// GIUSTO
server_addr.sin_port = htons(8080);
```

## CHECKLIST ESAME

✅ **Include**: sys/socket.h, netinet/in.h, arpa/inet.h
✅ **socket()**: Controlla sempre valore ritorno
✅ **SO_REUSEADDR**: Evita "Address already in use"
✅ **memset()**: Azzera struct sockaddr_in prima dell'uso
✅ **htons()**: Converti porta in network byte order
✅ **INADDR_ANY**: Per bind su tutte le interfacce
✅ **Controllo errori**: Su tutte le funzioni socket
✅ **close()**: Chiudi sempre i file descriptor
✅ **Null-terminate**: Buffer ricevuti con recv()
✅ **perror()**: Per debug errori di sistema

## TEMPLATE COMPLETO SERVER

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int create_server_socket(int port) {
    int server_fd;
    struct sockaddr_in server_addr;
    int opt = 1;
    
    // Crea socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        return -1;
    }
    
    // Riusa indirizzo
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(server_fd);
        return -1;
    }
    
    // Configura indirizzo
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    // Bind
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server_fd);
        return -1;
    }
    
    // Listen
    if (listen(server_fd, 5) < 0) {
        perror("listen failed");
        close(server_fd);
        return -1;
    }
    
    return server_fd;
}

void handle_client(int client_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes;
    
    while ((bytes = recv(client_fd, buffer, BUFFER_SIZE-1, 0)) > 0) {
        buffer[bytes] = '\0';
        printf("Ricevuto: %s", buffer);
        
        if (send(client_fd, buffer, bytes, 0) < 0) {
            perror("send failed");
            break;
        }
    }
    
    if (bytes < 0) {
        perror("recv failed");
    }
    
    close(client_fd);
}

int main() {
    int server_fd = create_server_socket(PORT);
    if (server_fd < 0) {
        exit(1);
    }
    
    printf("Server in ascolto sulla porta %d\n", PORT);
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }
        
        printf("Client connesso: %s:%d\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));
        
        handle_client(client_fd);
    }
    
    close(server_fd);
    return 0;
}
```