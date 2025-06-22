# Server/Client TCP e UDP - Guida Completa per Esame

## 📋 Indice
1. [Concetti Fondamentali](#concetti-fondamentali)
2. [TCP Server/Client](#tcp-serverclient)
3. [UDP Server/Client](#udp-serverclient)
4. [Sincronizzazione Thread](#sincronizzazione-thread)
5. [Select() - I/O Multiplexing](#select---io-multiplexing)
6. [Esempi Completi](#esempi-completi)
7. [Compilazione e Test](#compilazione-e-test)
8. [Troubleshooting](#troubleshooting)

---

## 🎯 Concetti Fondamentali

### TCP vs UDP - Differenze Chiave

| Aspetto | TCP | UDP |
|---------|-----|-----|
| **Tipo** | Stream (flusso) | Datagram (pacchetti) |
| **Connessione** | Orientato alla connessione | Senza connessione |
| **Affidabilità** | Garantita (acknowledgment) | Non garantita |
| **Ordine** | Messaggi ordinati | Possibile disordine |
| **Overhead** | Alto (headers, controllo) | Basso (minimal headers) |
| **Velocità** | Più lento | Più veloce |
| **Uso tipico** | Web, email, file transfer | Gaming, video streaming, DNS |

### Socket API - Confronto

**TCP Flow:**
```
Server: socket() → bind() → listen() → accept() → read()/write() → close()
Client: socket() → connect() → write()/read() → close()
```

**UDP Flow:**
```
Server: socket() → bind() → recvfrom()/sendto() → close()
Client: socket() → sendto()/recvfrom() → close()
```

### Tipi di Socket

```c
// TCP
int sock = socket(AF_INET, SOCK_STREAM, 0);

// UDP  
int sock = socket(AF_INET, SOCK_DGRAM, 0);
```

---

## 🔗 TCP Server/Client

### Server TCP Base

```c
#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080

int main(void) {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    
    // 1. Crea socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) err_sys("socket failed");
    
    // 2. Configura indirizzo
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;  // Tutte le interfacce
    address.sin_port = htons(PORT);
    
    // 3. Bind - associa socket all'indirizzo
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
        err_sys("bind failed");
    
    // 4. Listen - inizia ad ascoltare (max 3 in coda)
    if (listen(server_fd, 3) < 0) err_sys("listen failed");
    
    printf("Server TCP in ascolto su porta %d\n", PORT);
    
    while (1) {
        // 5. Accept - accetta connessione
        client_fd = accept(server_fd, (struct sockaddr *)&address, 
                          (socklen_t*)&addrlen);
        if (client_fd < 0) err_sys("accept failed");
        
        // 6. Comunicazione
        read(client_fd, buffer, 1024);
        printf("Ricevuto: %s\n", buffer);
        send(client_fd, buffer, strlen(buffer), 0);  // Echo
        
        close(client_fd);  // Chiudi connessione client
    }
    
    close(server_fd);
    return 0;
}
```

### Server TCP con Select() - Multi-Client

```c
#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

#define PORT 8080
#define MAX_CLIENTS 10

int main(void) {
    int server_fd, client_sockets[MAX_CLIENTS];
    struct sockaddr_in address;
    fd_set readfds;
    int max_fd, activity, i, new_socket, addrlen;
    char buffer[1024];
    
    // Inizializza array client
    for (i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;
    }
    
    // Setup server (socket, bind, listen)
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);
    
    printf("Server TCP con select() su porta %d\n", PORT);
    addrlen = sizeof(address);
    
    while (1) {
        // Prepara set per select
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_fd = server_fd;
        
        // Aggiungi client socket al set
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] > 0) {
                FD_SET(client_sockets[i], &readfds);
                if (client_sockets[i] > max_fd)
                    max_fd = client_sockets[i];
            }
        }
        
        // Aspetta attività
        activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) err_sys("select error");
        
        // Nuova connessione
        if (FD_ISSET(server_fd, &readfds)) {
            new_socket = accept(server_fd, (struct sockaddr *)&address,
                              (socklen_t*)&addrlen);
            printf("Nuova connessione: socket %d\n", new_socket);
            
            // Trova slot libero
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
        }
        
        // Controlla client esistenti
        for (i = 0; i < MAX_CLIENTS; i++) {
            int sd = client_sockets[i];
            if (FD_ISSET(sd, &readfds)) {
                int valread = read(sd, buffer, 1024);
                if (valread == 0) {
                    // Client disconnesso
                    close(sd);
                    client_sockets[i] = 0;
                } else {
                    // Echo del messaggio
                    send(sd, buffer, valread, 0);
                }
            }
        }
    }
    
    return 0;
}
```

### Client TCP Multi-Thread con Condition Variable

```c
#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"

// Struttura sincronizzazione
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    int current_thread;  // 0 o 1
    int server_socket;
} sync_data_t;

sync_data_t sync_data;

void* thread_function(void* arg) {
    int thread_id = *((int*)arg);
    char received_char;
    char dummy = 'X';
    
    for (int i = 0; i < 5; i++) {
        // === SEZIONE CRITICA ===
        pthread_mutex_lock(&sync_data.mutex);
        
        // Aspetta il proprio turno
        while (sync_data.current_thread != thread_id) {
            pthread_cond_wait(&sync_data.condition, &sync_data.mutex);
        }
        
        printf("Thread %d: comunico con server\n", thread_id);
        
        // Comunicazione
        send(sync_data.server_socket, &dummy, 1, 0);
        recv(sync_data.server_socket, &received_char, 1, 0);
        printf("Thread %d ricevuto: '%c'\n", thread_id, received_char);
        
        // Passa il turno
        sync_data.current_thread = (thread_id == 0) ? 1 : 0;
        pthread_cond_signal(&sync_data.condition);
        
        pthread_mutex_unlock(&sync_data.mutex);
        // === FINE SEZIONE CRITICA ===
        
        sleep(1);  // Lavoro non critico
    }
    
    return NULL;
}

int main(void) {
    int sock;
    struct sockaddr_in serv_addr;
    pthread_t thread1, thread2;
    int thread_id1 = 0, thread_id2 = 1;
    
    // Inizializza sincronizzazione
    pthread_mutex_init(&sync_data.mutex, NULL);
    pthread_cond_init(&sync_data.condition, NULL);
    sync_data.current_thread = 0;
    
    // Connessione al server
    sock = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr);
    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    
    sync_data.server_socket = sock;
    
    // Crea thread
    pthread_create(&thread1, NULL, thread_function, &thread_id1);
    pthread_create(&thread2, NULL, thread_function, &thread_id2);
    
    // Aspetta thread
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    // Pulizia
    close(sock);
    pthread_mutex_destroy(&sync_data.mutex);
    pthread_cond_destroy(&sync_data.condition);
    
    return 0;
}
```

---

## 📡 UDP Server/Client

### Server UDP Multi-Porta

```c
#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <stdlib.h>
#include <time.h>

#define PORT_A 8080
#define PORT_B 8081

int main(void) {
    int socket_A, socket_B;
    struct sockaddr_in addr_A, addr_B, client_addr;
    fd_set readfds;
    int max_fd;
    socklen_t client_len = sizeof(client_addr);
    char buffer[1024];
    
    srand(time(NULL));
    
    // Crea socket UDP
    socket_A = socket(AF_INET, SOCK_DGRAM, 0);
    socket_B = socket(AF_INET, SOCK_DGRAM, 0);
    
    // Configura indirizzi
    addr_A.sin_family = AF_INET;
    addr_A.sin_addr.s_addr = INADDR_ANY;
    addr_A.sin_port = htons(PORT_A);
    
    addr_B.sin_family = AF_INET;
    addr_B.sin_addr.s_addr = INADDR_ANY;
    addr_B.sin_port = htons(PORT_B);
    
    // Bind
    bind(socket_A, (struct sockaddr *)&addr_A, sizeof(addr_A));
    bind(socket_B, (struct sockaddr *)&addr_B, sizeof(addr_B));
    
    printf("Server UDP su porte %d e %d\n", PORT_A, PORT_B);
    max_fd = (socket_A > socket_B) ? socket_A : socket_B;
    
    while (1) {
        // Prepara set
        FD_ZERO(&readfds);
        FD_SET(socket_A, &readfds);
        FD_SET(socket_B, &readfds);
        
        // Select
        select(max_fd + 1, &readfds, NULL, NULL, NULL);
        
        // Porta A
        if (FD_ISSET(socket_A, &readfds)) {
            recvfrom(socket_A, buffer, sizeof(buffer), 0,
                    (struct sockaddr *)&client_addr, &client_len);
            char random_char = 'A' + (rand() % 26);
            sendto(socket_A, &random_char, 1, 0,
                  (struct sockaddr *)&client_addr, client_len);
            printf("Porta A: inviato '%c'\n", random_char);
        }
        
        // Porta B
        if (FD_ISSET(socket_B, &readfds)) {
            recvfrom(socket_B, buffer, sizeof(buffer), 0,
                    (struct sockaddr *)&client_addr, &client_len);
            char random_char = 'A' + (rand() % 26);
            sendto(socket_B, &random_char, 1, 0,
                  (struct sockaddr *)&client_addr, client_len);
            printf("Porta B: inviato '%c'\n", random_char);
        }
    }
    
    return 0;
}
```

### Client UDP Multi-Thread con Semafori POSIX

```c
#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>

#define SERVER_IP "127.0.0.1"
#define PORT_A 8080
#define PORT_B 8081

typedef struct {
    int thread_id;
    int port;
} thread_data_t;

sem_t *sync_semaphore;

void* client_thread(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    int client_fd;
    struct sockaddr_in server_addr;
    char dummy = 'X';
    char received_char;
    socklen_t server_len = sizeof(server_addr);
    
    // Crea socket UDP
    client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    
    // Configura indirizzo server
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(data->port);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
    
    for (int i = 0; i < 3; i++) {
        // === SEZIONE CRITICA CON SEMAFORO ===
        printf("Thread %d: richiedo accesso...\n", data->thread_id);
        sem_wait(sync_semaphore);  // Prendi semaforo
        
        printf("Thread %d: comunico con porta %d\n", 
               data->thread_id, data->port);
        
        // Comunicazione UDP
        sendto(client_fd, &dummy, 1, 0,
               (struct sockaddr *)&server_addr, sizeof(server_addr));
        
        int bytes = recvfrom(client_fd, &received_char, 1, 0,
                           (struct sockaddr *)&server_addr, &server_len);
        
        if (bytes > 0) {
            printf("Thread %d ricevuto: '%c'\n", data->thread_id, received_char);
        }
        
        sleep(1);  // Simula elaborazione
        
        printf("Thread %d: rilascio accesso\n", data->thread_id);
        sem_post(sync_semaphore);  // Rilascia semaforo
        // === FINE SEZIONE CRITICA ===
        
        sleep(1);  // Pausa tra comunicazioni
    }
    
    close(client_fd);
    return NULL;
}

int main(void) {
    pthread_t thread1, thread2;
    thread_data_t data1 = {1, PORT_A};
    thread_data_t data2 = {2, PORT_B};
    
    // Crea semaforo named (valore 1 = mutex)
    sync_semaphore = sem_open("/udp_sync", O_CREAT, 0644, 1);
    if (sync_semaphore == SEM_FAILED) {
        err_sys("sem_open failed");
    }
    
    printf("Client UDP sincronizzato\n");
    
    // Crea thread
    pthread_create(&thread1, NULL, client_thread, &data1);
    pthread_create(&thread2, NULL, client_thread, &data2);
    
    // Aspetta thread
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    // Pulizia semaforo
    sem_close(sync_semaphore);
    sem_unlink("/udp_sync");
    
    printf("Client terminato\n");
    return 0;
}
```

---

## 🔄 Sincronizzazione Thread

### Mutex (Mutual Exclusion)

```c
#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void critical_section() {
    pthread_mutex_lock(&mutex);    // Entra sezione critica
    // ... codice che solo un thread può eseguire ...
    pthread_mutex_unlock(&mutex);  // Esce sezione critica
}
```

### Condition Variable

```c
#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
int ready = 0;

void* waiter_thread(void* arg) {
    pthread_mutex_lock(&mutex);
    while (!ready) {
        pthread_cond_wait(&condition, &mutex);  // Aspetta condizione
    }
    // ... condizione soddisfatta ...
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void* signaler_thread(void* arg) {
    // ... lavoro ...
    pthread_mutex_lock(&mutex);
    ready = 1;
    pthread_cond_signal(&condition);  // Notifica waiter
    pthread_mutex_unlock(&mutex);
    return NULL;
}
```

### Semafori POSIX Named

```c
#include <semaphore.h>
#include <fcntl.h>

sem_t *semaforo;

// Inizializzazione
semaforo = sem_open("/my_semaphore", O_CREAT, 0644, 1);  // Valore iniziale 1

// Uso
sem_wait(semaforo);    // P operation (decrementa, blocca se 0)
// ... sezione critica ...
sem_post(semaforo);    // V operation (incrementa, sblocca waiter)

// Pulizia
sem_close(semaforo);
sem_unlink("/my_semaphore");
```

### Confronto Sincronizzazione

| Tipo | Uso | Vantaggi | Svantaggi |
|------|-----|----------|-----------|
| **Mutex** | Accesso esclusivo | Semplice, veloce | Solo tra thread stesso processo |
| **Condition Variable** | Attesa condizione | Efficiente, flessibile | Più complesso |
| **Semaforo Named** | Tra processi diversi | Inter-process | Overhead maggiore |

---

## ⚡ Select() - I/O Multiplexing

### Concetto Base

```c
#include <sys/select.h>

fd_set readfds, writefds, exceptfds;
struct timeval timeout;

// Prepara set
FD_ZERO(&readfds);
FD_SET(socket1, &readfds);
FD_SET(socket2, &readfds);

// Timeout
timeout.tv_sec = 5;    // 5 secondi
timeout.tv_usec = 0;   // 0 microsecondi

// Aspetta attività
int activity = select(max_fd + 1, &readfds, &writefds, &exceptfds, &timeout);

if (activity > 0) {
    if (FD_ISSET(socket1, &readfds)) {
        // socket1 ha dati da leggere
    }
    if (FD_ISSET(socket2, &readfds)) {
        // socket2 ha dati da leggere  
    }
}
```

### Operazioni fd_set

```c
fd_set set;

FD_ZERO(&set);        // Svuota set
FD_SET(fd, &set);     // Aggiungi fd al set
FD_CLR(fd, &set);     // Rimuovi fd dal set  
FD_ISSET(fd, &set);   // Controlla se fd è nel set (ritorna 1/0)
```

### Template Select Completo

```c
void server_select_loop() {
    fd_set readfds;
    int max_fd = 0;
    struct timeval timeout;
    
    while (1) {
        // Prepara set ad ogni iterazione
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);
        max_fd = server_socket;
        
        // Aggiungi client socket
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] > 0) {
                FD_SET(client_sockets[i], &readfds);
                if (client_sockets[i] > max_fd) {
                    max_fd = client_sockets[i];
                }
            }
        }
        
        // Timeout (opzionale)
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        // Select
        int activity = select(max_fd + 1, &readfds, NULL, NULL, &timeout);
        
        if (activity < 0) {
            perror("select error");
            break;
        }
        
        if (activity == 0) {
            // Timeout - nessuna attività
            continue;
        }
        
        // Controlla server socket per nuove connessioni
        if (FD_ISSET(server_socket, &readfds)) {
            handle_new_connection();
        }
        
        // Controlla client socket per dati
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_sockets[i] > 0 && FD_ISSET(client_sockets[i], &readfds)) {
                handle_client_data(client_sockets[i]);
            }
        }
    }
}
```

---

## 💻 Esempi Completi

### Makefile per Tutti gli Esercizi

```makefile
CC = gcc
CFLAGS = -g -Wall -I/usr/local/include -D_GNU_SOURCE
LDFLAGS = -L/usr/local/lib
LDLIBS = -lapue -lpthread

# Programmi TCP
TCP_PROGS = tcp_server tcp_client_sync

# Programmi UDP  
UDP_PROGS = udp_server_multi udp_client_sync

# Tutti i programmi
ALL_PROGS = $(TCP_PROGS) $(UDP_PROGS)

all: $(ALL_PROGS)

# TCP
tcp_server: tcp_server.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(LDLIBS)

tcp_client_sync: tcp_client_sync.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(LDLIBS)

# UDP
udp_server_multi: udp_server_multi.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(LDLIBS)

udp_client_sync: udp_client_sync.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(LDLIBS)

# Test automatici
test-tcp: tcp_server tcp_client_sync
	@echo "Test TCP..."
	./tcp_server &
	sleep 2
	./tcp_client_sync
	pkill -f tcp_server || true

test-udp: udp_server_multi udp_client_sync
	@echo "Test UDP..."
	./udp_server_multi &
	sleep 2  
	./udp_client_sync
	pkill -f udp_server_multi || true

clean:
	rm -f $(ALL_PROGS)
	
.PHONY: all test-tcp test-udp clean
```

### Script di Test Completo

```bash
#!/bin/bash
# test_all.sh

echo "=== TEST COMPLETO SERVER/CLIENT ==="

# Funzione per killare processi
cleanup() {
    pkill -f tcp_server || true
    pkill -f udp_server || true
    echo "Processi terminati"
}

# Trap per cleanup automatico
trap cleanup EXIT

echo "1. Test TCP Server/Client..."
make tcp_server tcp_client_sync

echo "Avvio server TCP..."
./tcp_server &
TCP_PID=$!
sleep 2

echo "Avvio client TCP..."
./tcp_client_sync

echo "Termino server TCP..."
kill $TCP_PID 2>/dev/null

echo ""
echo "2. Test UDP Server/Client..."
make udp_server_multi udp_client_sync

echo "Avvio server UDP..."
./udp_server_multi &
UDP_PID=$!
sleep 2

echo "Avvio client UDP..."
./udp_client_sync

echo "Termino server UDP..."
kill $UDP_PID 2>/dev/null

echo ""
echo "=== TUTTI I TEST COMPLETATI ==="
```

---

## 🔧 Compilazione e Test

### Compilazione Singola

```bash
# TCP
gcc -g -Wall -I/usr/local/include -D_GNU_SOURCE tcp_server.c -o tcp_server -L/usr/local/lib -lapue -lpthread

# UDP
gcc -g -Wall -I/usr/local/include -D_GNU_SOURCE udp_client.c -o udp_client -L/usr/local/lib -lapue -lpthread
```

### Comando runc Personalizzato

```bash
# Nel tuo script runc
runc() {
    gcc -g -Wall -I/usr/local/include -D_GNU_SOURCE "$1" -o "${1%.c}" -L/usr/local/lib -lapue -lpthread
    if [ $? -eq 0 ]; then
        echo "Compilazione riuscita!"
        ./"${1%.c}"
    fi
}
```

### Test con Telnet/Netcat

```bash
# Test TCP server
telnet localhost 8080

# Test UDP server  
echo "test message" | nc -u localhost 8080

# Client UDP interattivo
nc -u localhost 8080
```

---

## 🚨 Troubleshooting

### Errori Comuni

#### 1. "Address already in use"

```bash
# Problema: Porta già occupata
# Soluzione:
netstat -tulpn | grep :8080    # Trova processo
kill -9 PID                    # Termina processo

# Oppure nel codice:
int opt = 1;
setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
```

#### 2. "Connection refused"

```bash
# Problema: Server non in ascolto
# Soluzione: Verifica che server sia avviato
ps aux | grep server
```

#### 3. "Segmentation fault" nei thread

```c
// Problema comune: Passaggio parametri sbagliato
// SBAGLIATO:
pthread_create(&thread, NULL, func, thread_id);  // Passa valore

// CORRETTO:
pthread_create(&thread, NULL, func, &thread_id); // Passa indirizzo
```

#### 4. Thread non sincronizzati

```c
// Verifica inizializzazione
pthread_mutex_init(&mutex, NULL);
pthread_cond_init(&condition, NULL);

// Verifica ordine lock/unlock
pthread_mutex_lock(&mutex);
// ... sezione critica ...
pthread_mutex_unlock(&mutex);  // Non dimenticare!
```

#### 5. "sem_open failed"

```bash
# Problema: Semaforo già esistente
# Soluzione: Rimuovi semaforo esistente
ls /dev/shm/sem.*              # Lista semafori
rm /dev/shm/sem.nome_semaforo  # Rimuovi se necessario

# Nel codice:
sem_unlink("/nome_semaforo");  // Prima di creare nuovo
```

### Debug Tools

```bash
# Network connections
netstat -tulpn | grep :8080

# Processes
ps aux | grep server

# Memory leaks
valgrind --leak-check=full ./program

# Thread debugging
gdb ./program
(gdb) info threads
(gdb) thread apply all bt
```

### Log e Debug

```c
// Macro di debug
#ifdef DEBUG
#define DBG_PRINT(fmt, ...) \
    printf("[DEBUG] %s:%d " fmt "\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define DBG_PRINT(fmt, ...)
#endif

// Uso:
DBG_PRINT("Thread %d iniziato", thread_id);
```

---

## 📝 Note per l'Esame

### Checklist Pre-Esame

- [ ] **TCP**: socket, bind, listen, accept, read/write
- [ ] **UDP**: socket, bind, sendto/recvfrom  
- [ ] **Select()**: FD_ZERO, FD_SET, FD_ISSET, parametri corretti
- [ ] **Thread**: pthread_create, pthread_join, parametri corretti
- [ ] **Sincronizzazione**: mutex, condition variable, semafori
- [ ] **Gestione errori**: Controllo valori di ritorno
- [ ] **Pulizia risorse**: close(), pthread_mutex_destroy(), sem_unlink()

### Domande Tipiche

1. **Differenza TCP vs UDP?**
   - TCP: connessione, affidabile, ordinato
   - UDP: senza connessione, veloce, possibili perdite

2. **Cosa fa select()?**
   - Aspetta attività su più file descriptor
   - Non blocca se nessun fd è pronto

3. **Differenza mutex vs semaforo?**
   - Mutex: binario (0/1), stesso processo
   - Semaforo: contatore, inter-processo

4. **Perché pthread_cond_wait() rilascia mutex?**
   - Evita deadlock, permette ad altri thread di modificare condizione

### Template di Risposta

```c
// 1. Include necessari
#include "apue.h"
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>

// 2. Definizioni e strutture
#define PORT 8080
typedef struct { ... } data_t;

// 3. Variabili globali (se necessarie)
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// 4. Funzioni thread
void* thread_func(void* arg) {
    // Cast parametro
    // Sincronizzazione
    // Logica principale
    // Pulizia
    return NULL;
}

// 5. Main
int main(void) {
    // Inizializzazione
    // Creazione socket/thread
    // Join thread
    // Pulizia risorse
    return 0;
}
```

---

## 🎯 Riassunto Finale

**TCP**: Connessione affidabile, select() per multi-client, condition variable per sincronizzazione thread.

**UDP**: Messaggi veloci, multi-porta con select(), semafori POSIX per sincronizzazione.

**Thread**: pthread_create/join, sincronizzazione con mutex/condition/semafori.

**Select()**: I/O multiplexing, fd_set per gestire più socket.

**Errori comuni**: Parametri thread, gestione puntatori, pulizia risorse.

---

*Questa guida copre tutti i concetti necessari per l'esame. Pratica con gli esempi e testa il codice!* 🚀