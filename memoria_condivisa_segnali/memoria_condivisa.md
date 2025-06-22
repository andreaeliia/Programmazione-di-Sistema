# Memoria Condivisa, Segnali e Thread - Guida Completa

## 📋 Indice
1. [Concetti Fondamentali](#concetti-fondamentali)
2. [Processi vs Thread](#processi-vs-thread)
3. [Memoria Condivisa POSIX](#memoria-condivisa-posix)
4. [Segnali Unix](#segnali-unix)
5. [Thread e Gestione Segnali](#thread-e-gestione-segnali)
6. [Integrazione Completa](#integrazione-completa)
7. [Esempi Pratici](#esempi-pratici)
8. [Flussi di Lavoro](#flussi-di-lavoro)
9. [Troubleshooting](#troubleshooting)
10. [Preparazione Esame](#preparazione-esame)

---

## 🎯 Concetti Fondamentali

### Problema da Risolvere

**Scenario Tipico d'Esame:**
> "Due processi separati devono condividere dati e sincronizzarsi per evitare race condition"

### Componenti della Soluzione

```
Processo A                    Processo B
├── Thread Writer             ├── Thread Writer
├── Thread Signal Handler    ├── Thread Signal Handler
└── Accesso a memoria        └── Accesso a memoria
         ↓                           ↓
    MEMORIA CONDIVISA (IPC)
    [dati sincronizzati]
         ↑           ↑
    Segnale USR1  Segnale USR2
```

### Tecnologie Necessarie

| Componente | Scopo | API Principale |
|------------|-------|----------------|
| **fork()** | Creare processi separati | `pid = fork()` |
| **Memoria Condivisa** | Condividere dati tra processi | `shm_open()`, `mmap()` |
| **Segnali** | Comunicazione asincrona | `kill()`, `sigwait()` |
| **Thread** | Gestione separata segnali/lavoro | `pthread_create()` |

---

## 🔄 Processi vs Thread

### Differenze Fondamentali

**Thread (stesso processo):**
```
Processo
├── Thread 1  ┐
├── Thread 2  ├─ Condividono TUTTA la memoria
└── Thread 3  ┘
    ↓
[Memoria condivisa automaticamente]
```

**Processi (separati):**
```
Processo A     Processo B
├── Thread 1   ├── Thread 1
└── Thread 2   └── Thread 2
    ↓               ↓
Memoria A      Memoria B
(isolata)      (isolata)
    ↓               ↓
    └─── IPC ───────┘
    (serve meccanismo)
```

### Quando Usare Cosa

| Scenario | Soluzione | Perché |
|----------|-----------|--------|
| **Condivisione semplice** | Thread | Memoria già condivisa |
| **Isolamento sicurezza** | Processi | Crash di uno non affetta l'altro |
| **Distribuito** | Processi | Possono girare su macchine diverse |
| **Esercizio d'esame** | Processi | Più complesso = più punti! |

### Creazione Processi

```c
#include "apue.h"
#include <sys/wait.h>

int main(void) {
    pid_t pid;
    
    printf("Processo originale PID: %d\n", getpid());
    
    pid = fork();
    
    if (pid == 0) {
        // PROCESSO FIGLIO
        printf("Figlio PID: %d, padre: %d\n", getpid(), getppid());
        exit(0);
    } else if (pid > 0) {
        // PROCESSO PADRE
        printf("Padre PID: %d, figlio: %d\n", getpid(), pid);
        wait(NULL);  // Aspetta figlio
    } else {
        err_sys("fork failed");
    }
    
    return 0;
}
```

---

## 🗄️ Memoria Condivisa POSIX

### Concetto Base

**Problema:**
```c
// Processo A
int data = 42;

// Processo B (completamente separato)
printf("%d\n", data);  // ❌ ERRORE: data non esiste qui!
```

**Soluzione:**
```c
// Entrambi i processi
int *shared_data = mmap(...);  // Punta alla stessa memoria fisica
*shared_data = 42;            // Visibile a tutti
```

### API Memoria Condivisa

```c
#include <sys/mman.h>
#include <fcntl.h>

// 1. Crea/apri segmento
int shm_open(const char *name, int oflag, mode_t mode);

// 2. Imposta dimensione  
int ftruncate(int fd, off_t length);

// 3. Mappa in memoria
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);

// 4. Rilascia mapping
int munmap(void *addr, size_t length);

// 5. Rimuovi segmento
int shm_unlink(const char *name);
```

### Flusso Standard

```c
// === CREAZIONE ===
// 1. Crea segmento memoria condivisa
int shm_fd = shm_open("/my_memory", O_CREAT | O_RDWR, 0666);

// 2. Imposta dimensione
ftruncate(shm_fd, sizeof(my_struct_t));

// 3. Mappa in memoria
my_struct_t *shared_data = mmap(NULL, sizeof(my_struct_t), 
                               PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

// 4. Inizializza dati
shared_data->counter = 0;
shared_data->flag = 1;

// === UTILIZZO ===
// Leggi/scrivi normalmente
shared_data->counter++;
printf("Counter: %d\n", shared_data->counter);

// === PULIZIA ===
munmap(shared_data, sizeof(my_struct_t));
close(shm_fd);
shm_unlink("/my_memory");  // Solo l'ultimo processo
```

### Esempio Completo - Contatore Condiviso

```c
#include "apue.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/wait.h>

#define SHM_NAME "/shared_counter"

int main(void) {
    int shm_fd;
    int *shared_counter;
    pid_t pid;
    
    // Crea segmento memoria condivisa
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) err_sys("shm_open failed");
    
    // Imposta dimensione
    if (ftruncate(shm_fd, sizeof(int)) == -1) {
        err_sys("ftruncate failed");
    }
    
    // Mappa in memoria
    shared_counter = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE,
                         MAP_SHARED, shm_fd, 0);
    if (shared_counter == MAP_FAILED) err_sys("mmap failed");
    
    *shared_counter = 0;  // Inizializza
    
    pid = fork();
    
    if (pid == 0) {
        // PROCESSO FIGLIO
        for (int i = 0; i < 5; i++) {
            (*shared_counter)++;
            printf("Figlio: counter = %d\n", *shared_counter);
            sleep(1);
        }
        exit(0);
    } else if (pid > 0) {
        // PROCESSO PADRE
        for (int i = 0; i < 5; i++) {
            (*shared_counter) += 10;
            printf("Padre: counter = %d\n", *shared_counter);
            sleep(1);
        }
        
        wait(NULL);
        printf("Valore finale: %d\n", *shared_counter);
        
        // Pulizia
        munmap(shared_counter, sizeof(int));
        close(shm_fd);
        shm_unlink(SHM_NAME);
    }
    
    return 0;
}
```

### Race Condition Demo

```c
// PROBLEMA: Incremento non atomico
for (int i = 0; i < 100000; i++) {
    (*shared_counter)++;  // 3 operazioni separate!
    // 1. Leggi da memoria
    // 2. Incrementa in CPU  
    // 3. Scrivi in memoria
}

// RISULTATO: Valore finale < 200000 (perdite)
```

### Strutture Dati Condivise

```c
typedef struct {
    char buffer[1024];      // Buffer dati
    int write_index;        // Prossima posizione
    int process_a_active;   // Flag processo A
    int process_b_active;   // Flag processo B
    pid_t process_a_pid;    // PID processo A
    pid_t process_b_pid;    // PID processo B
} shared_data_t;

// Uso:
shared_data_t *shared = mmap(...);
shared->buffer[shared->write_index++] = 'A';
```

---

## 📡 Segnali Unix

### Cosa Sono i Segnali

**Definizione:** Notifiche asincrone tra processi

**Analogia:** Come ricevere un SMS - interrompe quello che stai facendo

### Segnali Comuni

| Segnale | Numero | Generato da | Scopo |
|---------|--------|-------------|-------|
| `SIGINT` | 2 | Ctrl+C | Interruzione utente |
| `SIGTERM` | 15 | `kill pid` | Terminazione gentile |
| `SIGKILL` | 9 | `kill -9 pid` | Terminazione forzata |
| `SIGUSR1` | 10 | Programma | Definito dall'utente |
| `SIGUSR2` | 12 | Programma | Definito dall'utente |

### API Segnali Base

```c
#include <signal.h>

// Invia segnale
int kill(pid_t pid, int sig);

// Gestore semplice (deprecato per threading)
signal(int signum, void (*handler)(int));

// Gestore avanzato
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
```

### Esempio Base - Comunicazione tra Processi

```c
#include "apue.h"
#include <signal.h>
#include <sys/wait.h>

void signal_handler(int sig) {
    printf("Figlio ricevuto segnale %d\n", sig);
}

int main(void) {
    pid_t pid = fork();
    
    if (pid == 0) {
        // PROCESSO FIGLIO
        signal(SIGUSR1, signal_handler);
        
        printf("Figlio in attesa segnali...\n");
        while (1) {
            pause();  // Aspetta segnali
        }
    } else {
        // PROCESSO PADRE
        sleep(2);  // Aspetta che figlio sia pronto
        
        printf("Padre invia SIGUSR1\n");
        kill(pid, SIGUSR1);
        
        sleep(2);
        kill(pid, SIGTERM);  // Termina figlio
        wait(NULL);
    }
    
    return 0;
}
```

---

## 🧵 Thread e Gestione Segnali

### Il Problema con Thread e Segnali

```c
// PROBLEMA: In un processo multi-thread, quale thread riceve il segnale?

Processo con 3 thread
├── Thread 1 ← Potrebbe ricevere
├── Thread 2 ← O questo
└── Thread 3 ← O questo
    ↓
Comportamento non deterministico!
```

### La Soluzione: Thread Dedicato

**Strategia:**
1. **Blocca segnali** in tutti i thread
2. **Un thread dedicato** aspetta segnali
3. **Altri thread** lavorano normalmente

### API Thread-Safe per Segnali

```c
#include <signal.h>
#include <pthread.h>

// Blocca/sblocca segnali per thread corrente
int pthread_sigmask(int how, const sigset_t *set, sigset_t *oldset);

// Aspetta segnale specifico (thread-safe)
int sigwait(const sigset_t *set, int *sig);

// Manipolazione sigset_t
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);  
int sigaddset(sigset_t *set, int signum);
int sigdelset(sigset_t *set, int signum);
```

### Template Thread Gestore Segnali

```c
#include "apue.h"
#include <signal.h>
#include <pthread.h>

// Set globale segnali
sigset_t signal_set;

void* signal_thread(void* arg) {
    int received_signal;
    
    printf("Thread segnali avviato\n");
    
    while (1) {
        // Aspetta segnali dal set
        if (sigwait(&signal_set, &received_signal) != 0) {
            err_sys("sigwait failed");
        }
        
        switch (received_signal) {
            case SIGUSR1:
                printf("Ricevuto SIGUSR1\n");
                // Gestisci logica specifica
                break;
            case SIGUSR2:
                printf("Ricevuto SIGUSR2\n");
                // Gestisci logica specifica
                break;
            case SIGTERM:
                printf("Terminazione richiesta\n");
                return NULL;
            default:
                printf("Segnale sconosciuto: %d\n", received_signal);
                break;
        }
    }
}

void* worker_thread(void* arg) {
    int id = *((int*)arg);
    
    // Questo thread non riceve segnali
    for (int i = 0; i < 10; i++) {
        printf("Worker %d: lavoro %d\n", id, i);
        sleep(1);
    }
    
    return NULL;
}

int main(void) {
    pthread_t signal_tid, worker_tid;
    int worker_id = 1;
    
    // Prepara set segnali
    sigemptyset(&signal_set);
    sigaddset(&signal_set, SIGUSR1);
    sigaddset(&signal_set, SIGUSR2);
    sigaddset(&signal_set, SIGTERM);
    
    // Blocca segnali per TUTTI i thread
    if (pthread_sigmask(SIG_BLOCK, &signal_set, NULL) != 0) {
        err_sys("pthread_sigmask failed");
    }
    
    printf("Processo PID: %d\n", getpid());
    
    // Crea thread
    pthread_create(&signal_tid, NULL, signal_thread, NULL);
    pthread_create(&worker_tid, NULL, worker_thread, &worker_id);
    
    // Aspetta worker
    pthread_join(worker_tid, NULL);
    
    // Termina signal thread
    kill(getpid(), SIGTERM);
    pthread_join(signal_tid, NULL);
    
    return 0;
}
```

---

## 🔗 Integrazione Completa

### Architettura del Sistema

```
Processo A                           Processo B
├── Main Thread                      ├── Main Thread
│   └── Inizializzazione             │   └── Inizializzazione
├── Signal Thread                    ├── Signal Thread  
│   ├── sigwait(USR1, USR2)         │   ├── sigwait(USR1, USR2)
│   └── Coordina sincronizzazione   │   └── Coordina sincronizzazione
└── Writer Thread                   └── Writer Thread
    ├── Genera dati casuali          │   ├── Genera dati casuali
    └── Scrive in memoria            │   └── Scrive in memoria
            ↓                                    ↓
    MEMORIA CONDIVISA POSIX
    ┌─────────────────────────┐
    │ Buffer: [A,B,C,D,...]   │
    │ Index: 4                │
    │ Process_A_PID: 1234     │
    │ Process_B_PID: 5678     │
    │ Flags sincronizzazione  │
    └─────────────────────────┘
```

### Protocollo di Sincronizzazione

```
1. Inizializzazione:
   - Processo A crea memoria condivisa
   - Entrambi registrano loro PID
   - A inizia con permesso di scrittura

2. Ciclo di lavoro:
   A: Scrive → Invia USR1 a B → Aspetta USR2
   B: Riceve USR1 → Scrive → Invia USR2 ad A → Aspetta USR1
   
3. Coordinazione:
   - Signal thread riceve segnali
   - Comunica con writer thread (flag/variabili)
   - Writer thread aspetta permesso prima di scrivere
```

### Struttura Dati Condivisa

```c
#define BUFFER_SIZE 100
#define SHM_NAME "/sync_buffer"

typedef struct {
    char buffer[BUFFER_SIZE];     // Buffer caratteri
    int write_index;              // Prossima posizione scrittura
    pid_t process_a_pid;          // PID processo A
    pid_t process_b_pid;          // PID processo B
    int process_a_can_write;      // Flag: A può scrivere
    int process_b_can_write;      // Flag: B può scrivere
    struct timespec last_write;   // Timestamp ultima scrittura
} shared_data_t;
```

---

## 💻 Esempi Pratici

### Esempio 1: Due Processi con Sincronizzazione

#### processo_a.c
```c
#include "apue.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

#define SHM_NAME "/char_sync"
#define BUFFER_SIZE 50

typedef struct {
    char buffer[BUFFER_SIZE];
    int write_index;
    pid_t process_a_pid;
    pid_t process_b_pid;
} shared_data_t;

shared_data_t *shared_data;
volatile int can_write = 1;  // A inizia con permesso

void* signal_thread(void* arg) {
    sigset_t set;
    int sig;
    
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    sigaddset(&set, SIGTERM);
    
    while (1) {
        sigwait(&set, &sig);
        
        switch (sig) {
            case SIGUSR2:
                printf("Processo A: ricevuto USR2, posso scrivere\n");
                can_write = 1;
                break;
            case SIGTERM:
                return NULL;
        }
    }
}

void* writer_thread(void* arg) {
    srand(time(NULL));
    
    for (int i = 0; i < 5; i++) {
        // Aspetta permesso
        while (!can_write) {
            usleep(10000);
        }
        
        if (shared_data->write_index >= BUFFER_SIZE) {
            printf("Buffer pieno\n");
            break;
        }
        
        // Genera carattere casuale
        char ch = 'A' + (rand() % 26);
        
        // Scrivi in memoria condivisa
        shared_data->buffer[shared_data->write_index] = ch;
        printf("Processo A: scritto '%c' in pos %d\n", 
               ch, shared_data->write_index);
        shared_data->write_index++;
        
        can_write = 0;  // Resetta permesso
        
        // Invia segnale a B
        if (shared_data->process_b_pid > 0) {
            kill(shared_data->process_b_pid, SIGUSR1);
        }
        
        // Attesa casuale
        sleep(rand() % 3 + 1);
    }
    
    return NULL;
}

int main(void) {
    pthread_t sig_tid, writer_tid;
    sigset_t set;
    int shm_fd;
    
    // Inizializza memoria condivisa
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(shared_data_t));
    shared_data = mmap(NULL, sizeof(shared_data_t), 
                      PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    
    // Inizializza se primo processo
    shared_data->write_index = 0;
    shared_data->process_a_pid = getpid();
    
    close(shm_fd);
    
    // Blocca segnali
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);  
    sigaddset(&set, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    
    printf("Processo A (PID: %d) avviato\n", getpid());
    
    // Crea thread
    pthread_create(&sig_tid, NULL, signal_thread, NULL);
    pthread_create(&writer_tid, NULL, writer_thread, NULL);
    
    // Aspetta writer
    pthread_join(writer_tid, NULL);
    
    // Termina
    pthread_cancel(sig_tid);
    
    // Mostra buffer
    printf("Buffer: ");
    for (int i = 0; i < shared_data->write_index; i++) {
        printf("%c", shared_data->buffer[i]);
    }
    printf("\n");
    
    munmap(shared_data, sizeof(shared_data_t));
    return 0;
}
```

#### processo_b.c
```c
// Simile a processo_a.c ma con modifiche:

volatile int can_write = 0;  // B aspetta segnale

// Nel signal_thread:
case SIGUSR1:
    printf("Processo B: ricevuto USR1, posso scrivere\n");
    can_write = 1;
    break;

// Nel writer_thread:
// Invia USR2 invece di USR1
kill(shared_data->process_a_pid, SIGUSR2);

// Nel main:
shared_data->process_b_pid = getpid();
```

### Esempio 2: Caratteri Casuali in Tempi Casuali

```c
void* writer_thread(void* arg) {
    srand(time(NULL) + getpid());  // Seed diverso per processo
    
    while (shared_data->write_index < BUFFER_SIZE) {
        // Aspetta permesso
        while (!can_write) {
            usleep(10000);
        }
        
        // Attesa casuale PRIMA di scrivere
        usleep((rand() % 2000000) + 500000);  // 0.5-2.5 secondi
        
        // Genera carattere casuale
        char ch = 'A' + (rand() % 26);
        
        // Scrivi con timestamp
        shared_data->buffer[shared_data->write_index] = ch;
        clock_gettime(CLOCK_REALTIME, &shared_data->last_write);
        
        printf("Processo %s: '%c' in pos %d (tempo: %ld)\n", 
               (getpid() == shared_data->process_a_pid) ? "A" : "B",
               ch, shared_data->write_index, 
               shared_data->last_write.tv_nsec);
        
        shared_data->write_index++;
        can_write = 0;
        
        // Invia segnale all'altro processo
        pid_t other_pid = (getpid() == shared_data->process_a_pid) ? 
                         shared_data->process_b_pid : shared_data->process_a_pid;
        int signal = (getpid() == shared_data->process_a_pid) ? SIGUSR1 : SIGUSR2;
        
        if (other_pid > 0) {
            kill(other_pid, signal);
        }
    }
    
    return NULL;
}
```

---

## 📊 Flussi di Lavoro

### Flusso 1: Setup Iniziale

```
1. Processo A avvia
   ├── Crea memoria condivisa
   ├── Inizializza struttura dati
   ├── Registra proprio PID
   ├── Blocca segnali per tutti thread
   ├── Crea signal_thread
   ├── Crea writer_thread
   └── A può scrivere (can_write = 1)

2. Processo B avvia  
   ├── Apre memoria condivisa esistente
   ├── Registra proprio PID
   ├── Blocca segnali per tutti thread
   ├── Crea signal_thread
   ├── Crea writer_thread
   └── B aspetta segnale (can_write = 0)
```

### Flusso 2: Ciclo di Sincronizzazione

```
Stato Iniziale:
├── A: can_write = 1
├── B: can_write = 0
└── Buffer: []

Iterazione 1:
├── A: Scrive 'K' → Buffer: [K]
├── A: Invia USR1 a B → A: can_write = 0
├── B: Riceve USR1 → B: can_write = 1
├── B: Scrive 'M' → Buffer: [K,M]
├── B: Invia USR2 ad A → B: can_write = 0
└── A: Riceve USR2 → A: can_write = 1

Iterazione 2:
├── A: Scrive 'Z' → Buffer: [K,M,Z]
├── A: Invia USR1 a B → A: can_write = 0
└── ... continua ...
```

### Flusso 3: Gestione Thread

```
Main Thread:
├── Inizializzazione memoria condivisa
├── pthread_sigmask(SIG_BLOCK) 
├── pthread_create(signal_thread)
├── pthread_create(writer_thread)
├── pthread_join(writer_thread)
└── Pulizia risorse

Signal Thread:
├── sigwait() loop infinito
├── Riceve segnale USR1/USR2
├── Imposta can_write = 1
└── Continua ad aspettare

Writer Thread:
├── while (!can_write) → Aspetta permesso
├── Genera carattere casuale
├── Scrive in memoria condivisa
├── can_write = 0
├── kill(other_pid, signal)
└── Ripete ciclo
```

### Flusso 4: Terminazione

```
1. Writer thread termina:
   ├── Buffer pieno O numero iterazioni finito
   └── return NULL

2. Main thread:
   ├── pthread_join(writer_thread) → Sblocca
   ├── pthread_cancel(signal_thread) → Termina signal thread
   └── Pulizia memoria

3. Sistema:
   ├── munmap() → Disconnette memoria
   ├── close(shm_fd) → Chiude file descriptor
   └── shm_unlink() → Rimuove segmento (ultimo processo)
```

---

## 🚨 Troubleshooting

### Errori Comuni

#### 1. Segmentation Fault in sigwait()

**Sintomi:**
```
Thread segnali avviato
Segmentation fault (core dumped)
```

**Cause:**
```c
// SBAGLIATO ❌
sigset_t set;  // Non inizializzato!
sigwait(&set, &sig);

// CORRETTO ✅  
sigset_t set;
sigemptyset(&set);
sigaddset(&set, SIGUSR1);
sigwait(&set, &sig);
```

#### 2. Thread non riceve segnali

**Sintomi:** Signal thread non stampa mai "ricevuto segnale"

**Cause:**
```c
// PROBLEMA: Segnali non bloccati nel main
// pthread_sigmask(SIG_BLOCK, &set, NULL);  // Manca!

// SOLUZIONE: Blocca prima di creare thread
pthread_sigmask(SIG_BLOCK, &set, NULL);
pthread_create(&signal_tid, NULL, signal_thread, NULL);
```

#### 3. "No such file or directory" (shm_open)

**Sintomi:**
```
shm_open failed: No such file or directory
```

**Soluzioni:**
```bash
# Controlla memoria condivisa esistente
ls /dev/shm/

# Rimuovi memoria rimasta
rm /dev/shm/char_sync

# Aggiungi flag -lrt alla compilazione
gcc ... -lrt
```

#### 4. Race Condition in memoria condivisa

**Sintomi:** Buffer corrotto, caratteri mancanti

**Problema:**
```c
// Due processi scrivono contemporaneamente
shared_data->buffer[shared_data->write_index] = 'A';  // Processo A
shared_data->write_index++;                           // Processo A
shared_data->buffer[shared_data->write_index] = 'B';  // Processo B ← SOVRASCRIVE!
```

**Soluzione:** Sincronizzazione corretta con segnali

#### 5. Process zombie

**Sintomi:**
```bash
ps aux | grep <defunct>
```

**Soluzione:**
```c
// Nel processo padre
wait(NULL);  // Aspetta figlio

// O gestione asincrona
signal(SIGCHLD, SIG_IGN);  // Auto-cleanup
```

### Debug Tools

#### 1. Controllo Memoria Condivisa

```bash
# Lista segmenti memoria condivisa
ls -la /dev/shm/

# Informazioni dettagliate
ipcs -m

# Rimuovi memoria orfana
ipcrm -M key
# oppure
rm /dev/shm/nome_segmento
```

#### 2. Debug Segnali

```bash
# Monitoraggio segnali con strace
strace -e signal ./processo_a

# Test manuale segnali
kill -USR1 PID
kill -USR2 PID
```

#### 3. Debug Thread

```c
// Aggiungi print di debug
printf("[DEBUG] Thread %ld: can_write = %d\n", pthread_self(), can_write);
printf("[DEBUG] PID %d invia %s a PID %d\n", getpid(), 
       (sig == SIGUSR1) ? "USR1" : "USR2", target_pid);
```

#### 4. Valgrind per Memory Leaks

```bash
# Controlla memory leaks
valgrind --leak-check=full ./processo_a

# Controlla thread
valgrind --tool=helgrind ./processo_a
```

---

## 📚 Preparazione Esame

### Checklist Concetti

- [ ] **fork()** e differenza processi/thread
- [ ] **shm_open(), mmap(), munmap(), shm_unlink()**
- [ ] **kill(), sigwait(), pthread_sigmask()**
- [ ] **pthread_create(), pthread_join()**
- [ ] **Sincronizzazione con segnali USR1/USR2**
- [ ] **Gestione race condition**
- [ ] **Pulizia risorse**

### Template di Risposta Esame

```c
#include "apue.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

#define SHM_NAME "/exam_shared"
#define BUFFER_SIZE 100

typedef struct {
    char buffer[BUFFER_SIZE];
    int write_index;
    pid_t process_a_pid;
    pid_t process_b_pid;
    // Altri campi per sincronizzazione
} shared_data_t;

shared_data_t *shared_data;
volatile int can_write = 0;  // Inizializza appropriatamente

void* signal_thread(void* arg) {
    sigset_t set;
    int sig;
    
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    sigaddset(&set, SIGTERM);
    
    while (1) {
        sigwait(&set, &sig);
        
        switch (sig) {
            case SIGUSR1:
                // Gestisci logica segnale 1
                break;
            case SIGUSR2:
                // Gestisci logica segnale 2  
                break;
            case SIGTERM:
                return NULL;
        }
    }
}

void* writer_thread(void* arg) {
    srand(time(NULL) + getpid());
    
    while (/* condizione loop */) {
        // Aspetta permesso
        while (!can_write) {
            usleep(10000);
        }
        
        // Genera dato casuale
        char data = /* generazione casuale */;
        
        // Scrivi in memoria condivisa
        shared_data->buffer[shared_data->write_index] = data;
        shared_data->write_index++;
        
        // Reset permesso
        can_write = 0;
        
        // Invia segnale altro processo
        kill(/* altro_pid */, /* segnale */);
        
        // Attesa casuale
        usleep((rand() % 2000000) + 500000);
    }
    
    return NULL;
}

int main(void) {
    pthread_t signal_tid, writer_tid;
    sigset_t set;
    int shm_fd;
    
    // === MEMORIA CONDIVISA ===
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(shared_data_t));
    shared_data = mmap(NULL, sizeof(shared_data_t), 
                      PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    close(shm_fd);
    
    // === INIZIALIZZAZIONE ===
    // Inizializza struttura condivisa
    // Registra PID processo
    
    // === SEGNALI ===
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);
    sigaddset(&set, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
    
    // === THREAD ===
    pthread_create(&signal_tid, NULL, signal_thread, NULL);
    pthread_create(&writer_tid, NULL, writer_thread, NULL);
    
    // === ATTESA E PULIZIA ===
    pthread_join(writer_tid, NULL);
    pthread_cancel(signal_tid);
    
    munmap(shared_data, sizeof(shared_data_t));
    // shm_unlink() solo se ultimo processo
    
    return 0;
}
```

### Domande Tipiche Esame

1. **"Spiegare differenza tra thread e processi"**
   - Thread: memoria condivisa, stesso address space
   - Processi: memoria isolata, serve IPC

2. **"Come funziona mmap() con MAP_SHARED?"**
   - Mappa memoria fisica in address space processo
   - MAP_SHARED: modifiche visibili a tutti i processi
   - MAP_PRIVATE: copia privata per ogni processo

3. **"Perché usare sigwait() invece di signal()?"**
   - sigwait() è thread-safe
   - signal() ha comportamento indefinito con thread
   - sigwait() permette gestione centralizzata

4. **"Come evitare race condition?"**
   - Sincronizzazione con segnali
   - Un solo processo scrive alla volta
   - Protocollo di handshake con USR1/USR2

### Errori da Evitare all'Esame

1. **Non bloccare segnali nel main thread**
2. **Usare signal() invece di sigwait()**
3. **Dimenticare ftruncate() dopo shm_open()**
4. **Non gestire pulizia risorse**
5. **Race condition senza sincronizzazione**
6. **Parametri sbagliati a pthread_create()**

---

## 🎯 Riassunto Finale

### Flusso Completo

1. **Setup**: Memoria condivisa + blocco segnali
2. **Thread**: Signal handler + worker separati  
3. **Sincronizzazione**: Alternanza con USR1/USR2
4. **Dati**: Caratteri casuali in tempi casuali
5. **Pulizia**: munmap + shm_unlink + pthread cleanup

### Concetti Chiave

- **IPC**: Comunicazione tra processi tramite memoria condivisa
- **Sincronizzazione**: Segnali per coordinamento 
- **Thread**: Separazione responsabilità (segnali vs lavoro)
- **Race Condition**: Problema risolto con protocollo

### Per l'Esame

- **Memorizza template** di codice base
- **Pratica debugging** con valgrind/gdb
- **Testa sempre** la sincronizzazione
- **Non dimenticare** la gestione risorse

---

*Questa guida copre tutto il necessario per padroneggiare memoria condivisa, segnali e thread. Pratica con gli esempi e sarai pronto per qualsiasi variante d'esame!* 🚀