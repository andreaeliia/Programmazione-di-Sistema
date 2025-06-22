# Message Queues IPC - Riferimento Completo

## 🎯 Concetti Fondamentali

### **Message Queues**
- **System V IPC**: meccanismo di comunicazione inter-processo
- **FIFO ordering**: messaggi consegnati nell'ordine di invio
- **Typed messages**: ogni messaggio ha un tipo identificativo
- **Persistent**: rimangono nel sistema anche dopo terminazione processi
- **Kernel managed**: gestite dal kernel del sistema operativo

### **Caratteristiche**
- **Asynchronous**: sender non blocca aspettando receiver
- **Buffered**: messaggi memorizzati in coda kernel
- **Multiple readers**: più processi possono leggere dalla stessa coda
- **Message types**: filtraggio messaggi per tipo
- **Access control**: permessi di lettura/scrittura

### **Componenti Chiave**
- **Message Queue ID**: identificatore univoco coda
- **Key**: chiave per accesso condiviso alla coda
- **Message type**: tipo del messaggio (long)
- **Message data**: payload del messaggio

---

## 📨 Message Queues Base - System V

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <time.h>
#include <signal.h>

#define MSG_KEY 1234
#define MAX_MSG_SIZE 256

// Struttura messaggio base
typedef struct {
    long message_type;              // Tipo messaggio (richiesto da System V)
    char data[MAX_MSG_SIZE];        // Dati del messaggio
} BasicMessage;

// Struttura messaggio completa per applicazioni
typedef struct {
    long message_type;              // Tipo: 1=command, 2=response, 3=notification
    int sender_pid;                 // PID del sender
    time_t timestamp;               // Timestamp invio
    int sequence_number;            // Numero sequenziale
    char payload[MAX_MSG_SIZE - 20]; // Dati effettivi (spazio per header)
} ApplicationMessage;

// Tipi di messaggio predefiniti
#define MSG_TYPE_COMMAND      1
#define MSG_TYPE_RESPONSE     2
#define MSG_TYPE_NOTIFICATION 3
#define MSG_TYPE_SHUTDOWN     99

// Esempio base message queue
void basic_message_queue_example() {
    printf("=== ESEMPIO BASE MESSAGE QUEUE ===\n");
    
    key_t key = MSG_KEY;
    int msgid;
    BasicMessage msg;
    
    // 1. CREAZIONE MESSAGE QUEUE
    msgid = msgget(key, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("msgget");
        return;
    }
    
    printf("✅ Message queue creata: ID=%d, Key=%d\n", msgid, key);
    
    // 2. INVIO MESSAGGI
    printf("\n📤 Invio messaggi...\n");
    
    // Messaggio tipo 1
    msg.message_type = 1;
    strcpy(msg.data, "Primo messaggio di test");
    
    if (msgsnd(msgid, &msg, strlen(msg.data) + 1, 0) == -1) {
        perror("msgsnd");
    } else {
        printf("Inviato messaggio tipo %ld: '%s'\n", msg.message_type, msg.data);
    }
    
    // Messaggio tipo 2
    msg.message_type = 2;
    strcpy(msg.data, "Secondo messaggio diverso tipo");
    
    if (msgsnd(msgid, &msg, strlen(msg.data) + 1, 0) == -1) {
        perror("msgsnd");
    } else {
        printf("Inviato messaggio tipo %ld: '%s'\n", msg.message_type, msg.data);
    }
    
    // Messaggio tipo 1 (altro)
    msg.message_type = 1;
    strcpy(msg.data, "Terzo messaggio tipo 1");
    
    if (msgsnd(msgid, &msg, strlen(msg.data) + 1, 0) == -1) {
        perror("msgsnd");
    } else {
        printf("Inviato messaggio tipo %ld: '%s'\n", msg.message_type, msg.data);
    }
    
    // 3. RICEZIONE MESSAGGI
    printf("\n📥 Ricezione messaggi...\n");
    
    // Ricevi primo messaggio tipo 1
    if (msgrcv(msgid, &msg, MAX_MSG_SIZE, 1, 0) != -1) {
        printf("Ricevuto messaggio tipo %ld: '%s'\n", msg.message_type, msg.data);
    }
    
    // Ricevi qualsiasi messaggio (tipo 0)
    if (msgrcv(msgid, &msg, MAX_MSG_SIZE, 0, 0) != -1) {
        printf("Ricevuto messaggio (any type) tipo %ld: '%s'\n", msg.message_type, msg.data);
    }
    
    // Ricevi altro messaggio tipo 1
    if (msgrcv(msgid, &msg, MAX_MSG_SIZE, 1, 0) != -1) {
        printf("Ricevuto messaggio tipo %ld: '%s'\n", msg.message_type, msg.data);
    }
    
    // 4. STATISTICHE MESSAGE QUEUE
    struct msqid_ds queue_stats;
    if (msgctl(msgid, IPC_STAT, &queue_stats) == 0) {
        printf("\n📊 Statistiche message queue:\n");
        printf("   Messaggi in coda: %lu\n", queue_stats.msg_qnum);
        printf("   Bytes in coda: %lu\n", queue_stats.msg_cbytes);
        printf("   Max bytes: %lu\n", queue_stats.msg_qbytes);
        printf("   Ultimo send PID: %d\n", queue_stats.msg_lspid);
        printf("   Ultimo recv PID: %d\n", queue_stats.msg_lrpid);
        printf("   Ultimo send time: %s", ctime(&queue_stats.msg_stime));
        printf("   Ultimo recv time: %s", ctime(&queue_stats.msg_rtime));
    }
    
    // 5. RIMOZIONE MESSAGE QUEUE
    if (msgctl(msgid, IPC_RMID, NULL) == 0) {
        printf("\n🗑️  Message queue rimossa\n");
    } else {
        perror("msgctl IPC_RMID");
    }
}

// Sender process
void message_sender_process() {
    printf("=== PROCESSO SENDER ===\n");
    
    key_t key = MSG_KEY;
    int msgid;
    ApplicationMessage msg;
    int sequence = 1;
    
    // Accedi alla message queue esistente
    msgid = msgget(key, 0);
    if (msgid == -1) {
        // Se non esiste, creala
        msgid = msgget(key, IPC_CREAT | 0666);
        if (msgid == -1) {
            perror("msgget sender");
            return;
        }
        printf("📬 Message queue creata dal sender: ID=%d\n", msgid);
    } else {
        printf("📬 Accesso a message queue esistente: ID=%d\n", msgid);
    }
    
    printf("📤 Sender avviato (PID: %d)\n", getpid());
    
    // Invia diversi tipi di messaggi
    const char* commands[] = {
        "START_PROCESS",
        "PROCESS_DATA file1.txt", 
        "PROCESS_DATA file2.txt",
        "GET_STATUS",
        "STOP_PROCESS"
    };
    
    int num_commands = sizeof(commands) / sizeof(commands[0]);
    
    for (int i = 0; i < num_commands; i++) {
        // Prepara messaggio
        msg.message_type = MSG_TYPE_COMMAND;
        msg.sender_pid = getpid();
        msg.timestamp = time(NULL);
        msg.sequence_number = sequence++;
        strncpy(msg.payload, commands[i], sizeof(msg.payload) - 1);
        msg.payload[sizeof(msg.payload) - 1] = '\0';
        
        // Invia messaggio
        size_t msg_size = sizeof(ApplicationMessage) - sizeof(long);
        if (msgsnd(msgid, &msg, msg_size, 0) == 0) {
            printf("📤 [%d] Inviato comando: '%s'\n", sequence - 1, commands[i]);
        } else {
            perror("msgsnd command");
        }
        
        sleep(1);  // Pausa tra comandi
    }
    
    // Invia messaggio di shutdown
    msg.message_type = MSG_TYPE_SHUTDOWN;
    msg.sender_pid = getpid();
    msg.timestamp = time(NULL);
    msg.sequence_number = sequence++;
    strcpy(msg.payload, "SHUTDOWN_REQUEST");
    
    size_t msg_size = sizeof(ApplicationMessage) - sizeof(long);
    if (msgsnd(msgid, &msg, msg_size, 0) == 0) {
        printf("📤 [%d] Inviato shutdown\n", sequence - 1);
    }
    
    printf("📤 Sender completato\n");
}

// Receiver process
void message_receiver_process() {
    printf("=== PROCESSO RECEIVER ===\n");
    
    key_t key = MSG_KEY;
    int msgid;
    ApplicationMessage msg;
    int running = 1;
    
    // Accedi alla message queue
    msgid = msgget(key, 0);
    if (msgid == -1) {
        printf("❌ Message queue non trovata\n");
        return;
    }
    
    printf("📬 Accesso a message queue: ID=%d\n", msgid);
    printf("📥 Receiver avviato (PID: %d)\n", getpid());
    
    while (running) {
        // Ricevi qualsiasi messaggio
        size_t msg_size = sizeof(ApplicationMessage) - sizeof(long);
        ssize_t received = msgrcv(msgid, &msg, msg_size, 0, 0);
        
        if (received == -1) {
            if (errno == EIDRM) {
                printf("📪 Message queue rimossa\n");
                break;
            } else {
                perror("msgrcv");
                break;
            }
        }
        
        // Elabora messaggio in base al tipo
        switch (msg.message_type) {
            case MSG_TYPE_COMMAND:
                printf("📥 [%d] Comando da PID %d: '%s' (time: %ld)\n",
                       msg.sequence_number, msg.sender_pid, msg.payload, msg.timestamp);
                
                // Simula elaborazione comando
                sleep(1);
                
                // Invia risposta
                ApplicationMessage response;
                response.message_type = MSG_TYPE_RESPONSE;
                response.sender_pid = getpid();
                response.timestamp = time(NULL);
                response.sequence_number = msg.sequence_number;
                snprintf(response.payload, sizeof(response.payload), 
                        "OK: Comando '%s' elaborato", msg.payload);
                
                if (msgsnd(msgid, &response, msg_size, 0) == 0) {
                    printf("📤 Inviata risposta per comando %d\n", msg.sequence_number);
                }
                break;
                
            case MSG_TYPE_RESPONSE:
                printf("📥 Risposta: '%s'\n", msg.payload);
                break;
                
            case MSG_TYPE_NOTIFICATION:
                printf("📥 Notifica: '%s'\n", msg.payload);
                break;
                
            case MSG_TYPE_SHUTDOWN:
                printf("📥 Ricevuto shutdown: '%s'\n", msg.payload);
                running = 0;
                break;
                
            default:
                printf("📥 Messaggio tipo sconosciuto %ld: '%s'\n", 
                       msg.message_type, msg.payload);
                break;
        }
    }
    
    printf("📥 Receiver terminato\n");
}

// Server message queue multiprocesso
void message_queue_server_example() {
    printf("=== EXAMPLE MULTIPROCESSO MESSAGE QUEUE ===\n");
    
    pid_t sender_pid, receiver_pid;
    
    // Fork per receiver
    receiver_pid = fork();
    if (receiver_pid == 0) {
        // Processo receiver
        sleep(1);  // Aspetta che sender crei queue
        message_receiver_process();
        exit(0);
    } else if (receiver_pid > 0) {
        // Fork per sender
        sender_pid = fork();
        if (sender_pid == 0) {
            // Processo sender
            message_sender_process();
            exit(0);
        } else if (sender_pid > 0) {
            // Processo padre - aspetta completion
            printf("👨 Processo padre: avviati sender (PID %d) e receiver (PID %d)\n", 
                   sender_pid, receiver_pid);
            
            // Aspetta sender
            int status;
            waitpid(sender_pid, &status, 0);
            printf("👨 Sender completato\n");
            
            // Aspetta receiver
            waitpid(receiver_pid, &status, 0);
            printf("👨 Receiver completato\n");
            
            // Cleanup finale message queue (se esiste ancora)
            key_t key = MSG_KEY;
            int msgid = msgget(key, 0);
            if (msgid != -1) {
                msgctl(msgid, IPC_RMID, NULL);
                printf("👨 Message queue pulita dal padre\n");
            }
            
        } else {
            perror("fork sender");
        }
    } else {
        perror("fork receiver");
    }
}

// Message queue con timeout
int msgrcv_timeout(int msgid, void* msgp, size_t msgsz, long msgtyp, int timeout_sec) {
    // Imposta alarm
    alarm(timeout_sec);
    
    // Salva handler precedente
    void (*old_handler)(int) = signal(SIGALRM, SIG_DFL);
    
    int result = msgrcv(msgid, msgp, msgsz, msgtyp, 0);
    int saved_errno = errno;
    
    // Cancella alarm
    alarm(0);
    signal(SIGALRM, old_handler);
    
    errno = saved_errno;
    return result;
}

// Priority message queue
typedef struct {
    long message_type;      // Priority: 1=high, 2=medium, 3=low
    int priority;
    char task[200];
    int task_id;
} PriorityMessage;

void priority_message_example() {
    printf("\n=== EXAMPLE PRIORITY MESSAGE QUEUE ===\n");
    
    key_t key = 5678;
    int msgid;
    PriorityMessage msg;
    
    // Crea queue
    msgid = msgget(key, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("msgget priority");
        return;
    }
    
    printf("📬 Priority queue creata: ID=%d\n", msgid);
    
    // Invia messaggi con priorità diverse
    const char* tasks[] = {
        "Task bassa priorità 1",
        "Task alta priorità 1", 
        "Task media priorità 1",
        "Task bassa priorità 2",
        "Task alta priorità 2"
    };
    
    int priorities[] = {3, 1, 2, 3, 1};  // 1=high, 2=medium, 3=low
    int num_tasks = sizeof(tasks) / sizeof(tasks[0]);
    
    printf("\n📤 Invio task con priorità:\n");
    for (int i = 0; i < num_tasks; i++) {
        msg.message_type = priorities[i];  // Usa priority come message type
        msg.priority = priorities[i];
        msg.task_id = i + 1;
        strcpy(msg.task, tasks[i]);
        
        size_t msg_size = sizeof(PriorityMessage) - sizeof(long);
        if (msgsnd(msgid, &msg, msg_size, 0) == 0) {
            printf("   Task %d: priority %d - '%s'\n", i + 1, priorities[i], tasks[i]);
        }
    }
    
    // Ricevi messaggi per priorità (prima high, poi medium, poi low)
    printf("\n📥 Elaborazione per priorità:\n");
    
    for (int priority = 1; priority <= 3; priority++) {
        printf("   Priorità %d:\n", priority);
        
        while (1) {
            size_t msg_size = sizeof(PriorityMessage) - sizeof(long);
            if (msgrcv(msgid, &msg, msg_size, priority, IPC_NOWAIT) == -1) {
                if (errno == ENOMSG) {
                    break;  // Nessun messaggio di questa priorità
                } else {
                    perror("msgrcv priority");
                    break;
                }
            }
            
            printf("     Task %d: '%s'\n", msg.task_id, msg.task);
        }
    }
    
    // Cleanup
    msgctl(msgid, IPC_RMID, NULL);
    printf("\n🗑️  Priority queue rimossa\n");
}
```

---

## 🔧 Message Queue Avanzate

```c
// Message queue con acknowledgment
typedef struct {
    long message_type;
    int msg_id;
    int sender_pid;
    int requires_ack;
    time_t timestamp;
    char payload[200];
} ReliableMessage;

#define MSG_TYPE_DATA 1
#define MSG_TYPE_ACK  2

typedef struct {
    int msg_id;
    time_t sent_time;
    int retries;
    ReliableMessage original_msg;
} PendingMessage;

typedef struct {
    int msgid;
    PendingMessage pending[100];
    int pending_count;
    int next_msg_id;
    pthread_mutex_t mutex;
} ReliableQueue;

ReliableQueue* create_reliable_queue(key_t key) {
    ReliableQueue* rq = malloc(sizeof(ReliableQueue));
    
    rq->msgid = msgget(key, IPC_CREAT | 0666);
    if (rq->msgid == -1) {
        free(rq);
        return NULL;
    }
    
    rq->pending_count = 0;
    rq->next_msg_id = 1;
    pthread_mutex_init(&rq->mutex, NULL);
    
    return rq;
}

int reliable_send(ReliableQueue* rq, const char* data, int requires_ack) {
    ReliableMessage msg;
    
    pthread_mutex_lock(&rq->mutex);
    
    msg.message_type = MSG_TYPE_DATA;
    msg.msg_id = rq->next_msg_id++;
    msg.sender_pid = getpid();
    msg.requires_ack = requires_ack;
    msg.timestamp = time(NULL);
    strncpy(msg.payload, data, sizeof(msg.payload) - 1);
    msg.payload[sizeof(msg.payload) - 1] = '\0';
    
    size_t msg_size = sizeof(ReliableMessage) - sizeof(long);
    int result = msgsnd(rq->msgid, &msg, msg_size, 0);
    
    if (result == 0 && requires_ack && rq->pending_count < 100) {
        // Aggiungi a pending list
        PendingMessage* pending = &rq->pending[rq->pending_count];
        pending->msg_id = msg.msg_id;
        pending->sent_time = time(NULL);
        pending->retries = 0;
        pending->original_msg = msg;
        rq->pending_count++;
    }
    
    pthread_mutex_unlock(&rq->mutex);
    
    return (result == 0) ? msg.msg_id : -1;
}

void send_ack(ReliableQueue* rq, int msg_id, int original_sender) {
    ReliableMessage ack;
    
    ack.message_type = MSG_TYPE_ACK;
    ack.msg_id = msg_id;
    ack.sender_pid = getpid();
    ack.requires_ack = 0;
    ack.timestamp = time(NULL);
    snprintf(ack.payload, sizeof(ack.payload), "ACK for message %d", msg_id);
    
    size_t msg_size = sizeof(ReliableMessage) - sizeof(long);
    msgsnd(rq->msgid, &ack, msg_size, 0);
}

int reliable_receive(ReliableQueue* rq, char* buffer, size_t buffer_size) {
    ReliableMessage msg;
    size_t msg_size = sizeof(ReliableMessage) - sizeof(long);
    
    if (msgrcv(rq->msgid, &msg, msg_size, 0, 0) == -1) {
        return -1;
    }
    
    if (msg.message_type == MSG_TYPE_DATA) {
        strncpy(buffer, msg.payload, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        
        if (msg.requires_ack) {
            send_ack(rq, msg.msg_id, msg.sender_pid);
        }
        
        return msg.msg_id;
        
    } else if (msg.message_type == MSG_TYPE_ACK) {
        // Rimuovi da pending list
        pthread_mutex_lock(&rq->mutex);
        
        for (int i = 0; i < rq->pending_count; i++) {
            if (rq->pending[i].msg_id == msg.msg_id) {
                // Sposta ultimo elemento in questa posizione
                rq->pending[i] = rq->pending[rq->pending_count - 1];
                rq->pending_count--;
                break;
            }
        }
        
        pthread_mutex_unlock(&rq->mutex);
        
        printf("✅ ACK ricevuto per messaggio %d\n", msg.msg_id);
        return 0;  // ACK processed
    }
    
    return -1;
}

// Broadcast message queue
typedef struct {
    long message_type;  // Sempre 1 per broadcast
    int sender_pid;
    int broadcast_id;
    time_t timestamp;
    char topic[32];
    char message[200];
} BroadcastMessage;

void broadcast_send(int msgid, const char* topic, const char* message) {
    static int broadcast_counter = 1;
    BroadcastMessage msg;
    
    msg.message_type = 1;  // Tutti ricevono tipo 1
    msg.sender_pid = getpid();
    msg.broadcast_id = broadcast_counter++;
    msg.timestamp = time(NULL);
    strncpy(msg.topic, topic, sizeof(msg.topic) - 1);
    msg.topic[sizeof(msg.topic) - 1] = '\0';
    strncpy(msg.message, message, sizeof(msg.message) - 1);
    msg.message[sizeof(msg.message) - 1] = '\0';
    
    size_t msg_size = sizeof(BroadcastMessage) - sizeof(long);
    msgsnd(msgid, &msg, msg_size, 0);
    
    printf("📡 Broadcast [%d] topic '%s': %s\n", msg.broadcast_id, topic, message);
}

int broadcast_receive(int msgid, char* topic, char* message, int timeout_sec) {
    BroadcastMessage msg;
    size_t msg_size = sizeof(BroadcastMessage) - sizeof(long);
    
    int result;
    if (timeout_sec > 0) {
        result = msgrcv_timeout(msgid, &msg, msg_size, 1, timeout_sec);
    } else {
        result = msgrcv(msgid, &msg, msg_size, 1, 0);
    }
    
    if (result != -1) {
        if (topic) strcpy(topic, msg.topic);
        if (message) strcpy(message, msg.message);
        return msg.broadcast_id;
    }
    
    return -1;
}

// Message queue monitor
void print_queue_info(int msgid) {
    struct msqid_ds info;
    
    if (msgctl(msgid, IPC_STAT, &info) == 0) {
        printf("📊 Queue Info:\n");
        printf("   Messages: %lu\n", info.msg_qnum);
        printf("   Bytes: %lu / %lu\n", info.msg_cbytes, info.msg_qbytes);
        printf("   Last send PID: %d\n", info.msg_lspid);
        printf("   Last recv PID: %d\n", info.msg_lrpid);
        
        if (info.msg_stime > 0) {
            printf("   Last send: %s", ctime(&info.msg_stime));
        }
        if (info.msg_rtime > 0) {
            printf("   Last recv: %s", ctime(&info.msg_rtime));
        }
    } else {
        perror("msgctl IPC_STAT");
    }
}

// Lista tutte le message queue del sistema
void list_system_message_queues() {
    printf("=== MESSAGE QUEUE DI SISTEMA ===\n");
    
    // Questo è un metodo non portabile ma funziona su Linux
    system("ipcs -q");
    
    // Metodo più portabile ma limitato
    printf("\n🔍 Tentativo accesso queue con chiavi note:\n");
    
    for (int key = 1000; key < 2000; key += 100) {
        int msgid = msgget(key, 0);
        if (msgid != -1) {
            printf("   Key %d -> Queue ID %d\n", key, msgid);
            print_queue_info(msgid);
        }
    }
}

// Cleanup tutte le message queue create
void cleanup_message_queues() {
    printf("\n🧹 Cleanup message queue...\n");
    
    // Lista chiavi note usate negli esempi
    key_t known_keys[] = {MSG_KEY, 5678, 9999};
    int num_keys = sizeof(known_keys) / sizeof(known_keys[0]);
    
    for (int i = 0; i < num_keys; i++) {
        int msgid = msgget(known_keys[i], 0);
        if (msgid != -1) {
            if (msgctl(msgid, IPC_RMID, NULL) == 0) {
                printf("🗑️  Rimossa queue key=%d, id=%d\n", known_keys[i], msgid);
            }
        }
    }
}

// Stress test message queue
void stress_test_message_queue() {
    printf("\n=== STRESS TEST MESSAGE QUEUE ===\n");
    
    key_t key = 9999;
    int msgid = msgget(key, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("msgget stress test");
        return;
    }
    
    const int num_messages = 1000;
    BasicMessage msg;
    clock_t start_time, end_time;
    
    // Test invio
    printf("📤 Test invio %d messaggi...\n", num_messages);
    start_time = clock();
    
    for (int i = 0; i < num_messages; i++) {
        msg.message_type = 1;
        snprintf(msg.data, sizeof(msg.data), "Stress test message %d", i);
        
        if (msgsnd(msgid, &msg, strlen(msg.data) + 1, 0) == -1) {
            printf("❌ Errore invio messaggio %d\n", i);
            break;
        }
        
        if (i % 100 == 0) {
            printf("   Inviati %d messaggi\n", i);
        }
    }
    
    end_time = clock();
    double send_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("✅ Invio completato in %.2f secondi (%.0f msg/sec)\n", 
           send_time, num_messages / send_time);
    
    // Verifica queue
    print_queue_info(msgid);
    
    // Test ricezione
    printf("\n📥 Test ricezione %d messaggi...\n", num_messages);
    start_time = clock();
    
    for (int i = 0; i < num_messages; i++) {
        if (msgrcv(msgid, &msg, sizeof(msg.data), 0, 0) == -1) {
            printf("❌ Errore ricezione messaggio %d\n", i);
            break;
        }
        
        if (i % 100 == 0) {
            printf("   Ricevuti %d messaggi\n", i);
        }
    }
    
    end_time = clock();
    double recv_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("✅ Ricezione completata in %.2f secondi (%.0f msg/sec)\n", 
           recv_time, num_messages / recv_time);
    
    // Cleanup
    msgctl(msgid, IPC_RMID, NULL);
    printf("🗑️  Queue stress test rimossa\n");
}
```

---

## 🛠️ Utility Message Queue

```c
// Message queue wrapper per facilità d'uso
typedef struct {
    int msgid;
    key_t key;
    int created_by_us;
} MessageQueue;

MessageQueue* mq_open(key_t key, int create) {
    MessageQueue* mq = malloc(sizeof(MessageQueue));
    
    if (create) {
        mq->msgid = msgget(key, IPC_CREAT | IPC_EXCL | 0666);
        if (mq->msgid == -1 && errno == EEXIST) {
            mq->msgid = msgget(key, 0);
            mq->created_by_us = 0;
        } else {
            mq->created_by_us = 1;
        }
    } else {
        mq->msgid = msgget(key, 0);
        mq->created_by_us = 0;
    }
    
    if (mq->msgid == -1) {
        free(mq);
        return NULL;
    }
    
    mq->key = key;
    return mq;
}

int mq_send(MessageQueue* mq, long type, const void* data, size_t size) {
    struct {
        long mtype;
        char mtext[1];
    } *msg = malloc(sizeof(long) + size);
    
    msg->mtype = type;
    memcpy(msg->mtext, data, size);
    
    int result = msgsnd(mq->msgid, msg, size, 0);
    free(msg);
    
    return result;
}

ssize_t mq_receive(MessageQueue* mq, long type, void* buffer, size_t size) {
    struct {
        long mtype;
        char mtext[1];
    } *msg = malloc(sizeof(long) + size);
    
    ssize_t result = msgrcv(mq->msgid, msg, size, type, 0);
    
    if (result > 0) {
        memcpy(buffer, msg->mtext, result);
    }
    
    free(msg);
    return result;
}

void mq_close(MessageQueue* mq) {
    if (mq) {
        if (mq->created_by_us) {
            msgctl(mq->msgid, IPC_RMID, NULL);
        }
        free(mq);
    }
}

// Message serialization helpers
typedef struct {
    int id;
    float value;
    char name[32];
} CustomData;

size_t serialize_custom_data(const CustomData* data, char* buffer) {
    sprintf(buffer, "%d|%.2f|%s", data->id, data->value, data->name);
    return strlen(buffer);
}

int deserialize_custom_data(const char* buffer, CustomData* data) {
    return sscanf(buffer, "%d|%f|%31s", &data->id, &data->value, data->name) == 3;
}

// Message queue with JSON-like format
#include <ctype.h>

typedef struct {
    char key[32];
    char value[64];
} KeyValue;

int parse_message(const char* message, KeyValue* pairs, int max_pairs) {
    char* msg_copy = strdup(message);
    char* token = strtok(msg_copy, ",");
    int count = 0;
    
    while (token && count < max_pairs) {
        char* eq = strchr(token, '=');
        if (eq) {
            *eq = '\0';
            strncpy(pairs[count].key, token, sizeof(pairs[count].key) - 1);
            strncpy(pairs[count].value, eq + 1, sizeof(pairs[count].value) - 1);
            count++;
        }
        token = strtok(NULL, ",");
    }
    
    free(msg_copy);
    return count;
}

char* build_message(const KeyValue* pairs, int count, char* buffer, size_t size) {
    buffer[0] = '\0';
    
    for (int i = 0; i < count; i++) {
        if (i > 0) strcat(buffer, ",");
        strcat(buffer, pairs[i].key);
        strcat(buffer, "=");
        strcat(buffer, pairs[i].value);
    }
    
    return buffer;
}

// Message queue statistics collector
typedef struct {
    long total_sent;
    long total_received;
    long bytes_sent;
    long bytes_received;
    time_t start_time;
    double avg_send_time;
    double avg_recv_time;
} MessageQueueStats;

MessageQueueStats* create_mq_stats() {
    MessageQueueStats* stats = calloc(1, sizeof(MessageQueueStats));
    stats->start_time = time(NULL);
    return stats;
}

void update_send_stats(MessageQueueStats* stats, size_t bytes, double elapsed_time) {
    stats->total_sent++;
    stats->bytes_sent += bytes;
    stats->avg_send_time = (stats->avg_send_time * (stats->total_sent - 1) + elapsed_time) / stats->total_sent;
}

void update_recv_stats(MessageQueueStats* stats, size_t bytes, double elapsed_time) {
    stats->total_received++;
    stats->bytes_received += bytes;
    stats->avg_recv_time = (stats->avg_recv_time * (stats->total_received - 1) + elapsed_time) / stats->total_received;
}

void print_mq_stats(MessageQueueStats* stats) {
    time_t uptime = time(NULL) - stats->start_time;
    
    printf("📊 Message Queue Statistics:\n");
    printf("   Uptime: %ld seconds\n", uptime);
    printf("   Messages sent: %ld (%.2f/sec)\n", stats->total_sent, 
           uptime > 0 ? (double)stats->total_sent / uptime : 0);
    printf("   Messages received: %ld (%.2f/sec)\n", stats->total_received,
           uptime > 0 ? (double)stats->total_received / uptime : 0);
    printf("   Bytes sent: %ld\n", stats->bytes_sent);
    printf("   Bytes received: %ld\n", stats->bytes_received);
    printf("   Avg send time: %.4f ms\n", stats->avg_send_time * 1000);
    printf("   Avg recv time: %.4f ms\n", stats->avg_recv_time * 1000);
}
```

---

## 📋 Checklist Message Queues

### ✅ **Creazione e Accesso**
- [ ] `msgget()` con chiave appropriata
- [ ] Usa `IPC_CREAT` per creare nuove code
- [ ] Gestisci `EEXIST` se coda già esiste
- [ ] Imposta permessi corretti (0666)
- [ ] Verifica errori su tutte le operazioni

### ✅ **Invio e Ricezione**
- [ ] Struttura messaggio con `long message_type` come primo campo
- [ ] `msgsnd()` per invio, `msgrcv()` per ricezione
- [ ] Calcola dimensione messaggio senza `sizeof(long)`
- [ ] Usa tipi messaggio per filtraggio
- [ ] Gestisci timeout con `IPC_NOWAIT` o alarm

### ✅ **Gestione e Cleanup**
- [ ] `msgctl()` con `IPC_STAT` per statistiche
- [ ] `msgctl()` con `IPC_RMID` per rimozione
- [ ] Cleanup code in signal handler
- [ ] Gestisci queue persistenti tra restart
- [ ] Monitor utilizzo memoria kernel

### ✅ **Best Practices**
- [ ] Definisci tipi messaggio come costanti
- [ ] Implementa acknowledgment per reliability
- [ ] Limita dimensione messaggi
- [ ] Gestisci code piene con strategie appropriate
- [ ] Documenta protocollo messaggi

---

## 🎯 Debug e Monitoring

```bash
# Lista message queue sistema
ipcs -q

# Rimuovi tutte le queue di un utente
ipcrm -q $(ipcs -q | grep $USER | awk '{print $2}')

# Monitor utilizzo
watch -n 1 'ipcs -q'

# Limiti sistema
cat /proc/sys/kernel/msgmax    # Max dimensione singolo messaggio
cat /proc/sys/kernel/msgmnb    # Max bytes per queue
cat /proc/sys/kernel/msgmni    # Max numero queue

# Debug con strace
strace -e trace=msgget,msgsnd,msgrcv,msgctl ./program

# Test performance
time ./message_queue_test
```

## 🚀 Message Queue vs Altri IPC

| **Meccanismo** | **Ordering** | **Buffering** | **Complexity** | **Performance** |
|----------------|--------------|---------------|----------------|-----------------|
| **Pipe** | FIFO | Kernel buffer | Basso | Alto |
| **Message Queue** | FIFO + Type | Kernel queue | Medio | Medio |
| **Shared Memory** | No | No | Alto | Molto alto |
| **Socket** | FIFO | Network stack | Medio | Variabile |
| **Semaphore** | No | No | Basso | Alto |