# Server TCP - Riferimento Completo

## 🎯 Panoramica Approcci Server TCP

### **Tipi di Server TCP**

| **Tipo** | **Concorrenza** | **Complessità** | **Scalabilità** | **Uso Memoria** |
|----------|----------------|-----------------|-----------------|-----------------|
| **Sequenziale** | ❌ No | ✅ Bassa | ❌ Limitata | ✅ Minimo |
| **Fork** | ✅ Processi | ⚠️ Media | ⚠️ Media | ❌ Alto |
| **Thread** | ✅ Thread | ⚠️ Media | ✅ Buona | ⚠️ Medio |
| **Select** | ✅ I/O Multiplexing | ❌ Alta | ✅ Ottima | ✅ Minimo |

---

## 🔄 SERVER SEQUENZIALE

### **Concetti Fondamentali**
- **Mono-client**: gestisce UN solo client alla volta
- **Blocking**: si blocca su ogni operazione I/O
- **Semplice**: codice lineare e intuitivo
- **Limitato**: altri client devono attendere

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    int opt = 1;
    
    // 1. CREAZIONE SOCKET
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Riuso indirizzo
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // 2. CONFIGURAZIONE INDIRIZZO
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // 3. BIND E LISTEN
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, 5) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("🔄 Server Sequenziale avviato su porta %d\n", PORT);
    printf("⚠️  Gestisce UN solo client alla volta\n\n");
    
    // 4. MAIN LOOP - UN CLIENT ALLA VOLTA
    while (1) {
        printf("⏳ Aspettando nuova connessione...\n");
        
        // BLOCCA fino a nuova connessione
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }
        
        printf("✅ Client connesso: %s:%d [fd=%d]\n", 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port), client_fd);
        
        // Messaggio benvenuto
        send(client_fd, "🔄 Server Sequenziale - Sei l'unico client!\n", 44, 0);
        
        // 5. GESTIONE CLIENT (fino a disconnessione)
        while (1) {
            int bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
            
            if (bytes_read <= 0) {
                if (bytes_read == 0) {
                    printf("👋 Client [%d] disconnesso\n", client_fd);
                } else {
                    perror("recv error");
                }
                break;  // Esci dal loop client
            }
            
            buffer[bytes_read] = '\0';
            printf("📩 Ricevuto: '%s'\n", buffer);
            
            // Echo al client
            snprintf(buffer, BUFFER_SIZE, "✅ Echo: %s", buffer);
            send(client_fd, buffer, strlen(buffer), 0);
        }
        
        // 6. CHIUDI CONNESSIONE CLIENT
        close(client_fd);
        printf("🔒 Connessione chiusa, pronto per nuovo client\n\n");
    }
    
    close(server_fd);
    return 0;
}
```

---

## 🍴 SERVER CON FORK

### **Concetti Fondamentali**
- **Multi-processo**: crea processo figlio per ogni client
- **Isolamento**: ogni client ha processo dedicato
- **Robustezza**: crash di un client non influenza altri
- **Overhead**: alto consumo memoria per processi

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Gestore segnale per processi zombie
void sigchld_handler(int sig) {
    // Raccoglie processi figli terminati
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

// Funzione per gestire singolo client (processo figlio)
void gestisci_client(int client_fd, struct sockaddr_in client_addr) {
    char buffer[BUFFER_SIZE];
    pid_t pid = getpid();
    
    printf("🍴 Processo figlio [PID=%d] gestisce client %s:%d\n", 
           pid, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    
    // Messaggio benvenuto
    snprintf(buffer, BUFFER_SIZE, 
            "🍴 Benvenuto! Sei gestito dal processo %d\n", pid);
    send(client_fd, buffer, strlen(buffer), 0);
    
    // Loop gestione messaggi
    while (1) {
        int bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_read <= 0) {
            if (bytes_read == 0) {
                printf("👋 [PID=%d] Client disconnesso\n", pid);
            } else {
                printf("❌ [PID=%d] Errore recv: %s\n", pid, strerror(errno));
            }
            break;
        }
        
        buffer[bytes_read] = '\0';
        printf("📩 [PID=%d] Ricevuto: '%s'\n", pid, buffer);
        
        // Elaborazione (simula lavoro)
        sleep(1);
        
        // Risposta al client
        snprintf(buffer, BUFFER_SIZE, 
                "✅ [Processo %d] Elaborato: %s", pid, buffer);
        send(client_fd, buffer, strlen(buffer), 0);
    }
    
    close(client_fd);
    printf("🔒 [PID=%d] Processo figlio terminato\n", pid);
    exit(0);  // Termina processo figlio
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    int opt = 1;
    pid_t pid;
    
    // Installa gestore segnale per processi zombie
    signal(SIGCHLD, sigchld_handler);
    
    // 1. CREAZIONE SOCKET
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // 2. BIND E LISTEN
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_fd, 10);
    
    printf("🍴 Server Fork avviato su porta %d\n", PORT);
    printf("👨‍👧‍👦 Ogni client avrà il suo processo dedicato\n\n");
    
    // 3. MAIN LOOP - ACCETTA E FORKA
    while (1) {
        printf("⏳ Aspettando connessioni...\n");
        
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }
        
        printf("✅ Nuova connessione da %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        // 4. FORK - CREA PROCESSO FIGLIO
        pid = fork();
        
        if (pid == 0) {
            // PROCESSO FIGLIO
            close(server_fd);  // Figlio non ha bisogno del server socket
            gestisci_client(client_fd, client_addr);
            // gestisci_client termina con exit(0)
        }
        else if (pid > 0) {
            // PROCESSO PADRE
            close(client_fd);  // Padre non ha bisogno del client socket
            printf("🔄 Processo padre continua ad accettare connessioni\n");
        }
        else {
            // ERRORE FORK
            perror("fork failed");
            close(client_fd);
        }
    }
    
    close(server_fd);
    return 0;
}
```

---

## 🧵 SERVER CON THREAD

### **Concetti Fondamentali**
- **Multi-thread**: crea thread per ogni client
- **Condivisione**: thread condividono memoria del processo
- **Efficienza**: minor overhead rispetto ai processi
- **Sincronizzazione**: serve attenzione ai dati condivisi

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

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 50

// Struttura per passare dati al thread
typedef struct {
    int client_fd;
    struct sockaddr_in client_addr;
    int thread_id;
} ThreadData;

// Contatore thread attivi (condiviso)
int active_threads = 0;
pthread_mutex_t thread_count_mutex = PTHREAD_MUTEX_INITIALIZER;

// Funzione thread per gestire client
void* gestisci_client_thread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int client_fd = data->client_fd;
    int thread_id = data->thread_id;
    char buffer[BUFFER_SIZE];
    
    // Incrementa contatore thread
    pthread_mutex_lock(&thread_count_mutex);
    active_threads++;
    printf("🧵 Thread %d avviato per client %s:%d (Attivi: %d)\n", 
           thread_id, inet_ntoa(data->client_addr.sin_addr), 
           ntohs(data->client_addr.sin_port), active_threads);
    pthread_mutex_unlock(&thread_count_mutex);
    
    // Messaggio benvenuto
    snprintf(buffer, BUFFER_SIZE, 
            "🧵 Benvenuto! Sei gestito dal thread %d\n", thread_id);
    send(client_fd, buffer, strlen(buffer), 0);
    
    // Loop gestione messaggi
    while (1) {
        int bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_read <= 0) {
            if (bytes_read == 0) {
                printf("👋 [Thread %d] Client disconnesso\n", thread_id);
            } else {
                printf("❌ [Thread %d] Errore recv: %s\n", thread_id, strerror(errno));
            }
            break;
        }
        
        buffer[bytes_read] = '\0';
        printf("📩 [Thread %d] Ricevuto: '%s'\n", thread_id, buffer);
        
        // Elaborazione (simula lavoro)
        usleep(500000);  // 0.5 secondi
        
        // Risposta
        snprintf(buffer, BUFFER_SIZE, 
                "✅ [Thread %d] Elaborato: %s", thread_id, buffer);
        send(client_fd, buffer, strlen(buffer), 0);
    }
    
    // Cleanup
    close(client_fd);
    
    // Decrementa contatore
    pthread_mutex_lock(&thread_count_mutex);
    active_threads--;
    printf("🔒 [Thread %d] Terminato (Attivi: %d)\n", thread_id, active_threads);
    pthread_mutex_unlock(&thread_count_mutex);
    
    free(data);  // Libera memoria passata al thread
    pthread_exit(NULL);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    int opt = 1;
    pthread_t thread;
    int thread_counter = 0;
    
    // 1. CREAZIONE SOCKET
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // 2. BIND E LISTEN
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_fd, MAX_CLIENTS);
    
    printf("🧵 Server Thread avviato su porta %d\n", PORT);
    printf("🔢 Gestisce fino a %d client contemporaneamente\n\n", MAX_CLIENTS);
    
    // 3. MAIN LOOP - ACCETTA E CREA THREAD
    while (1) {
        printf("⏳ Aspettando connessioni...\n");
        
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }
        
        printf("✅ Nuova connessione da %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        // Controllo limite thread
        pthread_mutex_lock(&thread_count_mutex);
        if (active_threads >= MAX_CLIENTS) {
            pthread_mutex_unlock(&thread_count_mutex);
            printf("❌ Troppi client attivi, connessione rifiutata\n");
            send(client_fd, "Server pieno, riprova più tardi\n", 32, 0);
            close(client_fd);
            continue;
        }
        pthread_mutex_unlock(&thread_count_mutex);
        
        // 4. PREPARA DATI PER THREAD
        ThreadData* data = malloc(sizeof(ThreadData));
        data->client_fd = client_fd;
        data->client_addr = client_addr;
        data->thread_id = ++thread_counter;
        
        // 5. CREA THREAD
        if (pthread_create(&thread, NULL, gestisci_client_thread, data) != 0) {
            perror("pthread_create failed");
            free(data);
            close(client_fd);
        } else {
            // Thread detached (si pulisce automaticamente)
            pthread_detach(thread);
        }
    }
    
    close(server_fd);
    pthread_mutex_destroy(&thread_count_mutex);
    return 0;
}
```

---

## 🎯 Server TCP con Select (I/O Multiplexing)

### **Cos'è Select?**
- **I/O Multiplexing**: gestisce multiple connessioni con un solo thread
- **Non-blocking**: evita che il server si blocchi su una singola connessione
- **Scalabile**: può gestire centinaia di client contemporaneamente
- **Event-driven**: reagisce solo quando ci sono dati pronti

### **Come Funziona Select?**
```
1. Server crea socket in ascolto
2. Aggiunge socket a un SET di file descriptor
3. Select() monitora TUTTI i socket nel set
4. Quando un socket ha dati pronti, select() ritorna
5. Server elabora solo i socket pronti
6. Ripete il ciclo
```

---

## 🔧 Struttura Base Server TCP con Select

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
#include <time.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

// Struttura per gestire client
typedef struct {
    int socket_fd;          // File descriptor del socket client
    struct sockaddr_in addr; // Indirizzo del client
    time_t connect_time;    // Timestamp connessione
    char buffer[BUFFER_SIZE]; // Buffer per dati parziali
    int buffer_len;         // Lunghezza dati nel buffer
} ClientInfo;

int main() {
    int server_fd;                    // Socket server principale
    int max_fd;                       // Massimo file descriptor per select
    fd_set read_fds, master_fds;      // Set di file descriptor
    ClientInfo clients[MAX_CLIENTS];  // Array dei client connessi
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    int opt = 1;
    
    // Inizializza array clienti
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket_fd = -1;  // -1 = slot libero
    }
    
    // 1. CREAZIONE SOCKET SERVER
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Opzioni socket (riuso indirizzo)
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    // 2. CONFIGURAZIONE INDIRIZZO
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Accetta da qualsiasi IP
    server_addr.sin_port = htons(PORT);
    
    // 3. BIND E LISTEN
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    
    printf("🚀 Server TCP avviato sulla porta %d\n", PORT);
    printf("📊 Gestisce fino a %d client contemporaneamente\n", MAX_CLIENTS);
    printf("🔄 Utilizzando I/O multiplexing con select()\n\n");
    
    // 4. INIZIALIZZA SELECT
    FD_ZERO(&master_fds);           // Pulisce il set
    FD_SET(server_fd, &master_fds); // Aggiunge server socket al set
    max_fd = server_fd;             // Traccia il massimo FD
    
    // 5. MAIN LOOP - GESTIONE EVENTI
    while (1) {
        // Copia master set (select modifica il set)
        read_fds = master_fds;
        
        printf("⏳ Waiting for activity on %d sockets...\n", max_fd + 1);
        
        // BLOCCA fino a quando un socket è pronto
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        
        if (activity < 0) {
            perror("select error");
            break;
        }
        
        printf("🎯 Activity detected on %d socket(s)\n", activity);
        
        // 6. CONTROLLA SERVER SOCKET (nuove connessioni)
        if (FD_ISSET(server_fd, &read_fds)) {
            int new_client = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            
            if (new_client < 0) {
                perror("accept failed");
                continue;
            }
            
            // Trova slot libero per nuovo client
            int slot = -1;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].socket_fd == -1) {
                    slot = i;
                    break;
                }
            }
            
            if (slot == -1) {
                // Nessun slot libero
                printf("❌ Server pieno! Rifiutando client %s\n", 
                       inet_ntoa(client_addr.sin_addr));
                send(new_client, "Server full\n", 12, 0);
                close(new_client);
            } else {
                // Aggiungi nuovo client
                clients[slot].socket_fd = new_client;
                clients[slot].addr = client_addr;
                clients[slot].connect_time = time(NULL);
                clients[slot].buffer_len = 0;
                
                FD_SET(new_client, &master_fds);  // Aggiungi al set select
                
                if (new_client > max_fd) {
                    max_fd = new_client;  // Aggiorna max FD
                }
                
                printf("✅ Nuovo client [%d]: %s:%d (slot %d)\n", 
                       new_client, inet_ntoa(client_addr.sin_addr), 
                       ntohs(client_addr.sin_port), slot);
                
                // Messaggio di benvenuto
                char welcome[256];
                snprintf(welcome, sizeof(welcome), 
                        "🎉 Benvenuto! Sei il client #%d\n", slot + 1);
                send(new_client, welcome, strlen(welcome), 0);
            }
        }
        
        // 7. CONTROLLA CLIENT SOCKETS (dati in arrivo)
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int client_fd = clients[i].socket_fd;
            
            if (client_fd != -1 && FD_ISSET(client_fd, &read_fds)) {
                // Dati pronti da questo client
                int bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
                
                if (bytes_read <= 0) {
                    // Client disconnesso
                    if (bytes_read == 0) {
                        printf("👋 Client [%d] disconnesso normalmente\n", client_fd);
                    } else {
                        printf("❌ Errore client [%d]: %s\n", client_fd, strerror(errno));
                    }
                    
                    // Rimuovi client
                    close(client_fd);
                    FD_CLR(client_fd, &master_fds);
                    clients[i].socket_fd = -1;
                    
                    // Aggiorna max_fd se necessario
                    if (client_fd == max_fd) {
                        max_fd = server_fd;
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (clients[j].socket_fd > max_fd) {
                                max_fd = clients[j].socket_fd;
                            }
                        }
                    }
                } else {
                    // Elabora dati ricevuti
                    buffer[bytes_read] = '\0';
                    printf("📩 Client [%d]: '%s'\n", client_fd, buffer);
                    
                    // Echo dei dati a tutti gli altri client
                    char response[BUFFER_SIZE + 100];
                    snprintf(response, sizeof(response), 
                            "Client #%d dice: %s", i + 1, buffer);
                    
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (clients[j].socket_fd != -1 && j != i) {
                            send(clients[j].socket_fd, response, strlen(response), 0);
                        }
                    }
                    
                    // Risposta al mittente
                    snprintf(response, sizeof(response), 
                            "✅ Messaggio inoltrato a %d client\n", 
                            get_active_clients_count(clients) - 1);
                    send(client_fd, response, strlen(response), 0);
                }
            }
        }
        
        printf("📊 Client attivi: %d/%d\n\n", get_active_clients_count(clients), MAX_CLIENTS);
    }
    
    // Cleanup
    close(server_fd);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket_fd != -1) {
            close(clients[i].socket_fd);
        }
    }
    
    return 0;
}

// Funzione utility: conta client attivi
int get_active_clients_count(ClientInfo clients[]) {
    int count = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket_fd != -1) {
            count++;
        }
    }
    return count;
}
```

---

## 🎯 Server TCP per Traccia Esame (Carattere Casuale)

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <time.h>

#define PORT 8080
#define MAX_CLIENTS 10

// Genera carattere casuale alfanumerico
char genera_carattere_casuale() {
    int tipo = rand() % 3;
    switch (tipo) {
        case 0: return 'A' + (rand() % 26);           // Maiuscola
        case 1: return 'a' + (rand() % 26);           // Minuscola  
        case 2: return '0' + (rand() % 10);           // Numero
    }
    return 'X';
}

int main() {
    int server_fd, client_sockets[MAX_CLIENTS];
    fd_set read_fds, master_fds;
    int max_fd, activity, new_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[1024];
    int opt = 1;
    
    srand(time(NULL));  // Inizializza random
    
    // Inizializza array client
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;
    }
    
    // Crea socket server
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Configura indirizzo
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // Bind e listen
    bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_fd, MAX_CLIENTS);
    
    printf("🎲 Server caratteri casuali avviato su porta %d\n", PORT);
    
    // Inizializza select
    FD_ZERO(&master_fds);
    FD_SET(server_fd, &master_fds);
    max_fd = server_fd;
    
    while (1) {
        read_fds = master_fds;
        
        activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        
        // Nuova connessione
        if (FD_ISSET(server_fd, &read_fds)) {
            new_socket = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            
            // Trova slot libero
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    FD_SET(new_socket, &master_fds);
                    
                    if (new_socket > max_fd) {
                        max_fd = new_socket;
                    }
                    
                    printf("✅ Nuovo client [%d] connesso\n", new_socket);
                    
                    // INVIA CARATTERE CASUALE IMMEDIATAMENTE
                    char carattere = genera_carattere_casuale();
                    snprintf(buffer, sizeof(buffer), "🎲 Carattere casuale: %c\n", carattere);
                    send(new_socket, buffer, strlen(buffer), 0);
                    
                    printf("📤 Inviato '%c' al client [%d]\n", carattere, new_socket);
                    break;
                }
            }
        }
        
        // Gestisci client esistenti
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int client_fd = client_sockets[i];
            
            if (client_fd > 0 && FD_ISSET(client_fd, &read_fds)) {
                int bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
                
                if (bytes_read <= 0) {
                    // Client disconnesso
                    printf("👋 Client [%d] disconnesso\n", client_fd);
                    close(client_fd);
                    FD_CLR(client_fd, &master_fds);
                    client_sockets[i] = 0;
                } else {
                    // Client ha inviato richiesta - invia nuovo carattere
                    char carattere = genera_carattere_casuale();
                    snprintf(buffer, sizeof(buffer), "🎲 Nuovo carattere: %c\n", carattere);
                    send(client_fd, buffer, strlen(buffer), 0);
                    
                    printf("📤 Inviato '%c' al client [%d]\n", carattere, client_fd);
                }
            }
        }
    }
    
    return 0;
}
```

---

## 🎯 Server TCP per File di Testo (Traccia Riga Specifica)

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define MAX_LINES 1000
#define MAX_LINE_LENGTH 256

// Array globale per memorizzare righe del file
char file_lines[MAX_LINES][MAX_LINE_LENGTH];
int total_lines = 0;

// Carica file in memoria
int carica_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Errore apertura file");
        return -1;
    }
    
    total_lines = 0;
    while (fgets(file_lines[total_lines], MAX_LINE_LENGTH, file) && 
           total_lines < MAX_LINES) {
        // Rimuovi newline finale
        file_lines[total_lines][strcspn(file_lines[total_lines], "\n")] = 0;
        total_lines++;
    }
    
    fclose(file);
    printf("📄 Caricato file '%s' con %d righe\n", filename, total_lines);
    return total_lines;
}

// Invia riga specifica al client
void invia_riga(int client_fd, int numero_riga) {
    char response[MAX_LINE_LENGTH + 50];
    
    if (numero_riga >= 1 && numero_riga <= total_lines) {
        // Riga valida - invia contenuto
        snprintf(response, sizeof(response), 
                "RIGA_%d: %s", numero_riga, file_lines[numero_riga - 1]);
        send(client_fd, response, strlen(response), 0);
        
        printf("📤 Inviata riga %d al client [%d]: '%s'\n", 
               numero_riga, client_fd, file_lines[numero_riga - 1]);
    } else {
        // Riga non valida
        snprintf(response, sizeof(response), 
                "ERRORE: Riga %d non esiste (file ha %d righe)", 
                numero_riga, total_lines);
        send(client_fd, response, strlen(response), 0);
        
        printf("❌ Richiesta riga %d non valida dal client [%d]\n", 
               numero_riga, client_fd);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Uso: %s <nome_file.txt>\n", argv[0]);
        return 1;
    }
    
    // Carica file in memoria
    if (carica_file(argv[1]) < 0) {
        return 1;
    }
    
    int server_fd, client_sockets[MAX_CLIENTS];
    fd_set read_fds, master_fds;
    int max_fd, activity, new_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[1024];
    int opt = 1;
    
    // Inizializza
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;
    }
    
    // Crea server
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_fd, MAX_CLIENTS);
    
    printf("📚 Server file di testo avviato su porta %d\n", PORT);
    printf("📖 File caricato: %s (%d righe)\n", argv[1], total_lines);
    
    // Select loop
    FD_ZERO(&master_fds);
    FD_SET(server_fd, &master_fds);
    max_fd = server_fd;
    
    while (1) {
        read_fds = master_fds;
        activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        
        // Nuova connessione
        if (FD_ISSET(server_fd, &read_fds)) {
            new_socket = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    FD_SET(new_socket, &master_fds);
                    
                    if (new_socket > max_fd) {
                        max_fd = new_socket;
                    }
                    
                    printf("✅ Client [%d] connesso\n", new_socket);
                    
                    // Invia istruzioni
                    snprintf(buffer, sizeof(buffer), 
                            "📚 Benvenuto! File con %d righe. "
                            "Invia numero riga (1-%d):\n", 
                            total_lines, total_lines);
                    send(new_socket, buffer, strlen(buffer), 0);
                    break;
                }
            }
        }
        
        // Gestisci richieste client
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int client_fd = client_sockets[i];
            
            if (client_fd > 0 && FD_ISSET(client_fd, &read_fds)) {
                int bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
                
                if (bytes_read <= 0) {
                    printf("👋 Client [%d] disconnesso\n", client_fd);
                    close(client_fd);
                    FD_CLR(client_fd, &master_fds);
                    client_sockets[i] = 0;
                } else {
                    buffer[bytes_read] = '\0';
                    
                    // Converte richiesta in numero
                    int numero_riga = atoi(buffer);
                    printf("📥 Client [%d] richiede riga %d\n", client_fd, numero_riga);
                    
                    // Invia riga richiesta
                    invia_riga(client_fd, numero_riga);
                    
                    // CHIUDI CONNESSIONE dopo invio (come da traccia)
                    printf("🔒 Chiudo connessione client [%d] dopo invio\n", client_fd);
                    close(client_fd);
                    FD_CLR(client_fd, &master_fds);
                    client_sockets[i] = 0;
                }
            }
        }
    }
    
    return 0;
}
```

---

## 🛠️ Funzioni Utility per Server TCP

```c
// Imposta socket non-bloccante
#include <fcntl.h>
int set_nonblocking(int socket_fd) {
    int flags = fcntl(socket_fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
}

// Conta client attivi
int conta_client_attivi(int client_sockets[], int max_clients) {
    int count = 0;
    for (int i = 0; i < max_clients; i++) {
        if (client_sockets[i] > 0) count++;
    }
    return count;
}

// Invia messaggio a tutti i client
void broadcast_message(int client_sockets[], int max_clients, 
                      const char* message, int sender_fd) {
    for (int i = 0; i < max_clients; i++) {
        if (client_sockets[i] > 0 && client_sockets[i] != sender_fd) {
            send(client_sockets[i], message, strlen(message), 0);
        }
    }
}

// Trova client per socket
int trova_client_index(int client_sockets[], int max_clients, int socket_fd) {
    for (int i = 0; i < max_clients; i++) {
        if (client_sockets[i] == socket_fd) {
            return i;
        }
    }
    return -1;
}

// Chiudi tutti i client
void chiudi_tutti_client(int client_sockets[], int max_clients) {
    for (int i = 0; i < max_clients; i++) {
        if (client_sockets[i] > 0) {
            close(client_sockets[i]);
            client_sockets[i] = 0;
        }
    }
}
```

---

## 📋 Checklist Server TCP

### ✅ **Server Sequenziale**
- [ ] Un solo client alla volta
- [ ] Loop: `accept() → recv/send loop → close()`
- [ ] Semplice ma non scalabile

### ✅ **Server con Fork**
- [ ] `fork()` dopo ogni `accept()`
- [ ] Processo figlio gestisce client
- [ ] Padre continua ad accettare
- [ ] Gestore `SIGCHLD` per zombie

### ✅ **Server con Thread**
- [ ] `pthread_create()` dopo ogni `accept()`
- [ ] Thread gestisce client
- [ ] Sincronizzazione per dati condivisi
- [ ] `pthread_detach()` per cleanup automatico

### ✅ **Server con Select**
- [ ] `FD_ZERO(&master_fds)` - pulisce set
- [ ] `FD_SET(server_fd, &master_fds)` - aggiunge server
- [ ] Loop: `select() → controlla server → controlla client`
- [ ] Gestione dinamica di `max_fd`

---

## 🎯 Confronto Prestazioni

| **Metrica** | **Sequenziale** | **Fork** | **Thread** | **Select** |
|-------------|-----------------|----------|------------|------------|
| **Concorrenza** | ❌ 1 | ✅ Alta | ✅ Alta | ✅ Altissima |
| **Memoria** | ✅ Minima | ❌ Alta | ⚠️ Media | ✅ Minima |
| **CPU** | ✅ Bassa | ❌ Alta | ⚠️ Media | ✅ Bassa |
| **Scalabilità** | ❌ Pessima | ⚠️ Limitata | ✅ Buona | ✅ Ottima |
| **Complessità** | ✅ Semplice | ⚠️ Media | ⚠️ Media | ❌ Alta |
| **Debugging** | ✅ Facile | ❌ Difficile | ❌ Difficile | ⚠️ Medio |

---

## 🚀 Compilazione e Test

```bash
# Compila server sequenziale
gcc -o server_seq server_sequenziale.c

# Compila server con fork
gcc -o server_fork server_fork.c

# Compila server con thread
gcc -o server_thread server_thread.c -lpthread

# Compila server con select
gcc -o server_select server_select.c

# Test con netcat
nc localhost 8080

# Test multipli client (solo per fork/thread/select)
nc localhost 8080 &
nc localhost 8080 &
nc localhost 8080 &
```

---

## 🎯 Quale Scegliere?

### **Server Sequenziale** 
👍 **Quando**: Prototipo, test, applicazioni molto semplici  
👎 **Evita**: Applicazioni con multiple connessioni

### **Server Fork**
👍 **Quando**: Robustezza critica, client di lunga durata  
👎 **Evita**: Molti client, risorse limitate

### **Server Thread**  
👍 **Quando**: Buon compromesso, condivisione dati tra client  
👎 **Evita**: Quando la sincronizzazione diventa complessa

### **Server Select**
👍 **Quando**: Massime prestazioni, moltissimi client  
👎 **Evita**: Logica molto semplice, team poco esperto

---

## 🏆 Best Practices

1. **Sempre** usa `SO_REUSEADDR` per riavvii rapidi
2. **Gestisci** sempre gli errori di sistema
3. **Valida** input dei client prima di elaborarli  
4. **Implementa** timeout per connessioni inattive
5. **Usa** buffer appropriati per il tuo protocollo
6. **Testa** con tool come `netcat`, `telnet`, `siege`
7. **Monitora** risorse (memoria, file descriptor, CPU)
8. **Implementa** logging per debugging e monitoring