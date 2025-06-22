# SELECT - Guida Completa I/O Multiplexing

## LIBRERIE NECESSARIE

```c
#include <sys/select.h>  // select, fd_set, FD_* macros
#include <sys/time.h>    // struct timeval
#include <sys/types.h>   // fd_set (su alcuni sistemi)
#include <unistd.h>      // close, read, write
#include <errno.h>       // errno, EINTR
#include <stdio.h>       // printf, perror
#include <string.h>      // memset
```

## PROBLEMA RISOLTO DA SELECT

### Server TCP Tradizionale - Limitazioni
```c
// PROBLEMA: Server può gestire UN SOLO client
while (1) {
    int client = accept(server_fd, ...);  // BLOCCA qui
    
    while (1) {
        recv(client, ...);                // BLOCCA qui
        // Altri client non possono connettersi!
    }
}
```

### Soluzione con select()
```c
// SOLUZIONE: Monitora TUTTI i socket contemporaneamente
fd_set read_fds;
while (1) {
    select(..., &read_fds, ...);  // BLOCCA su TUTTI i socket
    
    // Controlla quale socket ha attività
    if (FD_ISSET(server_fd, &read_fds)) {
        // Nuova connessione
    }
    
    for (client in clients) {
        if (FD_ISSET(client, &read_fds)) {
            // Questo client ha inviato dati
        }
    }
}
```

## COS'È SELECT

### Definizione
**select()** è una system call che permette di monitorare **multipli file descriptor** contemporaneamente, bloccando fino a quando almeno uno diventa "pronto" per I/O.

### Tipi di Monitoraggio
- **Lettura**: File descriptor pronti per `read()` senza bloccare
- **Scrittura**: File descriptor pronti per `write()` senza bloccare  
- **Eccezioni**: File descriptor con condizioni eccezionali

### Vantaggi
- **Un solo thread** gestisce molti client
- **Efficiente**: Non spreca CPU su polling attivo
- **Scalabile**: Può gestire centinaia di connessioni
- **Portabile**: Disponibile su tutti i sistemi UNIX-like

## STRUTTURE DATI

### fd_set - Set di File Descriptor
```c
fd_set read_fds;    // Set per monitoraggio lettura
fd_set write_fds;   // Set per monitoraggio scrittura
fd_set except_fds;  // Set per monitoraggio eccezioni
```

**IMPLEMENTAZIONE INTERNA:**
- Array di bit, ogni bit rappresenta un file descriptor
- Dimensione fissa: `FD_SETSIZE` (tipicamente 1024)
- Non accedere direttamente, usa le macro

### Macro per Gestione fd_set
```c
void FD_ZERO(fd_set *set);                // Pulisce il set
void FD_SET(int fd, fd_set *set);         // Aggiunge fd al set
void FD_CLR(int fd, fd_set *set);         // Rimuove fd dal set
int  FD_ISSET(int fd, fd_set *set);       // Controlla se fd è nel set
```

**ESEMPIO:**
```c
fd_set read_fds;
FD_ZERO(&read_fds);           // Pulisce
FD_SET(server_fd, &read_fds); // Aggiunge server socket
FD_SET(client_fd, &read_fds); // Aggiunge client socket

// Dopo select()
if (FD_ISSET(server_fd, &read_fds)) {
    // Server socket ha nuova connessione
}
```

### struct timeval - Timeout
```c
struct timeval {
    long tv_sec;     // Secondi
    long tv_usec;    // Microsecondi (1/1000000 secondo)
};

// ESEMPI:
struct timeval timeout;
timeout.tv_sec = 5;      // 5 secondi
timeout.tv_usec = 500000; // + 0.5 secondi = 5.5 secondi totali

timeout.tv_sec = 0;
timeout.tv_usec = 100000; // 0.1 secondi (100ms)
```

## FUNZIONE SELECT

### Prototipo
```c
int select(int nfds, fd_set *readfds, fd_set *writefds, 
           fd_set *exceptfds, struct timeval *timeout);
```

### Parametri Dettagliati

#### nfds
```c
// Numero massimo file descriptor + 1
int max_fd = 0;
FD_SET(server_fd, &read_fds);
if (server_fd > max_fd) max_fd = server_fd;

FD_SET(client_fd, &read_fds);  
if (client_fd > max_fd) max_fd = client_fd;

select(max_fd + 1, &read_fds, NULL, NULL, NULL);
//     ^^^^^^^^^^ IMPORTANTE: +1
```

**PERCHÉ +1:**
- select() controlla FD da 0 a (nfds-1)
- Se max FD è 5, devi passare 6 per includere FD 5

#### readfds
```c
// File descriptor da monitorare per lettura
fd_set read_fds;
FD_ZERO(&read_fds);
FD_SET(server_fd, &read_fds);  // Nuove connessioni
FD_SET(client_fd, &read_fds);  // Dati da client
```

#### writefds
```c
// File descriptor da monitorare per scrittura
// Generalmente NULL per server TCP semplici
// Usato quando il buffer di output è pieno
```

#### exceptfds
```c
// File descriptor da monitorare per eccezioni
// Generalmente NULL per applicazioni normali
// Esempi: out-of-band data, errori socket
```

#### timeout
```c
// NULL: Blocca indefinitamente
select(max_fd+1, &read_fds, NULL, NULL, NULL);

// Timeout specifico
struct timeval timeout = {5, 0};  // 5 secondi
select(max_fd+1, &read_fds, NULL, NULL, &timeout);

// Non bloccare (polling)
struct timeval timeout = {0, 0};  // 0 secondi
select(max_fd+1, &read_fds, NULL, NULL, &timeout);
```

### Valori di Ritorno
```c
int result = select(...);

if (result > 0) {
    // Numero di FD pronti
    printf("%d file descriptor pronti\n", result);
} else if (result == 0) {
    // Timeout scaduto, nessun FD pronto
    printf("Timeout\n");
} else {
    // Errore (result == -1)
    if (errno == EINTR) {
        // Interrotto da segnale, riprova
    } else {
        perror("select failed");
    }
}
```

## COMPORTAMENTO SELECT

### Modifica dei Set
**IMPORTANTE:** `select()` **modifica** i set di input!

```c
fd_set master_fds, read_fds;

// Setup iniziale
FD_ZERO(&master_fds);
FD_SET(server_fd, &master_fds);
FD_SET(client_fd, &master_fds);

while (1) {
    read_fds = master_fds;  // COPIA il set master
    
    select(max_fd+1, &read_fds, NULL, NULL, NULL);
    
    // read_fds ora contiene SOLO i FD pronti
    // master_fds rimane inalterato
}
```

### Gestione File Descriptor Pronti
```c
// Controlla tutti i possibili FD
for (int fd = 0; fd <= max_fd; fd++) {
    if (FD_ISSET(fd, &read_fds)) {
        if (fd == server_fd) {
            // Nuova connessione
            handle_new_connection();
        } else {
            // Dati da client esistente
            handle_client_data(fd);
        }
    }
}
```

## PATTERN TIPICO SELECT

### Struttura Completa
```c
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define MAX_CLIENTS 100

int main() {
    int server_fd, client_fds[MAX_CLIENTS];
    fd_set master_fds, read_fds;
    int max_fd;
    
    // 1. INIZIALIZZAZIONE
    server_fd = create_server_socket(8080);
    
    // Inizializza array client
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_fds[i] = -1;  // -1 = slot vuoto
    }
    
    // Inizializza set master
    FD_ZERO(&master_fds);
    FD_SET(server_fd, &master_fds);
    max_fd = server_fd;
    
    // 2. MAIN LOOP
    while (1) {
        // Copia set master
        read_fds = master_fds;
        
        // Aspetta attività
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        
        if (activity < 0) {
            perror("select failed");
            break;
        }
        
        // 3. CONTROLLA SERVER SOCKET
        if (FD_ISSET(server_fd, &read_fds)) {
            handle_new_connection(server_fd, client_fds, &master_fds, &max_fd);
        }
        
        // 4. CONTROLLA CLIENT SOCKET
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int fd = client_fds[i];
            if (fd != -1 && FD_ISSET(fd, &read_fds)) {
                if (handle_client_data(fd) <= 0) {
                    // Client disconnesso
                    close(fd);
                    FD_CLR(fd, &master_fds);
                    client_fds[i] = -1;
                }
            }
        }
    }
    
    return 0;
}
```

### Funzioni di Supporto
```c
void handle_new_connection(int server_fd, int client_fds[], 
                          fd_set *master_fds, int *max_fd) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int new_client = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (new_client < 0) {
        perror("accept failed");
        return;
    }
    
    // Trova slot libero
    int slot = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_fds[i] == -1) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        printf("Troppi client, connessione rifiutata\n");
        close(new_client);
        return;
    }
    
    // Aggiungi client
    client_fds[slot] = new_client;
    FD_SET(new_client, master_fds);
    if (new_client > *max_fd) {
        *max_fd = new_client;
    }
    
    printf("Nuovo client slot %d (fd %d): %s:%d\n", 
           slot, new_client,
           inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port));
}

int handle_client_data(int client_fd) {
    char buffer[1024];
    ssize_t bytes = recv(client_fd, buffer, sizeof(buffer)-1, 0);
    
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("Client %d: %s", client_fd, buffer);
        
        // Echo back
        send(client_fd, buffer, bytes, 0);
        return bytes;
    } else if (bytes == 0) {
        printf("Client %d disconnesso\n", client_fd);
        return 0;
    } else {
        perror("recv failed");
        return -1;
    }
}
```

## ESEMPI PRATICI

### Esempio 1: Echo Server con Select
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

int create_server_socket(int port) {
    int server_fd;
    struct sockaddr_in server_addr;
    int opt = 1;
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        return -1;
    }
    
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server_fd);
        return -1;
    }
    
    if (listen(server_fd, 5) < 0) {
        perror("listen failed");
        close(server_fd);
        return -1;
    }
    
    return server_fd;
}

int main() {
    int server_fd, client_fds[MAX_CLIENTS];
    fd_set master_fds, read_fds;
    int max_fd, activity;
    char buffer[BUFFER_SIZE];
    
    // Inizializzazione
    server_fd = create_server_socket(PORT);
    if (server_fd < 0) {
        exit(1);
    }
    
    printf("Server select in ascolto sulla porta %d\n", PORT);
    
    // Inizializza array client
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_fds[i] = -1;
    }
    
    // Inizializza set master
    FD_ZERO(&master_fds);
    FD_SET(server_fd, &master_fds);
    max_fd = server_fd;
    
    // Main loop
    while (1) {
        // Copia set master
        read_fds = master_fds;
        
        printf("Aspetto attività su %d socket...\n", max_fd + 1);
        activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        
        if (activity < 0 && errno != EINTR) {
            perror("select failed");
            break;
        }
        
        // Controlla server socket per nuove connessioni
        if (FD_ISSET(server_fd, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            
            int new_client = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (new_client < 0) {
                perror("accept failed");
                continue;
            }
            
            printf("Nuova connessione da %s:%d (fd %d)\n",
                   inet_ntoa(client_addr.sin_addr),
                   ntohs(client_addr.sin_port),
                   new_client);
            
            // Trova slot libero
            int slot = -1;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_fds[i] == -1) {
                    slot = i;
                    break;
                }
            }
            
            if (slot == -1) {
                printf("Troppi client, connessione rifiutata\n");
                close(new_client);
            } else {
                client_fds[slot] = new_client;
                FD_SET(new_client, &master_fds);
                if (new_client > max_fd) {
                    max_fd = new_client;
                }
                printf("Client assegnato allo slot %d\n", slot);
                
                // Messaggio di benvenuto
                char welcome[] = "Benvenuto nel server select!\n";
                send(new_client, welcome, strlen(welcome), 0);
            }
        }
        
        // Controlla dati dai client esistenti
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int client_fd = client_fds[i];
            
            if (client_fd != -1 && FD_ISSET(client_fd, &read_fds)) {
                memset(buffer, 0, BUFFER_SIZE);
                ssize_t bytes = recv(client_fd, buffer, BUFFER_SIZE-1, 0);
                
                if (bytes <= 0) {
                    // Client disconnesso
                    if (bytes == 0) {
                        printf("Client slot %d disconnesso\n", i);
                    } else {
                        perror("recv failed");
                    }
                    
                    close(client_fd);
                    FD_CLR(client_fd, &master_fds);
                    client_fds[i] = -1;
                } else {
                    printf("Slot %d: %s", i, buffer);
                    
                    // Echo a tutti gli altri client
                    char broadcast[BUFFER_SIZE + 50];
                    snprintf(broadcast, sizeof(broadcast), 
                             "[Client %d]: %s", i, buffer);
                    
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (client_fds[j] != -1 && j != i) {
                            send(client_fds[j], broadcast, strlen(broadcast), 0);
                        }
                    }
                    
                    // Conferma al mittente
                    char ack[] = "Messaggio ricevuto\n";
                    send(client_fd, ack, strlen(ack), 0);
                }
            }
        }
    }
    
    // Pulizia
    close(server_fd);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_fds[i] != -1) {
            close(client_fds[i]);
        }
    }
    
    return 0;
}
```

### Esempio 2: Server Multi-Porta con Select
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define NUM_PORTS 3
#define BASE_PORT 8080
#define MAX_CLIENTS 20
#define BUFFER_SIZE 1024

int main() {
    int server_fds[NUM_PORTS];
    int client_fds[MAX_CLIENTS];
    fd_set master_fds, read_fds;
    int max_fd = 0;
    
    // Crea server socket per ogni porta
    for (int i = 0; i < NUM_PORTS; i++) {
        server_fds[i] = create_server_socket(BASE_PORT + i);
        if (server_fds[i] < 0) {
            exit(1);
        }
        printf("Server socket %d sulla porta %d\n", server_fds[i], BASE_PORT + i);
    }
    
    // Inizializza array client
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_fds[i] = -1;
    }
    
    // Inizializza set master con tutti i server socket
    FD_ZERO(&master_fds);
    for (int i = 0; i < NUM_PORTS; i++) {
        FD_SET(server_fds[i], &master_fds);
        if (server_fds[i] > max_fd) {
            max_fd = server_fds[i];
        }
    }
    
    printf("Server multi-porta in ascolto su porte %d-%d\n", 
           BASE_PORT, BASE_PORT + NUM_PORTS - 1);
    
    while (1) {
        read_fds = master_fds;
        
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("select failed");
            break;
        }
        
        // Controlla ogni server socket
        for (int s = 0; s < NUM_PORTS; s++) {
            if (FD_ISSET(server_fds[s], &read_fds)) {
                // Nuova connessione su porta s
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                
                int new_client = accept(server_fds[s], 
                                       (struct sockaddr*)&client_addr, 
                                       &client_len);
                if (new_client < 0) {
                    perror("accept failed");
                    continue;
                }
                
                printf("Connessione su porta %d da %s:%d\n",
                       BASE_PORT + s,
                       inet_ntoa(client_addr.sin_addr),
                       ntohs(client_addr.sin_port));
                
                // Trova slot libero
                int slot = -1;
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (client_fds[i] == -1) {
                        slot = i;
                        break;
                    }
                }
                
                if (slot == -1) {
                    printf("Troppi client\n");
                    close(new_client);
                } else {
                    client_fds[slot] = new_client;
                    FD_SET(new_client, &master_fds);
                    if (new_client > max_fd) {
                        max_fd = new_client;
                    }
                    
                    char welcome[100];
                    snprintf(welcome, sizeof(welcome),
                             "Connesso alla porta %d (slot %d)\n",
                             BASE_PORT + s, slot);
                    send(new_client, welcome, strlen(welcome), 0);
                }
            }
        }
        
        // Controlla client esistenti
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int client_fd = client_fds[i];
            
            if (client_fd != -1 && FD_ISSET(client_fd, &read_fds)) {
                char buffer[BUFFER_SIZE];
                ssize_t bytes = recv(client_fd, buffer, BUFFER_SIZE-1, 0);
                
                if (bytes <= 0) {
                    printf("Client slot %d disconnesso\n", i);
                    close(client_fd);
                    FD_CLR(client_fd, &master_fds);
                    client_fds[i] = -1;
                } else {
                    buffer[bytes] = '\0';
                    printf("Slot %d: %s", i, buffer);
                    send(client_fd, buffer, bytes, 0);  // Echo
                }
            }
        }
    }
    
    return 0;
}
```

## TIMEOUT E GESTIONE TEMPO

### Select con Timeout
```c
struct timeval timeout;
timeout.tv_sec = 5;      // 5 secondi
timeout.tv_usec = 0;

int result = select(max_fd+1, &read_fds, NULL, NULL, &timeout);

if (result == 0) {
    printf("Timeout: nessuna attività per 5 secondi\n");
    // Fai manutenzione, statistiche, etc.
} else if (result > 0) {
    // Gestisci FD pronti
} else {
    // Errore
}
```

### Polling Non-Bloccante
```c
struct timeval timeout = {0, 0};  // Non bloccare

int result = select(max_fd+1, &read_fds, NULL, NULL, &timeout);

if (result == 0) {
    // Nessun FD pronto, fai altro lavoro
    do_other_work();
} else {
    // Gestisci FD pronti
}
```

### Timeout Periodico per Statistiche
```c
struct timeval timeout;
time_t last_stats = time(NULL);

while (1) {
    timeout.tv_sec = 10;   // Check ogni 10 secondi
    timeout.tv_usec = 0;
    
    read_fds = master_fds;
    int result = select(max_fd+1, &read_fds, NULL, NULL, &timeout);
    
    if (result == 0) {
        // Timeout - stampa statistiche
        time_t now = time(NULL);
        if (now - last_stats >= 30) {  // Ogni 30 secondi
            print_statistics();
            last_stats = now;
        }
    } else {
        // Gestisci attività di rete
    }
}
```

## GESTIONE ERRORI E CASI SPECIALI

### Interruzione da Segnali
```c
int result = select(max_fd+1, &read_fds, NULL, NULL, NULL);

if (result < 0) {
    if (errno == EINTR) {
        // Interrotto da segnale (es. SIGCHLD)
        printf("Select interrotta da segnale\n");
        continue;  // Riprova select
    } else {
        perror("select failed");
        break;
    }
}
```

### File Descriptor Invalidi
```c
// Se un FD viene chiuso ma non rimosso dal set
if (result < 0 && errno == EBADF) {
    printf("File descriptor invalido nel set\n");
    
    // Ricostruisci il set rimuovendo FD invalidi
    rebuild_fd_set(&master_fds, &max_fd);
}
```

### Aggiornamento max_fd Dinamico
```c
void update_max_fd(int *max_fd, int client_fds[], int num_clients) {
    *max_fd = server_fd;  // Parti dal server FD
    
    for (int i = 0; i < num_clients; i++) {
        if (client_fds[i] != -1 && client_fds[i] > *max_fd) {
            *max_fd = client_fds[i];
        }
    }
}

// Chiamala dopo ogni disconnessione
if (bytes <= 0) {
    close(client_fd);
    FD_CLR(client_fd, &master_fds);
    client_fds[i] = -1;
    
    update_max_fd(&max_fd, client_fds, MAX_CLIENTS);
}
```

## ALTERNATIVE A SELECT

### poll() - Più Moderno
```c
#include <poll.h>

struct pollfd fds[MAX_FDS];
fds[0].fd = server_fd;
fds[0].events = POLLIN;  // Monitora lettura

int result = poll(fds, num_fds, timeout_ms);
```

### epoll() - Linux Specifico
```c
#include <sys/epoll.h>

int epoll_fd = epoll_create1(0);
struct epoll_event event;
event.events = EPOLLIN;
event.data.fd = server_fd;
epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);

struct epoll_event events[MAX_EVENTS];
int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, timeout);
```

### Confronto Performance
- **select()**: Limite FD_SETSIZE (~1024), O(n) scan
- **poll()**: Nessun limite fisso, O(n) scan  
- **epoll()**: Nessun limite, O(1) notifica (solo Linux)

## ERRORI COMUNI E SOLUZIONI

### 1. Non Copiare fd_set Prima di select()
```c
// SBAGLIATO - select() modifica read_fds
while (1) {
    select(max_fd+1, &read_fds, NULL, NULL, NULL);  // read_fds modificato!
}

// GIUSTO - copia il set master
while (1) {
    read_fds = master_fds;  // Copia prima di ogni select
    select(max_fd+1, &read_fds, NULL, NULL, NULL);
}
```

### 2. Dimenticare di Aggiornare max_fd
```c
// SBAGLIATO - max_fd non aggiornato
FD_SET(new_client, &master_fds);

// GIUSTO - aggiorna max_fd
FD_SET(new_client, &master_fds);
if (new_client > max_fd) {
    max_fd = new_client;
}
```

### 3. Non Rimuovere FD Chiusi dal Set
```c
// SBAGLIATO - FD rimane nel set dopo chiusura
close(client_fd);

// GIUSTO - rimuovi dal set
close(client_fd);
FD_CLR(client_fd, &master_fds);
```

### 4. Gestione Errata di EINTR
```c
// SBAGLIATO - esce su EINTR
if (select(...) < 0) {
    perror("select");
    exit(1);
}

// GIUSTO - continua su EINTR
if (select(...) < 0) {
    if (errno == EINTR) continue;
    perror("select");
    exit(1);
}
```

### 5. Buffer Non Azzerato
```c
// SBAGLIATO - buffer contiene dati vecchi
char buffer[1024];
recv(client_fd, buffer, 1024, 0);

// GIUSTO - azzera buffer
char buffer[1024];
memset(buffer, 0, sizeof(buffer));
recv(client_fd, buffer, sizeof(buffer)-1, 0);
```

## CHECKLIST ESAME

✅ **Include**: sys/select.h, sys/time.h
✅ **FD_ZERO**: Inizializza set prima dell'uso  
✅ **Copia set**: read_fds = master_fds prima di select()
✅ **max_fd**: Sempre max FD + 1
✅ **FD_SET**: Aggiungi FD al set
✅ **FD_CLR**: Rimuovi FD quando chiudi
✅ **FD_ISSET**: Controlla se FD è pronto
✅ **EINTR**: Gestisci interruzione da segnali
✅ **Timeout**: Usa struct timeval se necessario
✅ **Aggiorna max_fd**: Dopo ogni add/remove

## TEMPLATE COMPLETO SELECT

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024

int main() {
    int server_fd, client_fds[MAX_CLIENTS];
    fd_set master_fds, read_fds;
    int max_fd;
    
    // Setup server
    server_fd = create_server_socket(8080);
    
    // Inizializza client array
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_fds[i] = -1;
    }
    
    // Inizializza fd_set
    FD_ZERO(&master_fds);
    FD_SET(server_fd, &master_fds);
    max_fd = server_fd;
    
    while (1) {
        // Copia set master
        read_fds = master_fds;
        
        // Select con timeout opzionale
        struct timeval timeout = {10, 0};  // 10 secondi
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (activity < 0 && errno != EINTR) {
            perror("select failed");
            break;
        }
        
        if (activity == 0) {
            printf("Timeout - nessuna attività\n");
            continue;
        }
        
        // Nuova connessione
        if (FD_ISSET(server_fd, &read_fds)) {
            handle_new_connection(server_fd, client_fds, &master_fds, &max_fd);
        }
        
        // Dati da client esistenti
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int fd = client_fds[i];
            if (fd != -1 && FD_ISSET(fd, &read_fds)) {
                if (handle_client_data(fd) <= 0) {
                    // Disconnessione
                    close(fd);
                    FD_CLR(fd, &master_fds);
                    client_fds[i] = -1;
                    // Ricalcola max_fd se necessario
                }
            }
        }
    }
    
    return 0;
}
```