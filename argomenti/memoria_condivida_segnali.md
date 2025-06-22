# Memoria Condivisa e Segnali - Riferimento Completo

## 🎯 Concetti Fondamentali

### **Cos'è la Memoria Condivisa?**
- **Shared Memory**: segmento di memoria accessibile da più processi
- **IPC veloce**: comunicazione più veloce di pipe/socket
- **Sincronizzazione**: richiede meccanismi per evitare race conditions
- **Persistente**: rimane anche dopo terminazione processo creatore

### **Cos'è un Segnale?**
- **Interrupt software**: notifica asincrona a un processo
- **Event-driven**: scatena esecuzione di handler specifici
- **Tipi**: segnali standard (SIGTERM, SIGINT) e user-defined (SIGUSR1, SIGUSR2)
- **Atomici**: operazioni sui segnali sono atomiche

---

## 🔧 Memoria Condivisa Base - POSIX shm

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#define SHM_NAME "/my_shared_memory"
#define SHM_SIZE 4096

// Struttura dati condivisa
typedef struct {
    int magic_number;           // Per verificare integrità
    pid_t writer_pid;           // PID del processo che scrive
    time_t last_update;         // Timestamp ultimo aggiornamento
    int sequence_number;        // Numero sequenziale per debug
    char data[256];             // Dati effettivi
    int data_ready;             // Flag: dati pronti per lettura
    pthread_mutex_t mutex;      // Mutex per sincronizzazione
} SharedData;

#define MAGIC_NUMBER 0xDEADBEEF

// Crea segmento memoria condivisa
SharedData* create_shared_memory() {
    int shm_fd;
    SharedData* shared_ptr;
    
    // 1. CREA OGGETTO MEMORIA CONDIVISA
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open create");
        return NULL;
    }
    
    // 2. IMPOSTA DIMENSIONE
    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return NULL;
    }
    
    // 3. MAPPA IN MEMORIA
    shared_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_ptr == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return NULL;
    }
    
    close(shm_fd);  // Non serve più dopo mmap
    
    // 4. INIZIALIZZA STRUTTURA
    shared_ptr->magic_number = MAGIC_NUMBER;
    shared_ptr->writer_pid = getpid();
    shared_ptr->last_update = time(NULL);
    shared_ptr->sequence_number = 0;
    shared_ptr->data_ready = 0;
    strcpy(shared_ptr->data, "Memoria condivisa inizializzata");
    
    // Inizializza mutex per sincronizzazione tra processi
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shared_ptr->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    
    printf("✅ Memoria condivisa creata e inizializzata\n");
    printf("📍 Nome: %s, Dimensione: %d bytes\n", SHM_NAME, SHM_SIZE);
    printf("🔢 Magic: 0x%X, PID: %d\n", shared_ptr->magic_number, shared_ptr->writer_pid);
    
    return shared_ptr;
}

// Accedi a memoria condivisa esistente
SharedData* access_shared_memory() {
    int shm_fd;
    SharedData* shared_ptr;
    
    // 1. APRI OGGETTO ESISTENTE
    shm_fd = shm_open(SHM_NAME, O_RDWR, 0);
    if (shm_fd == -1) {
        perror("shm_open access");
        return NULL;
    }
    
    // 2. MAPPA IN MEMORIA
    shared_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_ptr == MAP_FAILED) {
        perror("mmap access");
        close(shm_fd);
        return NULL;
    }
    
    close(shm_fd);
    
    // 3. VERIFICA INTEGRITÀ
    if (shared_ptr->magic_number != MAGIC_NUMBER) {
        printf("❌ Errore: magic number non valido (0x%X)\n", shared_ptr->magic_number);
        munmap(shared_ptr, SHM_SIZE);
        return NULL;
    }
    
    printf("✅ Accesso a memoria condivisa riuscito\n");
    printf("📍 Creator PID: %d, Last update: %ld\n", 
           shared_ptr->writer_pid, shared_ptr->last_update);
    
    return shared_ptr;
}

// Scrivi dati in memoria condivisa (thread-safe)
int write_shared_data(SharedData* shared_ptr, const char* data) {
    if (!shared_ptr) return -1;
    
    // Lock mutex
    if (pthread_mutex_lock(&shared_ptr->mutex) != 0) {
        perror("mutex lock");
        return -1;
    }
    
    // Aggiorna dati
    strncpy(shared_ptr->data, data, sizeof(shared_ptr->data) - 1);
    shared_ptr->data[sizeof(shared_ptr->data) - 1] = '\0';
    shared_ptr->last_update = time(NULL);
    shared_ptr->sequence_number++;
    shared_ptr->writer_pid = getpid();
    shared_ptr->data_ready = 1;
    
    printf("📝 Scritto in memoria condivisa [seq:%d]: '%s'\n", 
           shared_ptr->sequence_number, data);
    
    // Unlock mutex
    pthread_mutex_unlock(&shared_ptr->mutex);
    
    return 0;
}

// Leggi dati da memoria condivisa (thread-safe)
int read_shared_data(SharedData* shared_ptr, char* buffer, size_t buffer_size) {
    if (!shared_ptr || !buffer) return -1;
    
    // Lock mutex
    if (pthread_mutex_lock(&shared_ptr->mutex) != 0) {
        perror("mutex lock");
        return -1;
    }
    
    // Leggi dati se pronti
    if (shared_ptr->data_ready) {
        strncpy(buffer, shared_ptr->data, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        
        printf("📖 Letto da memoria condivisa [seq:%d]: '%s'\n", 
               shared_ptr->sequence_number, buffer);
        
        // Reset flag (optional - dipende dalla logica)
        // shared_ptr->data_ready = 0;
        
        pthread_mutex_unlock(&shared_ptr->mutex);
        return strlen(buffer);
    } else {
        pthread_mutex_unlock(&shared_ptr->mutex);
        return 0;  // Nessun dato pronto
    }
}

// Cleanup memoria condivisa
void cleanup_shared_memory(SharedData* shared_ptr) {
    if (shared_ptr) {
        pthread_mutex_destroy(&shared_ptr->mutex);
        
        if (munmap(shared_ptr, SHM_SIZE) == -1) {
            perror("munmap");
        }
        
        // Rimuovi oggetto (solo il creatore dovrebbe farlo)
        if (shm_unlink(SHM_NAME) == -1) {
            perror("shm_unlink");
        } else {
            printf("🗑️  Memoria condivisa rimossa\n");
        }
    }
}

// Esempio processo writer
int esempio_writer() {
    SharedData* shared_ptr = create_shared_memory();
    if (!shared_ptr) {
        return -1;
    }
    
    printf("🖊️  Processo writer avviato (PID: %d)\n", getpid());
    
    // Scrivi dati periodicamente
    for (int i = 0; i < 10; i++) {
        char message[256];
        snprintf(message, sizeof(message), "Messaggio #%d da PID %d", i + 1, getpid());
        
        write_shared_data(shared_ptr, message);
        
        sleep(2);  // Pausa 2 secondi
    }
    
    cleanup_shared_memory(shared_ptr);
    return 0;
}

// Esempio processo reader
int esempio_reader() {
    // Aspetta che memoria condivisa sia creata
    sleep(1);
    
    SharedData* shared_ptr = access_shared_memory();
    if (!shared_ptr) {
        return -1;
    }
    
    printf("👁️  Processo reader avviato (PID: %d)\n", getpid());
    
    char buffer[256];
    int last_sequence = -1;
    
    // Leggi dati periodicamente
    for (int i = 0; i < 15; i++) {
        int bytes_read = read_shared_data(shared_ptr, buffer, sizeof(buffer));
        
        if (bytes_read > 0 && shared_ptr->sequence_number != last_sequence) {
            printf("📨 Nuovo dato ricevuto: '%s'\n", buffer);
            last_sequence = shared_ptr->sequence_number;
        } else {
            printf("⏳ Nessun nuovo dato...\n");
        }
        
        sleep(1);  // Controlla ogni secondo
    }
    
    munmap(shared_ptr, SHM_SIZE);
    return 0;
}
```

---

## 🔔 Gestione Segnali Base

```c
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>

// Variabili globali per gestione segnali
volatile sig_atomic_t signal_received = 0;
volatile sig_atomic_t signal_type = 0;
volatile sig_atomic_t signal_count = 0;

// Handler generico per segnali
void signal_handler(int sig) {
    signal_received = 1;
    signal_type = sig;
    signal_count++;
    
    // NON usare printf in signal handler! (non async-safe)
    // Solo operazioni async-safe
}

// Handler avanzato con sigaction
void advanced_signal_handler(int sig, siginfo_t* info, void* context) {
    signal_received = 1;
    signal_type = sig;
    signal_count++;
    
    // Informazioni aggiuntive dal sender
    // info->si_pid = PID del processo mittente
    // info->si_uid = UID del processo mittente
}

// Configura handler semplice
void setup_simple_signal_handlers() {
    // Handler per segnali user-defined
    signal(SIGUSR1, signal_handler);
    signal(SIGUSR2, signal_handler);
    
    // Handler per segnali di controllo
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    
    // Ignora SIGPIPE (utile per socket)
    signal(SIGPIPE, SIG_IGN);
    
    printf("📡 Signal handlers configurati\n");
    printf("💡 SIGUSR1=%d, SIGUSR2=%d, SIGTERM=%d, SIGINT=%d\n", 
           SIGUSR1, SIGUSR2, SIGTERM, SIGINT);
}

// Configura handler avanzato con sigaction
void setup_advanced_signal_handlers() {
    struct sigaction sa;
    
    // Configura handler avanzato
    sa.sa_sigaction = advanced_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;  // Abilita informazioni extra
    
    // Installa handler
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    
    printf("📡 Advanced signal handlers configurati\n");
}

// Invia segnale a processo
int send_signal_to_process(pid_t target_pid, int signal_number) {
    if (kill(target_pid, signal_number) == 0) {
        printf("📤 Inviato segnale %d a processo %d\n", signal_number, target_pid);
        return 0;
    } else {
        printf("❌ Errore invio segnale %d a processo %d: %s\n", 
               signal_number, target_pid, strerror(errno));
        return -1;
    }
}

// Aspetta segnale con timeout
int wait_for_signal_timeout(int timeout_seconds) {
    time_t start_time = time(NULL);
    
    while (!signal_received) {
        if (time(NULL) - start_time > timeout_seconds) {
            return -1;  // Timeout
        }
        usleep(100000);  // 100ms
    }
    
    return signal_type;
}

// Main loop gestione segnali
void signal_main_loop() {
    printf("🔄 Loop gestione segnali avviato (PID: %d)\n", getpid());
    printf("💡 Invia segnali con: kill -USR1 %d, kill -USR2 %d\n", getpid(), getpid());
    
    while (1) {
        if (signal_received) {
            time_t now = time(NULL);
            char* time_str = ctime(&now);
            time_str[strlen(time_str) - 1] = '\0';  // Rimuovi newline
            
            switch (signal_type) {
                case SIGUSR1:
                    printf("🟢 [%s] Ricevuto SIGUSR1 (#%d)\n", time_str, signal_count);
                    // Logica specifica per SIGUSR1
                    break;
                    
                case SIGUSR2:
                    printf("🔵 [%s] Ricevuto SIGUSR2 (#%d)\n", time_str, signal_count);
                    // Logica specifica per SIGUSR2
                    break;
                    
                case SIGTERM:
                    printf("🔴 [%s] Ricevuto SIGTERM - terminazione\n", time_str);
                    return;
                    
                case SIGINT:
                    printf("🟡 [%s] Ricevuto SIGINT (Ctrl+C) - terminazione\n", time_str);
                    return;
                    
                default:
                    printf("❓ [%s] Ricevuto segnale sconosciuto: %d\n", time_str, signal_type);
                    break;
            }
            
            // Reset flag
            signal_received = 0;
            signal_type = 0;
        }
        
        // Altra logica del programma
        usleep(500000);  // 500ms
    }
}
```

---

## 🎯 Soluzione Traccia Esame: Memoria Condivisa + Segnali + Thread

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#define SHM_NAME "/exam_shared_memory"
#define SHM_SIZE 4096
#define BUFFER_SIZE 1024

// Struttura memoria condivisa per l'esame
typedef struct {
    pthread_mutex_t mutex;      // Mutex per sincronizzazione
    int writer_turn;            // 0 = processo 1, 1 = processo 2
    char data[BUFFER_SIZE];     // Buffer dati condiviso
    int data_length;            // Lunghezza dati validi
    time_t last_write;          // Timestamp ultima scrittura
    pid_t last_writer;          // PID ultimo writer
    int sequence_number;        // Numero sequenziale scritture
    int process1_writes;        // Contatore scritture processo 1
    int process2_writes;        // Contatore scritture processo 2
} ExamSharedData;

// Variabili globali per comunicazione con thread
volatile sig_atomic_t usr1_received = 0;  // Segnale da processo 1
volatile sig_atomic_t usr2_received = 0;  // Segnale da processo 2
volatile sig_atomic_t terminate_signal = 0;
ExamSharedData* global_shared_ptr = NULL;
int process_id = 0;  // 1 o 2

// Signal handlers (async-safe)
void sigusr1_handler(int sig) {
    usr1_received = 1;
}

void sigusr2_handler(int sig) {
    usr2_received = 1;
}

void sigterm_handler(int sig) {
    terminate_signal = 1;
}

// Thread dedicato alla gestione segnali
void* signal_handling_thread(void* arg) {
    printf("🎧 Thread gestione segnali avviato per processo %d\n", process_id);
    
    while (!terminate_signal) {
        // Controlla segnali ricevuti
        if (usr1_received && process_id == 1) {
            printf("📡 Processo %d: ricevuto permesso scrittura USR1\n", process_id);
            
            // Genera carattere casuale
            char random_char = 'A' + (rand() % 26);
            char message[BUFFER_SIZE];
            snprintf(message, sizeof(message), 
                    "P%d-Char:%c-Time:%ld", process_id, random_char, time(NULL));
            
            // Scrittura in memoria condivisa
            pthread_mutex_lock(&global_shared_ptr->mutex);
            
            if (global_shared_ptr->writer_turn == 0) {  // È il nostro turno
                strcpy(global_shared_ptr->data, message);
                global_shared_ptr->data_length = strlen(message);
                global_shared_ptr->last_write = time(NULL);
                global_shared_ptr->last_writer = getpid();
                global_shared_ptr->sequence_number++;
                global_shared_ptr->process1_writes++;
                global_shared_ptr->writer_turn = 1;  // Passa turno a processo 2
                
                printf("✍️  Processo %d scritto: '%s'\n", process_id, message);
            } else {
                printf("⏳ Processo %d: non è il mio turno, aspetto...\n", process_id);
            }
            
            pthread_mutex_unlock(&global_shared_ptr->mutex);
            usr1_received = 0;  // Reset flag
        }
        
        if (usr2_received && process_id == 2) {
            printf("📡 Processo %d: ricevuto permesso scrittura USR2\n", process_id);
            
            // Genera carattere casuale (numerico)
            char random_char = '0' + (rand() % 10);
            char message[BUFFER_SIZE];
            snprintf(message, sizeof(message), 
                    "P%d-Num:%c-Time:%ld", process_id, random_char, time(NULL));
            
            // Scrittura in memoria condivisa
            pthread_mutex_lock(&global_shared_ptr->mutex);
            
            if (global_shared_ptr->writer_turn == 1) {  // È il nostro turno
                strcpy(global_shared_ptr->data, message);
                global_shared_ptr->data_length = strlen(message);
                global_shared_ptr->last_write = time(NULL);
                global_shared_ptr->last_writer = getpid();
                global_shared_ptr->sequence_number++;
                global_shared_ptr->process2_writes++;
                global_shared_ptr->writer_turn = 0;  // Passa turno a processo 1
                
                printf("✍️  Processo %d scritto: '%s'\n", process_id, message);
            } else {
                printf("⏳ Processo %d: non è il mio turno, aspetto...\n", process_id);
            }
            
            pthread_mutex_unlock(&global_shared_ptr->mutex);
            usr2_received = 0;  // Reset flag
        }
        
        usleep(100000);  // 100ms check interval
    }
    
    printf("🛑 Thread gestione segnali terminato\n");
    return NULL;
}

// Thread per invio periodico segnali (solo processo 1)
void* signal_sender_thread(void* arg) {
    if (process_id != 1) return NULL;  // Solo processo 1 invia segnali
    
    pid_t process2_pid = *(pid_t*)arg;
    printf("📤 Thread invio segnali avviato - target PID: %d\n", process2_pid);
    
    while (!terminate_signal) {
        // Attesa casuale 1-5 secondi
        int wait_time = 1 + (rand() % 5);
        sleep(wait_time);
        
        if (terminate_signal) break;
        
        // Invia USR1 a se stesso
        printf("📡 Invio USR1 a processo 1 (me stesso)\n");
        kill(getpid(), SIGUSR1);
        
        // Attesa casuale 1-3 secondi
        wait_time = 1 + (rand() % 3);
        sleep(wait_time);
        
        if (terminate_signal) break;
        
        // Invia USR2 a processo 2
        printf("📡 Invio USR2 a processo 2 (PID: %d)\n", process2_pid);
        if (kill(process2_pid, SIGUSR2) < 0) {
            printf("❌ Errore invio USR2: %s\n", strerror(errno));
            break;
        }
    }
    
    printf("🛑 Thread invio segnali terminato\n");
    return NULL;
}

// Crea/accede memoria condivisa
ExamSharedData* setup_shared_memory(int create) {
    int shm_fd;
    ExamSharedData* shared_ptr;
    
    if (create) {
        // Crea memoria condivisa
        shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
        if (shm_fd == -1) {
            perror("shm_open create");
            return NULL;
        }
        
        ftruncate(shm_fd, SHM_SIZE);
    } else {
        // Accedi a memoria esistente
        shm_fd = shm_open(SHM_NAME, O_RDWR, 0);
        if (shm_fd == -1) {
            perror("shm_open access");
            return NULL;
        }
    }
    
    shared_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    close(shm_fd);
    
    if (shared_ptr == MAP_FAILED) {
        perror("mmap");
        return NULL;
    }
    
    if (create) {
        // Inizializza struttura
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&shared_ptr->mutex, &attr);
        pthread_mutexattr_destroy(&attr);
        
        shared_ptr->writer_turn = 0;  // Inizia processo 1
        shared_ptr->data_length = 0;
        shared_ptr->sequence_number = 0;
        shared_ptr->process1_writes = 0;
        shared_ptr->process2_writes = 0;
        strcpy(shared_ptr->data, "Memoria condivisa inizializzata");
        
        printf("✅ Memoria condivisa creata e inizializzata\n");
    } else {
        printf("✅ Accesso a memoria condivisa esistente\n");
    }
    
    return shared_ptr;
}

// Thread monitor per statistiche
void* monitor_thread(void* arg) {
    printf("📊 Thread monitor avviato\n");
    
    while (!terminate_signal) {
        sleep(10);  // Report ogni 10 secondi
        
        if (global_shared_ptr) {
            pthread_mutex_lock(&global_shared_ptr->mutex);
            
            printf("\n📈 === STATISTICHE MEMORIA CONDIVISA ===\n");
            printf("🔄 Turno corrente: Processo %d\n", global_shared_ptr->writer_turn + 1);
            printf("📝 Scritture totali: %d\n", global_shared_ptr->sequence_number);
            printf("📝 Processo 1: %d scritture\n", global_shared_ptr->process1_writes);
            printf("📝 Processo 2: %d scritture\n", global_shared_ptr->process2_writes);
            printf("📄 Ultimo dato: '%s'\n", global_shared_ptr->data);
            printf("👤 Ultimo writer PID: %d\n", global_shared_ptr->last_writer);
            printf("⏰ Ultimo aggiornamento: %ld\n", global_shared_ptr->last_write);
            printf("=========================================\n\n");
            
            pthread_mutex_unlock(&global_shared_ptr->mutex);
        }
    }
    
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Uso: %s <1|2>\n", argv[0]);
        printf("  1 = Processo 1 (creatore memoria + sender segnali)\n");
        printf("  2 = Processo 2 (receiver segnali)\n");
        return 1;
    }
    
    process_id = atoi(argv[1]);
    if (process_id != 1 && process_id != 2) {
        printf("❌ Process ID deve essere 1 o 2\n");
        return 1;
    }
    
    srand(time(NULL) + getpid());  // Random seed
    
    printf("🚀 Avvio processo %d (PID: %d)\n", process_id, getpid());
    
    // Setup signal handlers
    signal(SIGUSR1, sigusr1_handler);
    signal(SIGUSR2, sigusr2_handler);
    signal(SIGTERM, sigterm_handler);
    signal(SIGINT, sigterm_handler);
    
    // Setup memoria condivisa
    global_shared_ptr = setup_shared_memory(process_id == 1);
    if (!global_shared_ptr) {
        printf("❌ Errore setup memoria condivisa\n");
        return 1;
    }
    
    // Crea thread gestione segnali
    pthread_t signal_thread, monitor_thread_id, sender_thread_id;
    
    pthread_create(&signal_thread, NULL, signal_handling_thread, NULL);
    pthread_create(&monitor_thread_id, NULL, monitor_thread, NULL);
    
    // Solo processo 1 crea thread sender
    if (process_id == 1) {
        printf("💡 Processo 1: inserisci PID del processo 2: ");
        pid_t process2_pid;
        scanf("%d", &process2_pid);
        
        pthread_create(&sender_thread_id, NULL, signal_sender_thread, &process2_pid);
    }
    
    printf("🎯 Processo %d pronto - premi Ctrl+C per terminare\n", process_id);
    
    // Main loop
    while (!terminate_signal) {
        sleep(1);
    }
    
    printf("🛑 Terminazione processo %d...\n", process_id);
    
    // Termina thread
    pthread_cancel(signal_thread);
    pthread_cancel(monitor_thread_id);
    if (process_id == 1) {
        pthread_cancel(sender_thread_id);
    }
    
    // Cleanup
    munmap(global_shared_ptr, SHM_SIZE);
    if (process_id == 1) {
        shm_unlink(SHM_NAME);
        printf("🗑️  Memoria condivisa rimossa\n");
    }
    
    printf("👋 Processo %d terminato\n", process_id);
    return 0;
}
```

---

## 🛠️ Utility Avanzate

```c
// Sistema robusto di mutex inter-processo
typedef struct {
    pthread_mutex_t mutex;
    pid_t owner_pid;
    time_t lock_time;
    int recursive_count;
} RobustMutex;

int init_robust_mutex(RobustMutex* rmutex) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
    
    int result = pthread_mutex_init(&rmutex->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    
    rmutex->owner_pid = 0;
    rmutex->lock_time = 0;
    rmutex->recursive_count = 0;
    
    return result;
}

// Semaforo con timeout
#include <semaphore.h>
#include <time.h>

int sem_wait_timeout(sem_t* sem, int timeout_ms) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000;
    
    if (ts.tv_nsec >= 1000000000) {
        ts.tv_sec++;
        ts.tv_nsec -= 1000000000;
    }
    
    return sem_timedwait(sem, &ts);
}

// Signal mask per bloccare segnali temporaneamente
void block_signals() {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
}

void unblock_signals() {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    pthread_sigmask(SIG_UNBLOCK, &set, NULL);
}
```

---

## 📋 Checklist Memoria Condivisa + Segnali

### ✅ **Memoria Condivisa**
- [ ] `shm_open()` per creare/accedere
- [ ] `ftruncate()` per impostare dimensione
- [ ] `mmap()` per mappare in memoria
- [ ] Mutex shared tra processi (`PTHREAD_PROCESS_SHARED`)
- [ ] `munmap()` e `shm_unlink()` per cleanup

### ✅ **Segnali**
- [ ] Handler async-safe (no printf/malloc)
- [ ] Thread dedicato per gestione segnali
- [ ] `sigaction()` per handler avanzati
- [ ] Variabili `volatile sig_atomic_t` per comunicazione
- [ ] `kill()` per inviare segnali tra processi

### ✅ **Sincronizzazione**
- [ ] Race condition prevenute con mutex
- [ ] Turn-based access per evitare starvation
- [ ] Timeout su operazioni bloccanti
- [ ] Gestione errori robusta

---

## 🎯 Compilazione e Test

```bash
# Compila
gcc -o shm_signals program.c -lpthread -lrt

# Test processo 1 (creatore)
./shm_signals 1

# Test processo 2 (in altro terminale)
./shm_signals 2

# Monitor memoria condivisa
ipcs -m

# Cleanup forzato se necessario
ipcrm -M $(ipcs -m | grep $(whoami) | awk '{print $2}')
```