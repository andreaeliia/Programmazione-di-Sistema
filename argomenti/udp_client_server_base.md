# UDP Client/Server - Guida Base

## Concetti Fondamentali

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

## Server UDP Base

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
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    
    // 1. Crea socket UDP
    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(1);
    }
    
    // 2. Configura indirizzo server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // 3. Bind socket
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(1);
    }
    
    printf("Server UDP in ascolto sulla porta %d\n", PORT);
    
    // 4. Loop principale
    while (1) {
        // Ricevi messaggio
        int bytes_received = recvfrom(server_fd, buffer, BUFFER_SIZE - 1, 0,
                                     (struct sockaddr*)&client_addr, &client_len);
        
        if (bytes_received < 0) {
            perror("recvfrom failed");
            continue;
        }
        
        buffer[bytes_received] = '\0';
        printf("Ricevuto da %s:%d: %s\n", 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port), 
               buffer);
        
        // Invia risposta
        char response[BUFFER_SIZE];
        snprintf(response, BUFFER_SIZE, "Echo: %s", buffer);
        
        sendto(server_fd, response, strlen(response), 0,
               (struct sockaddr*)&client_addr, client_len);
        
        printf("Risposta inviata: %s\n", response);
    }
    
    close(server_fd);
    return 0;
}
```

---

## Client UDP Base

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
    int client_fd;
    struct sockaddr_in server_addr;
    char message[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    
    // 1. Crea socket UDP
    client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_fd < 0) {
        perror("socket failed");
        exit(1);
    }
    
    // 2. Configura indirizzo server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
    
    printf("Client UDP connesso al server %s:%d\n", SERVER_IP, SERVER_PORT);
    
    // 3. Loop di invio messaggi
    while (1) {
        printf("Inserisci messaggio (quit per uscire): ");
        fgets(message, BUFFER_SIZE, stdin);
        
        // Rimuovi newline
        message[strcspn(message, "\n")] = 0;
        
        if (strcmp(message, "quit") == 0) {
            break;
        }
        
        // Invia messaggio
        sendto(client_fd, message, strlen(message), 0,
               (struct sockaddr*)&server_addr, sizeof(server_addr));
        
        // Ricevi risposta
        int bytes_received = recvfrom(client_fd, response, BUFFER_SIZE - 1, 0,
                                     NULL, NULL);
        
        if (bytes_received > 0) {
            response[bytes_received] = '\0';
            printf("Risposta server: %s\n", response);
        }
    }
    
    close(client_fd);
    return 0;
}
```

---

## Comunicazione Messaggi Semplici

### Invio/Ricezione Stringhe

```c
// Server - ricevi stringa
int receive_string(int sockfd, char *buffer, int buffer_size, 
                  struct sockaddr_in *client_addr) {
    socklen_t client_len = sizeof(*client_addr);
    
    int bytes = recvfrom(sockfd, buffer, buffer_size - 1, 0,
                        (struct sockaddr*)client_addr, &client_len);
    
    if (bytes > 0) {
        buffer[bytes] = '\0';
        return bytes;
    }
    return -1;
}

// Client - invia stringa
int send_string(int sockfd, const char *message, 
               struct sockaddr_in *server_addr) {
    return sendto(sockfd, message, strlen(message), 0,
                 (struct sockaddr*)server_addr, sizeof(*server_addr));
}

// Esempio uso
int main() {
    // ... setup socket ...
    
    char buffer[1024];
    struct sockaddr_in client_addr;
    
    // Server riceve
    if (receive_string(server_fd, buffer, sizeof(buffer), &client_addr) > 0) {
        printf("Stringa ricevuta: %s\n", buffer);
        
        // Risposta
        send_string(server_fd, "Messaggio ricevuto", &client_addr);
    }
    
    return 0;
}
```

### Invio/Ricezione Numeri

```c
// Invia numero intero
int send_integer(int sockfd, int number, struct sockaddr_in *addr) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d", number);
    return sendto(sockfd, buffer, strlen(buffer), 0,
                 (struct sockaddr*)addr, sizeof(*addr));
}

// Ricevi numero intero
int receive_integer(int sockfd, struct sockaddr_in *from_addr) {
    char buffer[32];
    socklen_t addr_len = sizeof(*from_addr);
    
    int bytes = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                        (struct sockaddr*)from_addr, &addr_len);
    
    if (bytes > 0) {
        buffer[bytes] = '\0';
        return atoi(buffer);
    }
    return -1;
}

// Esempio server calcoli
int main() {
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    
    // ... setup socket e bind ...
    
    while (1) {
        int num1 = receive_integer(server_fd, &client_addr);
        int num2 = receive_integer(server_fd, &client_addr);
        
        if (num1 >= 0 && num2 >= 0) {
            int risultato = num1 + num2;
            printf("Calcolo: %d + %d = %d\n", num1, num2, risultato);
            send_integer(server_fd, risultato, &client_addr);
        }
    }
    
    return 0;
}
```

### Invio/Ricezione Strutture

```c
// Struttura dati semplice
typedef struct {
    int id;
    char name[32];
    float value;
} SimpleData;

// Invia struttura
int send_data(int sockfd, SimpleData *data, struct sockaddr_in *addr) {
    return sendto(sockfd, data, sizeof(SimpleData), 0,
                 (struct sockaddr*)addr, sizeof(*addr));
}

// Ricevi struttura
int receive_data(int sockfd, SimpleData *data, struct sockaddr_in *from_addr) {
    socklen_t addr_len = sizeof(*from_addr);
    
    int bytes = recvfrom(sockfd, data, sizeof(SimpleData), 0,
                        (struct sockaddr*)from_addr, &addr_len);
    
    return (bytes == sizeof(SimpleData)) ? 0 : -1;
}

// Esempio uso
int main() {
    // ... setup ...
    
    SimpleData data;
    struct sockaddr_in client_addr;
    
    // Server riceve struttura
    if (receive_data(server_fd, &data, &client_addr) == 0) {
        printf("Ricevuto - ID: %d, Nome: %s, Valore: %.2f\n",
               data.id, data.name, data.value);
        
        // Modifica e rispedisci
        data.value *= 2;
        send_data(server_fd, &data, &client_addr);
    }
    
    return 0;
}
```

---

## Esempi di Comunicazione Base

### Server Echo Semplice

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[1024];
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    printf("Server echo avviato sulla porta %d\n", PORT);
    
    while (1) {
        int n = recvfrom(sockfd, buffer, 1024, 0,
                        (struct sockaddr*)&client_addr, &client_len);
        
        if (n > 0) {
            buffer[n] = '\0';
            printf("Echo: %s\n", buffer);
            
            sendto(sockfd, buffer, n, 0,
                  (struct sockaddr*)&client_addr, client_len);
        }
    }
    
    return 0;
}
```

### Client Invio Messaggi

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char message[1024];
    char response[1024];
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
    
    printf("Inserisci messaggi (quit per uscire):\n");
    
    while (1) {
        printf("> ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = 0;
        
        if (strcmp(message, "quit") == 0) break;
        
        sendto(sockfd, message, strlen(message), 0,
               (struct sockaddr*)&server_addr, sizeof(server_addr));
        
        int n = recvfrom(sockfd, response, sizeof(response), 0, NULL, NULL);
        if (n > 0) {
            response[n] = '\0';
            printf("Risposta: %s\n", response);
        }
    }
    
    close(sockfd);
    return 0;
}
```

### Server Calcolatrice

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[1024];
    char result[1024];
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    
    printf("Server calcolatrice avviato\n");
    printf("Formato: numero1 operatore numero2 (es: 5 + 3)\n");
    
    while (1) {
        int n = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                        (struct sockaddr*)&client_addr, &client_len);
        
        if (n > 0) {
            buffer[n] = '\0';
            
            int num1, num2;
            char op;
            
            if (sscanf(buffer, "%d %c %d", &num1, &op, &num2) == 3) {
                int res = 0;
                
                switch (op) {
                    case '+': res = num1 + num2; break;
                    case '-': res = num1 - num2; break;
                    case '*': res = num1 * num2; break;
                    case '/': res = (num2 != 0) ? num1 / num2 : 0; break;
                    default: res = 0; break;
                }
                
                snprintf(result, sizeof(result), "%d %c %d = %d", 
                        num1, op, num2, res);
            } else {
                strcpy(result, "Formato non valido");
            }
            
            printf("Calcolo: %s\n", result);
            sendto(sockfd, result, strlen(result), 0,
                  (struct sockaddr*)&client_addr, client_len);
        }
    }
    
    return 0;
}
```

### Client Test Numeri

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 8080

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char message[1024];
    char response[1024];
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
    
    printf("Client calcolatrice\n");
    printf("Formato: numero1 operatore numero2\n");
    
    while (1) {
        printf("Calcolo> ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = 0;
        
        if (strcmp(message, "quit") == 0) break;
        
        sendto(sockfd, message, strlen(message), 0,
               (struct sockaddr*)&server_addr, sizeof(server_addr));
        
        int n = recvfrom(sockfd, response, sizeof(response), 0, NULL, NULL);
        if (n > 0) {
            response[n] = '\0';
            printf("Risultato: %s\n", response);
        }
    }
    
    close(sockfd);
    return 0;
}
```

---

## Comunicazione con Timeout

### Client con Timeout

```c
#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

int send_with_timeout(int sockfd, const char* message, 
                     struct sockaddr_in* server_addr, 
                     char* response, int timeout_sec) {
    
    // Invia messaggio
    sendto(sockfd, message, strlen(message), 0,
           (struct sockaddr*)server_addr, sizeof(*server_addr));
    
    // Setup timeout per ricezione
    fd_set read_fds;
    struct timeval timeout;
    
    FD_ZERO(&read_fds);
    FD_SET(sockfd, &read_fds);
    
    timeout.tv_sec = timeout_sec;
    timeout.tv_usec = 0;
    
    // Aspetta risposta con timeout
    int activity = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);
    
    if (activity > 0) {
        int n = recvfrom(sockfd, response, 1024, 0, NULL, NULL);
        if (n > 0) {
            response[n] = '\0';
            return n;
        }
    } else if (activity == 0) {
        printf("Timeout - nessuna risposta dal server\n");
    }
    
    return -1;
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char response[1024];
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    
    // Test con timeout di 5 secondi
    if (send_with_timeout(sockfd, "ping", &server_addr, response, 5) > 0) {
        printf("Risposta ricevuta: %s\n", response);
    } else {
        printf("Nessuna risposta dal server\n");
    }
    
    close(sockfd);
    return 0;
}
```

---

## Funzioni di Utilità

### Funzioni Helper Base

```c
// Crea server UDP
int create_udp_server(int port) {
    int sockfd;
    struct sockaddr_in addr;
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) return -1;
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

// Crea client UDP
int create_udp_client() {
    return socket(AF_INET, SOCK_DGRAM, 0);
}

// Configura indirizzo
void setup_server_address(struct sockaddr_in* addr, const char* ip, int port) {
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr->sin_addr);
}

// Esempio uso
int main() {
    int server_fd = create_udp_server(8080);
    if (server_fd < 0) {
        printf("Errore creazione server\n");
        return 1;
    }
    
    struct sockaddr_in client_addr;
    char buffer[1024];
    socklen_t client_len = sizeof(client_addr);
    
    while (1) {
        int n = recvfrom(server_fd, buffer, sizeof(buffer), 0,
                        (struct sockaddr*)&client_addr, &client_len);
        
        if (n > 0) {
            buffer[n] = '\0';
            printf("Messaggio: %s\n", buffer);
            
            sendto(server_fd, "OK", 2, 0,
                  (struct sockaddr*)&client_addr, client_len);
        }
    }
    
    close(server_fd);
    return 0;
}
```

---

## Checklist UDP Programming

### Server UDP Base
- [ ] socket(AF_INET, SOCK_DGRAM, 0) per creare socket
- [ ] bind() su INADDR_ANY per accettare da tutti
- [ ] recvfrom() per ricevere con indirizzo mittente
- [ ] sendto() per rispondere al mittente
- [ ] Loop infinito per servire richieste

### Client UDP Base
- [ ] Socket UDP con indirizzo server configurato
- [ ] sendto() per inviare al server
- [ ] recvfrom() per ricevere risposta (opzionale)
- [ ] Gestione input utente
- [ ] Chiusura socket alla fine

### Comunicazione Base
- [ ] Stringhe: invio/ricezione testo semplice
- [ ] Numeri: conversione con sprintf/sscanf
- [ ] Strutture: invio/ricezione dati binari
- [ ] Timeout: gestione con select()
- [ ] Validazione: controllo dati ricevuti

---

## Compilazione e Test

```bash
# Compila server
gcc -o server server.c

# Compila client
gcc -o client client.c

# Test base
./server &
./client

# Test con netcat
echo "test" | nc -u localhost 8080

# Test multiple client
./client &
./client &
./client &
```

## UDP vs TCP - Quando Usare

| **Scenario** | **Protocollo** | **Motivo** |
|--------------|----------------|------------|
| **File transfer** | TCP | Reliability necessaria |
| **Gaming real-time** | UDP | Velocità più importante |
| **Streaming video** | UDP | Latenza bassa |
| **Chat messages** | TCP | Nessun messaggio perso |
| **DNS queries** | UDP | Semplice richiesta/risposta |
| **HTTP/Web** | TCP | Reliability e ordering |