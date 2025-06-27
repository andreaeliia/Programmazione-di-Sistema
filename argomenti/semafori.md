# Semafori - Guida Completa per Esame

## Indice
1. [Concetti Teorici](#concetti-teorici)
2. [Semafori POSIX Named](#semafori-posix-named)
3. [Semafori POSIX Unnamed](#semafori-posix-unnamed)
4. [Semafori System V](#semafori-system-v)
5. [Casi d'Uso Comuni](#casi-duso-comuni)
6. [Esempi Completi](#esempi-completi)
7. [Gestione Errori](#gestione-errori)
8. [Best Practices](#best-practices)

---

## Concetti Teorici

### Che cos'è un Semaforo
Un semaforo è un meccanismo di sincronizzazione che controlla l'accesso a risorse condivise attraverso un contatore intero non-negativo. Le operazioni principali sono:
- **P() / wait()** - Decrementa il contatore (attende se è 0)
- **V() / post()** - Incrementa il contatore

### Tipi di Semafori
1. **Binari (Mutex)** - Valore 0 o 1, per mutua esclusione
2. **Contatori** - Valore > 1, per limitare accesso a N risorse
3. **Barriere** - Per sincronizzazione di più processi

### Differenze tra API

| Caratteristica | POSIX Named | POSIX Unnamed | System V |
|----------------|-------------|---------------|----------|
| Visibilità | Filesystem | Memoria condivisa | System-wide |
| Persistenza | Oltre processo | Solo processo | Oltre processo |
| Naming | Nome stringa | Puntatore | ID numerico |
| Portabilità | Alta | Alta | Media |

---

## Semafori POSIX Named

### Funzioni Principali

```c
#include <semaphore.h>
#include <fcntl.h>

// Crea/apre semaforo named
sem_t *sem_open(const char *name, int oflag, mode_t mode, unsigned int value);

// Rimuove semaforo named
int sem_unlink(const char *name);

// Operazioni su semaforo
int sem_wait(sem_t *sem);       // P() - Decrementa (attende se 0)
int sem_trywait(sem_t *sem);    // P() non-bloccante
int sem_post(sem_t *sem);       // V() - Incrementa
int sem_getvalue(sem_t *sem, int *sval);  // Legge valore

// Chiude handle del semaforo
int sem_close(sem_t *sem);
```

### Esempio Base Semaforo Named

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/wait.h>

#define SEM_NAME "/example_semaphore"

int main() {
    sem_t *semaphore;
    pid_t pid;
    
    // Rimuove semaforo esistente (se presente)
    sem_unlink(SEM_NAME);
    
    // Crea semaforo con valore iniziale 1 (mutex)
    semaphore = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0644, 1);
    if (semaphore == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    
    printf("Semaforo creato con successo\n");
    
    // Fork per creare processo figlio
    pid = fork();
    
    if (pid == -1) {
        perror("fork");
        sem_close(semaphore);
        sem_unlink(SEM_NAME);
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        // Processo figlio
        printf("Figlio: tentativo di acquisire semaforo\n");
        
        if (sem_wait(semaphore) == -1) {
            perror("sem_wait");
            exit(EXIT_FAILURE);
        }
        
        printf("Figlio: semaforo acquisito, sezione critica\n");
        sleep(3); // Simula lavoro in sezione critica
        printf("Figlio: fine sezione critica\n");
        
        if (sem_post(semaphore) == -1) {
            perror("sem_post");
            exit(EXIT_FAILURE);
        }
        
        printf("Figlio: semaforo rilasciato\n");
        sem_close(semaphore);
        exit(0);
    }
    else {
        // Processo padre
        sleep(1); // Lascia al figlio il tempo di acquisire
        
        printf("Padre: tentativo di acquisire semaforo\n");
        
        if (sem_wait(semaphore) == -1) {
            perror("sem_wait");
            exit(EXIT_FAILURE);
        }
        
        printf("Padre: semaforo acquisito, sezione critica\n");
        sleep(2); // Simula lavoro in sezione critica
        printf("Padre: fine sezione critica\n");
        
        if (sem_post(semaphore) == -1) {
            perror("sem_post");
            exit(EXIT_FAILURE);
        }
        
        printf("Padre: semaforo rilasciato\n");
        
        // Aspetta terminazione figlio
        wait(NULL);
        
        // Cleanup
        sem_close(semaphore);
        sem_unlink(SEM_NAME);
    }
    
    return 0;
}
```

### Esempio Multi-Processo con Named Semaphore

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <string.h>

#define SEM_NAME "/resource_semaphore"
#define MAX_RESOURCES 3

// Funzione per processo worker
void worker_process(int worker_id) {
    sem_t *resource_sem;
    
    // Apre semaforo esistente
    resource_sem = sem_open(SEM_NAME, 0);
    if (resource_sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    
    printf("Worker %d: avviato\n", worker_id);
    
    // Acquisisce risorsa
    printf("Worker %d: richiede risorsa...\n", worker_id);
    if (sem_wait(resource_sem) == -1) {
        perror("sem_wait");
        exit(EXIT_FAILURE);
    }
    
    printf("Worker %d: risorsa acquisita\n", worker_id);
    
    // Simula utilizzo risorsa
    sleep(2 + worker_id);
    
    printf("Worker %d: rilascia risorsa\n", worker_id);
    
    // Rilascia risorsa
    if (sem_post(resource_sem) == -1) {
        perror("sem_post");
        exit(EXIT_FAILURE);
    }
    
    sem_close(resource_sem);
    printf("Worker %d: terminato\n", worker_id);
}

int main() {
    sem_t *resource_sem;
    pid_t pids[5];
    
    // Rimuove semaforo esistente
    sem_unlink(SEM_NAME);
    
    // Crea semaforo per MAX_RESOURCES risorse
    resource_sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0644, MAX_RESOURCES);
    if (resource_sem == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    
    printf("Semaforo creato con %d risorse disponibili\n", MAX_RESOURCES);
    
    // Crea 5 processi worker
    for (int i = 0; i < 5; i++) {
        pids[i] = fork();
        
        if (pids[i] == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pids[i] == 0) {
            // Processo figlio
            worker_process(i + 1);
            exit(0);
        }
    }
    
    // Processo padre aspetta tutti i figli
    for (int i = 0; i < 5; i++) {
        wait(NULL);
    }
    
    printf("Tutti i worker terminati\n");
    
    // Cleanup
    sem_close(resource_sem);
    sem_unlink(SEM_NAME);
    
    return 0;
}
```

---

## Semafori POSIX Unnamed

### Funzioni Principali

```c
#include <semaphore.h>

// Inizializza semaforo unnamed
int sem_init(sem_t *sem, int pshared, unsigned int value);

// Distrugge semaforo unnamed
int sem_destroy(sem_t *sem);

// Operazioni (identiche ai named)
int sem_wait(sem_t *sem);
int sem_trywait(sem_t *sem);
int sem_post(sem_t *sem);
int sem_getvalue(sem_t *sem, int *sval);
```

### Esempio Semaforo Unnamed in Memoria Condivisa

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define SHARED_SIZE sizeof(SharedData)

typedef struct {
    sem_t mutex;        // Semaforo per mutua esclusione
    sem_t empty;        // Spazi vuoti nel buffer
    sem_t full;         // Elementi pieni nel buffer
    int buffer[5];      // Buffer circolare
    int in;             // Indice inserimento
    int out;            // Indice estrazione
    int count;          // Contatore elementi
} SharedData;

// Produttore
void producer(SharedData* data) {
    printf("Producer: avviato\n");
    
    for (int i = 1; i <= 10; i++) {
        // Attende spazio vuoto
        sem_wait(&data->empty);
        
        // Entra in sezione critica
        sem_wait(&data->mutex);
        
        // Inserisce elemento
        data->buffer[data->in] = i;
        printf("Producer: inserito %d in posizione %d\n", i, data->in);
        data->in = (data->in + 1) % 5;
        data->count++;
        
        // Esce da sezione critica
        sem_post(&data->mutex);
        
        // Segnala elemento pieno
        sem_post(&data->full);
        
        usleep(100000); // 100ms
    }
    
    printf("Producer: terminato\n");
}

// Consumatore
void consumer(SharedData* data) {
    printf("Consumer: avviato\n");
    
    for (int i = 1; i <= 10; i++) {
        // Attende elemento pieno
        sem_wait(&data->full);
        
        // Entra in sezione critica
        sem_wait(&data->mutex);
        
        // Estrae elemento
        int item = data->buffer[data->out];
        printf("Consumer: estratto %d da posizione %d\n", item, data->out);
        data->out = (data->out + 1) % 5;
        data->count--;
        
        // Esce da sezione critica
        sem_post(&data->mutex);
        
        // Segnala spazio vuoto
        sem_post(&data->empty);
        
        usleep(150000); // 150ms
    }
    
    printf("Consumer: terminato\n");
}

int main() {
    SharedData* shared_data;
    pid_t pid;
    
    // Crea memoria condivisa
    shared_data = mmap(NULL, SHARED_SIZE,
                      PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shared_data == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    
    // Inizializza semafori unnamed
    // mutex: valore 1 (mutua esclusione)
    if (sem_init(&shared_data->mutex, 1, 1) == -1) {
        perror("sem_init mutex");
        exit(EXIT_FAILURE);
    }
    
    // empty: valore 5 (5 spazi vuoti iniziali)
    if (sem_init(&shared_data->empty, 1, 5) == -1) {
        perror("sem_init empty");
        exit(EXIT_FAILURE);
    }
    
    // full: valore 0 (0 elementi pieni iniziali)
    if (sem_init(&shared_data->full, 1, 0) == -1) {
        perror("sem_init full");
        exit(EXIT_FAILURE);
    }
    
    // Inizializza buffer
    shared_data->in = 0;
    shared_data->out = 0;
    shared_data->count = 0;
    
    printf("Buffer circolare inizializzato\n");
    
    // Fork
    pid = fork();
    
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        // Processo figlio - Consumer
        consumer(shared_data);
    }
    else {
        // Processo padre - Producer
        producer(shared_data);
        
        // Aspetta terminazione consumer
        wait(NULL);
        
        // Distrugge semafori
        sem_destroy(&shared_data->mutex);
        sem_destroy(&shared_data->empty);
        sem_destroy(&shared_data->full);
    }
    
    // Cleanup memoria condivisa
    munmap(shared_data, SHARED_SIZE);
    return 0;
}
```

---

## Semafori System V

### Funzioni Principali

```c
#include <sys/ipc.h>
#include <sys/sem.h>

// Crea/ottiene set di semafori
int semget(key_t key, int nsems, int semflg);

// Operazioni sui semafori
int semop(int semid, struct sembuf *sops, size_t nsops);

// Controllo semafori
int semctl(int semid, int semnum, int cmd, ...);

// Struttura per operazioni
struct sembuf {
    unsigned short sem_num;  // Numero semaforo nel set
    short sem_op;           // Operazione (-1=wait, +1=post, 0=test)
    short sem_flg;          // Flag (IPC_NOWAIT, SEM_UNDO)
};
```

### Esempio System V Semaphores

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <errno.h>

#define PATHNAME "/tmp"
#define PROJ_ID 42

// Union per semctl (richiesta da alcuni sistemi)
union semun {
    int val;                    // Valore per SETVAL
    struct semid_ds *buf;       // Buffer per IPC_STAT, IPC_SET
    unsigned short *array;      // Array per GETALL, SETALL
};

// Funzioni wrapper per operations
int sem_p(int semid, int sem_num) {
    struct sembuf sop;
    sop.sem_num = sem_num;
    sop.sem_op = -1;        // P() operation
    sop.sem_flg = SEM_UNDO; // Undo automatico alla terminazione
    
    return semop(semid, &sop, 1);
}

int sem_v(int semid, int sem_num) {
    struct sembuf sop;
    sop.sem_num = sem_num;
    sop.sem_op = 1;         // V() operation
    sop.sem_flg = SEM_UNDO; // Undo automatico alla terminazione
    
    return semop(semid, &sop, 1);
}

int sem_set_value(int semid, int sem_num, int value) {
    union semun arg;
    arg.val = value;
    return semctl(semid, sem_num, SETVAL, arg);
}

int sem_get_value(int semid, int sem_num) {
    return semctl(semid, sem_num, GETVAL);
}

int main() {
    key_t key;
    int semid;
    pid_t pid;
    
    // Genera chiave univoca
    key = ftok(PATHNAME, PROJ_ID);
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    
    // Crea set di 2 semafori
    semid = semget(key, 2, IPC_CREAT | 0644);
    if (semid == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    
    // Inizializza semafori
    // Semaforo 0: mutex (valore 1)
    if (sem_set_value(semid, 0, 1) == -1) {
        perror("sem_set_value mutex");
        exit(EXIT_FAILURE);
    }
    
    // Semaforo 1: counter (valore 3)
    if (sem_set_value(semid, 1, 3) == -1) {
        perror("sem_set_value counter");
        exit(EXIT_FAILURE);
    }
    
    printf("Set di semafori creato e inizializzato\n");
    printf("Mutex: %d, Counter: %d\n", 
           sem_get_value(semid, 0), sem_get_value(semid, 1));
    
    // Fork per creare processi
    for (int i = 0; i < 3; i++) {
        pid = fork();
        
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0) {
            // Processo figlio
            printf("Figlio %d: richiede risorsa (counter)...\n", i+1);
            
            // Acquisisce risorsa contata
            if (sem_p(semid, 1) == -1) {
                perror("sem_p counter");
                exit(EXIT_FAILURE);
            }
            
            printf("Figlio %d: risorsa acquisita\n", i+1);
            
            // Acquisisce mutex per sezione critica
            if (sem_p(semid, 0) == -1) {
                perror("sem_p mutex");
                exit(EXIT_FAILURE);
            }
            
            printf("Figlio %d: in sezione critica\n", i+1);
            printf("Figlio %d: valori semafori - Mutex: %d, Counter: %d\n", 
                   i+1, sem_get_value(semid, 0), sem_get_value(semid, 1));
            
            sleep(2); // Simula lavoro
            
            printf("Figlio %d: esce da sezione critica\n", i+1);
            
            // Rilascia mutex
            if (sem_v(semid, 0) == -1) {
                perror("sem_v mutex");
                exit(EXIT_FAILURE);
            }
            
            // Rilascia risorsa contata
            if (sem_v(semid, 1) == -1) {
                perror("sem_v counter");
                exit(EXIT_FAILURE);
            }
            
            printf("Figlio %d: risorsa rilasciata\n", i+1);
            exit(0);
        }
    }
    
    // Processo padre aspetta tutti i figli
    for (int i = 0; i < 3; i++) {
        wait(NULL);
    }
    
    printf("Tutti i processi terminati\n");
    printf("Valori finali - Mutex: %d, Counter: %d\n", 
           sem_get_value(semid, 0), sem_get_value(semid, 1));
    
    // Rimuove set di semafori
    if (semctl(semid, 0, IPC_RMID) == -1) {
        perror("semctl IPC_RMID");
        exit(EXIT_FAILURE);
    }
    
    printf("Set di semafori rimosso\n");
    return 0;
}
```

---

## Casi d'Uso Comuni

### 1. Mutex (Mutua Esclusione)

```c
// Template per sezione critica
sem_t *mutex;

void critical_section_template() {
    // Acquisisce mutex
    if (sem_wait(mutex) == -1) {
        perror("sem_wait");
        return;
    }
    
    // *** SEZIONE CRITICA ***
    // Accesso esclusivo alla risorsa condivisa
    printf("Processo %d in sezione critica\n", getpid());
    
    // *** FINE SEZIONE CRITICA ***
    
    // Rilascia mutex
    if (sem_post(mutex) == -1) {
        perror("sem_post");
        return;
    }
}
```

### 2. Producer-Consumer

```c
// Semafori per producer-consumer
sem_t *mutex;    // Mutua esclusione buffer
sem_t *empty;    // Spazi vuoti disponibili
sem_t *full;     // Elementi pieni disponibili

void producer_template(int item) {
    sem_wait(empty);    // Attende spazio vuoto
    sem_wait(mutex);    // Entra in sezione critica
    
    // Inserisce item nel buffer
    add_to_buffer(item);
    
    sem_post(mutex);    // Esce da sezione critica
    sem_post(full);     // Segnala elemento aggiunto
}

void consumer_template() {
    sem_wait(full);     // Attende elemento pieno
    sem_wait(mutex);    // Entra in sezione critica
    
    // Rimuove item dal buffer
    int item = remove_from_buffer();
    
    sem_post(mutex);    // Esce da sezione critica
    sem_post(empty);    // Segnala spazio liberato
    
    return item;
}
```

### 3. Readers-Writers

```c
// Semafori per readers-writers
sem_t *mutex;           // Mutua esclusione contatori
sem_t *write_mutex;     // Mutua esclusione writers
int reader_count = 0;   // Numero di readers attivi

void reader_template() {
    sem_wait(mutex);
    reader_count++;
    if (reader_count == 1) {
        sem_wait(write_mutex); // Primo reader blocca writers
    }
    sem_post(mutex);
    
    // *** LETTURA ***
    printf("Reader %d sta leggendo\n", getpid());
    
    sem_wait(mutex);
    reader_count--;
    if (reader_count == 0) {
        sem_post(write_mutex); // Ultimo reader sblocca writers
    }
    sem_post(mutex);
}

void writer_template() {
    sem_wait(write_mutex);
    
    // *** SCRITTURA ***
    printf("Writer %d sta scrivendo\n", getpid());
    
    sem_post(write_mutex);
}
```

---

## Esempi Completi

### Esempio Completo: Dining Philosophers

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define NUM_PHILOSOPHERS 5
#define SHARED_SIZE sizeof(DiningTable)

typedef struct {
    sem_t forks[NUM_PHILOSOPHERS];  // Un semaforo per ogni forchetta
    sem_t mutex;                    // Mutex per sezione critica
} DiningTable;

void philosopher(DiningTable* table, int id) {
    int left_fork = id;
    int right_fork = (id + 1) % NUM_PHILOSOPHERS;
    
    printf("Filosofo %d: inizia a pensare\n", id);
    
    for (int meal = 0; meal < 3; meal++) {
        // Pensa
        printf("Filosofo %d: pensa (pasto %d)\n", id, meal + 1);
        sleep(1 + rand() % 3);
        
        // Ha fame
        printf("Filosofo %d: ha fame\n", id);
        
        // Acquisisce forchette (ordine per evitare deadlock)
        if (id % 2 == 0) {
            // Filosofi pari: prima sinistra, poi destra
            sem_wait(&table->forks[left_fork]);
            printf("Filosofo %d: prende forchetta sinistra %d\n", id, left_fork);
            
            sem_wait(&table->forks[right_fork]);
            printf("Filosofo %d: prende forchetta destra %d\n", id, right_fork);
        } else {
            // Filosofi dispari: prima destra, poi sinistra
            sem_wait(&table->forks[right_fork]);
            printf("Filosofo %d: prende forchetta destra %d\n", id, right_fork);
            
            sem_wait(&table->forks[left_fork]);
            printf("Filosofo %d: prende forchetta sinistra %d\n", id, left_fork);
        }
        
        // Mangia
        printf("Filosofo %d: MANGIA (pasto %d)\n", id, meal + 1);
        sleep(1 + rand() % 2);
        
        // Rilascia forchette
        sem_post(&table->forks[left_fork]);
        printf("Filosofo %d: rilascia forchetta sinistra %d\n", id, left_fork);
        
        sem_post(&table->forks[right_fork]);
        printf("Filosofo %d: rilascia forchetta destra %d\n", id, right_fork);
    }
    
    printf("Filosofo %d: terminato\n", id);
}

int main() {
    DiningTable* table;
    pid_t pids[NUM_PHILOSOPHERS];
    
    // Crea memoria condivisa
    table = mmap(NULL, SHARED_SIZE,
                PROT_READ | PROT_WRITE,
                MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (table == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    
    // Inizializza semafori
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        if (sem_init(&table->forks[i], 1, 1) == -1) {
            perror("sem_init fork");
            exit(EXIT_FAILURE);
        }
    }
    
    if (sem_init(&table->mutex, 1, 1) == -1) {
        perror("sem_init mutex");
        exit(EXIT_FAILURE);
    }
    
    printf("Tavolo dei filosofi inizializzato\n");
    
    // Crea processi filosofi
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pids[i] = fork();
        
        if (pids[i] == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pids[i] == 0) {
            // Processo figlio - Filosofo
            srand(getpid()); // Seed diverso per ogni processo
            philosopher(table, i);
            exit(0);
        }
    }
    
    // Processo padre aspetta tutti i filosofi
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        wait(NULL);
    }
    
    printf("Tutti i filosofi hanno terminato\n");
    
    // Distrugge semafori
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        sem_destroy(&table->forks[i]);
    }
    sem_destroy(&table->mutex);
    
    // Cleanup memoria condivisa
    munmap(table, SHARED_SIZE);
    return 0;
}
```

### Esempio Completo: Barbiere che Dorme

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define MAX_CUSTOMERS 5
#define NUM_CUSTOMERS 10
#define SHARED_SIZE sizeof(BarberShop)

typedef struct {
    sem_t customers;        // Numero di clienti in attesa
    sem_t barber;          // Barbiere disponibile
    sem_t mutex;           // Mutua esclusione per waiting_customers
    int waiting_customers; // Numero di clienti in attesa
} BarberShop;

void barber_process(BarberShop* shop) {
    printf("Barbiere: apre il negozio\n");
    
    while (1) {
        printf("Barbiere: dorme in attesa di clienti\n");
        
        // Aspetta che arrivi un cliente
        sem_wait(&shop->customers);
        
        // Acquisisce mutex per leggere waiting_customers
        sem_wait(&shop->mutex);
        shop->waiting_customers--;
        printf("Barbiere: si sveglia, clienti in attesa: %d\n", shop->waiting_customers);
        sem_post(&shop->mutex);
        
        // Segnala che è disponibile
        sem_post(&shop->barber);
        
        // Taglia i capelli
        printf("Barbiere: taglia i capelli\n");
        sleep(2); // Tempo per tagliare
        printf("Barbiere: taglio completato\n");
    }
}

void customer_process(BarberShop* shop, int customer_id) {
    printf("Cliente %d: arriva al negozio\n", customer_id);
    
    // Acquisisce mutex per controllare posti disponibili
    sem_wait(&shop->mutex);
    
    if (shop->waiting_customers < MAX_CUSTOMERS) {
        // C'è posto, si siede
        shop->waiting_customers++;
        printf("Cliente %d: si siede (clienti in attesa: %d)\n", 
               customer_id, shop->waiting_customers);
        sem_post(&shop->mutex);
        
        // Sveglia il barbiere
        sem_post(&shop->customers);
        
        // Aspetta che il barbiere sia disponibile
        sem_wait(&shop->barber);
        
        printf("Cliente %d: viene servito\n", customer_id);
        // Il taglio avviene nel processo barbiere
        
        printf("Cliente %d: taglio terminato, se ne va\n", customer_id);
    } else {
        // Non c'è posto, se ne va
        printf("Cliente %d: negozio pieno, se ne va\n", customer_id);
        sem_post(&shop->mutex);
    }
}

int main() {
    BarberShop* shop;
    pid_t barber_pid;
    pid_t customer_pids[NUM_CUSTOMERS];
    
    // Crea memoria condivisa
    shop = mmap(NULL, SHARED_SIZE,
               PROT_READ | PROT_WRITE,
               MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shop == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    
    // Inizializza semafori
    if (sem_init(&shop->customers, 1, 0) == -1) {
        perror("sem_init customers");
        exit(EXIT_FAILURE);
    }
    
    if (sem_init(&shop->barber, 1, 0) == -1) {
        perror("sem_init barber");
        exit(EXIT_FAILURE);
    }
    
    if (sem_init(&shop->mutex, 1, 1) == -1) {
        perror("sem_init mutex");
        exit(EXIT_FAILURE);
    }
    
    shop->waiting_customers = 0;
    
    printf("Negozio del barbiere inizializzato (max %d clienti)\n", MAX_CUSTOMERS);
    
    // Crea processo barbiere
    barber_pid = fork();
    if (barber_pid == -1) {
        perror("fork barber");
        exit(EXIT_FAILURE);
    }
    else if (barber_pid == 0) {
        barber_process(shop);
        exit(0);
    }
    
    // Crea processi clienti
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        customer_pids[i] = fork();
        
        if (customer_pids[i] == -1) {
            perror("fork customer");
            exit(EXIT_FAILURE);
        }
        else if (customer_pids[i] == 0) {
            customer_process(shop, i + 1);
            exit(0);
        }
        
        // Intervallo tra arrivi clienti
        sleep(1);
    }
    
    // Aspetta che tutti i clienti terminino
    for (int i = 0; i < NUM_CUSTOMERS; i++) {
        wait(NULL);
    }
    
    printf("Tutti i clienti hanno terminato\n");
    
    // Termina il barbiere
    kill(barber_pid, SIGTERM);
    wait(NULL);
    
    // Distrugge semafori
    sem_destroy(&shop->customers);
    sem_destroy(&shop->barber);
    sem_destroy(&shop->mutex);
    
    // Cleanup memoria condivisa
    munmap(shop, SHARED_SIZE);
    return 0;
}
```

---

## Gestione Errori

### Template per Gestione Robusta

```c
#include <errno.h>
#include <string.h>

// Wrapper per sem_wait con gestione interruzioni
int safe_sem_wait(sem_t *sem) {
    while (sem_wait(sem) == -1) {
        if (errno == EINTR) {
            // Interruzione da segnale, riprova
            continue;
        } else {
            // Errore reale
            perror("sem_wait");
            return -1;
        }
    }
    return 0;
}

// Wrapper per sem_trywait non-bloccante
int safe_sem_trywait(sem_t *sem) {
    if (sem_trywait(sem) == -1) {
        if (errno == EAGAIN) {
            // Semaforo non disponibile (normale)
            return 0;
        } else {
            // Errore reale
            perror("sem_trywait");
            return -1;
        }
    }
    return 1; // Successo
}

// Controllo valore semaforo
void check_semaphore_value(sem_t *sem, const char* name) {
    int value;
    if (sem_getvalue(sem, &value) == -1) {
        perror("sem_getvalue");
    } else {
        printf("Semaforo %s: valore = %d\n", name, value);
    }
}
```

---

## Best Practices

### Regole Fondamentali

1. **Sempre fare cleanup** - sem_close(), sem_unlink(), sem_destroy()
2. **Gestire interruzioni** - EINTR in sem_wait()
3. **Usare SEM_UNDO** - Per System V semafori
4. **Evitare deadlock** - Ordinamento acquisizione risorse
5. **Timeout quando necessario** - sem_timedwait()

### Checklist per l'Esame

**Semafori POSIX Named:**
- [ ] sem_open() con flags corretti
- [ ] sem_unlink() prima della creazione
- [ ] sem_close() in tutti i processi
- [ ] sem_unlink() per rimuovere

**Semafori POSIX Unnamed:**
- [ ] sem_init() con pshared=1 per IPC
- [ ] Memoria condivisa per il semaforo
- [ ] sem_destroy() per cleanup

**Semafori System V:**
- [ ] ftok() per generare chiave
- [ ] semget() per creare set
- [ ] semctl(SETVAL) per inizializzare
- [ ] SEM_UNDO in sembuf
- [ ] semctl(IPC_RMID) per rimuovere

**Pattern Comuni:**
- [ ] Mutex: valore iniziale 1
- [ ] Producer-Consumer: empty=N, full=0, mutex=1
- [ ] Readers-Writers: mutex=1, write_mutex=1
- [ ] Barriera: counter + mutex + barrier

**Gestione Errori:**
- [ ] Controllo return value di tutte le funzioni
- [ ] Gestione EINTR in sem_wait()
- [ ] Cleanup in caso di errore
- [ ] Handler per segnali   