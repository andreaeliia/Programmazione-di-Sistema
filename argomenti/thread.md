# Thread POSIX - Guida Completa per Esame

## Indice
1. [Teoria Fondamentale](#teoria-fondamentale)
2. [Funzioni Base pthread](#funzioni-base-pthread)
3. [Esempi Commentati](#esempi-commentati)
4. [Sincronizzazione](#sincronizzazione)
5. [Template per Esame](#template-per-esame)
6. [Casi d'Uso Comuni](#casi-duso-comuni)
7. [Best Practices](#best-practices)

---

## Teoria Fondamentale

### Cosa sono i Thread

I **thread** sono unità di esecuzione leggere che condividono lo stesso spazio di memoria all'interno di un processo. A differenza dei processi, i thread condividono:

- **Memoria virtuale** (heap, data segment)
- **File descriptor** aperti
- **Variabili globali** e statiche
- **Segnali** e handler

Ma mantengono separati:
- **Stack** individuale
- **Registri CPU** e program counter
- **Thread ID** univoco

### Vantaggi dei Thread

1. **Performance**: Creazione più veloce dei processi
2. **Condivisione memoria**: Comunicazione diretta senza IPC
3. **Parallelismo**: Sfruttamento CPU multi-core
4. **Responsiveness**: UI che rimane attiva durante calcoli

### Svantaggi dei Thread

1. **Complessità**: Gestione sincronizzazione
2. **Race conditions**: Accesso concorrente a dati condivisi
3. **Debugging difficile**: Comportamento non-deterministico
4. **Overhead**: Context switching e sincronizzazione

### Modelli di Threading

**1:1 (Kernel-level)**: Ogni thread utente corrisponde a un thread kernel
**N:1 (User-level)**: Più thread utente su un thread kernel
**N:M (Hybrid)**: N thread utente su M thread kernel

POSIX pthread implementa tipicamente il modello **1:1**.

---

## Funzioni Base pthread

### Include e Linking

```c
#include <pthread.h>
// Compilazione: gcc -o programma programma.c -lpthread
```

### Gestione Thread

#### pthread_create()
```c
int pthread_create(pthread_t *thread,           // ID thread creato
                   const pthread_attr_t *attr, // Attributi (NULL = default)
                   void *(*start_routine)(void*), // Funzione da eseguire
                   void *arg);                  // Argomento alla funzione
```

#### pthread_join()
```c
int pthread_join(pthread_t thread,    // Thread da aspettare
                 void **retval);      // Valore di ritorno (può essere NULL)
```

#### pthread_exit()
```c
void pthread_exit(void *retval);      // Termina thread corrente
```

#### pthread_self()
```c
pthread_t pthread_self(void);         // Restituisce ID thread corrente
```

#### pthread_detach()
```c
int pthread_detach(pthread_t thread); // Marca thread come detached
```

### Sincronizzazione Base

#### Mutex
```c
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // Inizializzazione statica

int pthread_mutex_init(pthread_mutex_t *mutex, 
                       const pthread_mutexattr_t *attr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
```

#### Condition Variables
```c
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

int pthread_cond_init(pthread_cond_t *cond, 
                      const pthread_condattr_t *attr);
int pthread_cond_destroy(pthread_cond_t *cond);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);
```

---

## Esempi Commentati

### Esempio 1: Thread Base

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// Funzione che sarà eseguita dal thread
// Deve avere signature: void* function_name(void* arg)
void* thread_function(void* arg) {
    // Cast dell'argomento ricevuto a intero
    int thread_id = *(int*)arg;
    
    // Stampa informazioni thread
    printf("Thread %d: avviato\n", thread_id);
    
    // Simula lavoro con sleep
    sleep(2);
    
    // Stampa completamento
    printf("Thread %d: completato\n", thread_id);
    
    // Termina thread restituendo valore
    return NULL;
}

int main() {
    // Dichiara ID del thread (opaco, gestito dalla libreria)
    pthread_t thread_id;
    
    // Variabile da passare al thread
    int arg = 42;
    
    printf("Main: creazione thread\n");
    
    // Crea thread
    // &thread_id: puntatore dove salvare ID del nuovo thread
    // NULL: attributi default
    // thread_function: funzione da eseguire
    // &arg: argomento da passare alla funzione
    if (pthread_create(&thread_id, NULL, thread_function, &arg) != 0) {
        perror("pthread_create fallita");
        exit(EXIT_FAILURE);
    }
    
    printf("Main: thread creato, aspetto terminazione\n");
    
    // Aspetta che il thread termini
    // thread_id: ID del thread da aspettare
    // NULL: non ci interessa il valore di ritorno
    if (pthread_join(thread_id, NULL) != 0) {
        perror("pthread_join fallita");
        exit(EXIT_FAILURE);
    }
    
    printf("Main: thread terminato, programma finito\n");
    return 0;
}
```

### Esempio 2: Multiple Thread con Array

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS 3

// Struttura per passare dati ai thread
// Necessaria perché possiamo passare solo un puntatore void*
typedef struct {
    int thread_id;    // ID univoco del thread
    int start_value;  // Valore iniziale per calcoli
    int result;       // Risultato del calcolo
} ThreadData;

// Funzione eseguita da ogni thread
void* worker_function(void* arg) {
    // Cast dell'argomento alla struttura corretta
    ThreadData* data = (ThreadData*)arg;
    
    printf("Worker %d: iniziato con valore %d\n", 
           data->thread_id, data->start_value);
    
    // Simula calcolo (quadrato del valore)
    data->result = data->start_value * data->start_value;
    
    // Simula tempo di elaborazione
    sleep(data->thread_id);  // Thread diversi dormono tempi diversi
    
    printf("Worker %d: risultato = %d\n", 
           data->thread_id, data->result);
    
    // Il thread termina, data rimane valida perché è nello stack del main
    return NULL;
}

int main() {
    // Array di ID thread - uno per ogni thread che creeremo
    pthread_t threads[NUM_THREADS];
    
    // Array di dati - una struttura per ogni thread
    ThreadData thread_data[NUM_THREADS];
    
    printf("Main: creazione di %d threads\n", NUM_THREADS);
    
    // Crea tutti i thread
    for (int i = 0; i < NUM_THREADS; i++) {
        // Inizializza dati per questo thread
        thread_data[i].thread_id = i + 1;        // ID da 1 a NUM_THREADS
        thread_data[i].start_value = (i + 1) * 10; // Valori 10, 20, 30
        thread_data[i].result = 0;               // Inizializza risultato
        
        // Crea il thread passando puntatore alla sua struttura dati
        if (pthread_create(&threads[i], NULL, worker_function, &thread_data[i]) != 0) {
            perror("Errore creazione thread");
            exit(EXIT_FAILURE);
        }
    }
    
    printf("Main: tutti i thread creati, aspetto terminazione\n");
    
    // Aspetta che tutti i thread terminino
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Errore join thread");
            exit(EXIT_FAILURE);
        }
    }
    
    // Stampa tutti i risultati
    printf("\nMain: risultati finali:\n");
    for (int i = 0; i < NUM_THREADS; i++) {
        printf("Thread %d: %d^2 = %d\n", 
               thread_data[i].thread_id,
               thread_data[i].start_value,
               thread_data[i].result);
    }
    
    return 0;
}
```

### Esempio 3: Mutex per Protezione Dati

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 5
#define INCREMENTS_PER_THREAD 100000

// Variabile condivisa tra tutti i thread
// PROBLEMA: accesso concorrente non protetto causa race conditions
int shared_counter = 0;

// Mutex per proteggere accessi alla variabile condivisa
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

// Funzione eseguita da ogni thread
void* increment_counter(void* arg) {
    int thread_id = *(int*)arg;
    
    printf("Thread %d: iniziato\n", thread_id);
    
    // Ogni thread fa molti incrementi
    for (int i = 0; i < INCREMENTS_PER_THREAD; i++) {
        // SEZIONE CRITICA: accesso alla variabile condivisa
        
        // Acquisisce il mutex (operazione atomica)
        // Se un altro thread ha già il mutex, questo thread si blocca qui
        pthread_mutex_lock(&counter_mutex);
        
        // Solo un thread alla volta può essere qui
        shared_counter++;  // Operazione protetta
        
        // Rilascia il mutex permettendo ad altri thread di entrare
        pthread_mutex_unlock(&counter_mutex);
        
        // FINE SEZIONE CRITICA
    }
    
    printf("Thread %d: completato %d incrementi\n", thread_id, INCREMENTS_PER_THREAD);
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    
    printf("Iniziale shared_counter = %d\n", shared_counter);
    printf("Ogni thread farà %d incrementi\n", INCREMENTS_PER_THREAD);
    printf("Valore finale atteso = %d\n", NUM_THREADS * INCREMENTS_PER_THREAD);
    
    // Crea tutti i thread
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i + 1;
        if (pthread_create(&threads[i], NULL, increment_counter, &thread_ids[i]) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }
    
    // Aspetta tutti i thread
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("Finale shared_counter = %d\n", shared_counter);
    
    // Verifica correttezza
    int expected = NUM_THREADS * INCREMENTS_PER_THREAD;
    if (shared_counter == expected) {
        printf("SUCCESSO: valore corretto!\n");
    } else {
        printf("ERRORE: valore atteso %d, ottenuto %d\n", expected, shared_counter);
    }
    
    // Distrugge il mutex (buona pratica)
    pthread_mutex_destroy(&counter_mutex);
    
    return 0;
}
```

### Esempio 4: Producer-Consumer con Condition Variables

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 5
#define ITEMS_TO_PRODUCE 10

// Buffer circolare condiviso
typedef struct {
    int buffer[BUFFER_SIZE];    // Array per i dati
    int in;                     // Indice inserimento (producer)
    int out;                    // Indice estrazione (consumer)
    int count;                  // Numero elementi presenti
} CircularBuffer;

// Variabili globali condivise
CircularBuffer shared_buffer;
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;

// Condition variables per sincronizzazione
pthread_cond_t buffer_not_full = PTHREAD_COND_INITIALIZER;   // Producer aspetta
pthread_cond_t buffer_not_empty = PTHREAD_COND_INITIALIZER;  // Consumer aspetta

// Funzione producer: produce elementi e li inserisce nel buffer
void* producer(void* arg) {
    int producer_id = *(int*)arg;
    
    for (int i = 1; i <= ITEMS_TO_PRODUCE; i++) {
        // Acquisisce mutex per accesso sicuro al buffer
        pthread_mutex_lock(&buffer_mutex);
        
        // Attende finché buffer non è pieno
        // IMPORTANTE: while (non if) per gestire spurious wakeup
        while (shared_buffer.count == BUFFER_SIZE) {
            printf("Producer %d: buffer pieno, aspetto\n", producer_id);
            // pthread_cond_wait rilascia automaticamente il mutex
            // e lo ri-acquisisce quando si risveglia
            pthread_cond_wait(&buffer_not_full, &buffer_mutex);
        }
        
        // Inserisce elemento nel buffer
        shared_buffer.buffer[shared_buffer.in] = i;
        printf("Producer %d: inserito %d in posizione %d\n", 
               producer_id, i, shared_buffer.in);
        
        // Aggiorna indici (circolare)
        shared_buffer.in = (shared_buffer.in + 1) % BUFFER_SIZE;
        shared_buffer.count++;
        
        // Segnala ai consumer che c'è un nuovo elemento
        pthread_cond_signal(&buffer_not_empty);
        
        // Rilascia mutex
        pthread_mutex_unlock(&buffer_mutex);
        
        // Simula tempo di produzione
        usleep(100000);  // 100ms
    }
    
    printf("Producer %d: terminato\n", producer_id);
    return NULL;
}

// Funzione consumer: estrae elementi dal buffer
void* consumer(void* arg) {
    int consumer_id = *(int*)arg;
    int items_consumed = 0;
    
    while (items_consumed < ITEMS_TO_PRODUCE) {
        // Acquisisce mutex
        pthread_mutex_lock(&buffer_mutex);
        
        // Attende finché buffer non è vuoto
        while (shared_buffer.count == 0) {
            printf("Consumer %d: buffer vuoto, aspetto\n", consumer_id);
            pthread_cond_wait(&buffer_not_empty, &buffer_mutex);
        }
        
        // Estrae elemento
        int item = shared_buffer.buffer[shared_buffer.out];
        printf("Consumer %d: estratto %d da posizione %d\n", 
               consumer_id, item, shared_buffer.out);
        
        // Aggiorna indici
        shared_buffer.out = (shared_buffer.out + 1) % BUFFER_SIZE;
        shared_buffer.count--;
        items_consumed++;
        
        // Segnala ai producer che c'è spazio libero
        pthread_cond_signal(&buffer_not_full);
        
        // Rilascia mutex
        pthread_mutex_unlock(&buffer_mutex);
        
        // Simula tempo di consumo
        usleep(150000);  // 150ms
    }
    
    printf("Consumer %d: terminato\n", consumer_id);
    return NULL;
}

int main() {
    pthread_t producer_thread, consumer_thread;
    int producer_id = 1, consumer_id = 1;
    
    // Inizializza buffer
    shared_buffer.in = 0;
    shared_buffer.out = 0;
    shared_buffer.count = 0;
    
    printf("Avvio Producer-Consumer con buffer di %d elementi\n", BUFFER_SIZE);
    
    // Crea thread producer e consumer
    pthread_create(&producer_thread, NULL, producer, &producer_id);
    pthread_create(&consumer_thread, NULL, consumer, &consumer_id);
    
    // Aspetta entrambi i thread
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);
    
    printf("Programma terminato\n");
    
    // Cleanup
    pthread_mutex_destroy(&buffer_mutex);
    pthread_cond_destroy(&buffer_not_full);
    pthread_cond_destroy(&buffer_not_empty);
    
    return 0;
}
```

---

## Sincronizzazione

### Mutex (Mutual Exclusion)

#### Quando Usare i Mutex
- Protezione variabili condivise
- Sezioni critiche
- Accesso esclusivo a risorse

#### Pattern di Utilizzo
```c
pthread_mutex_t mutex;

// Inizializzazione
pthread_mutex_init(&mutex, NULL);

// Uso tipico
pthread_mutex_lock(&mutex);
// SEZIONE CRITICA - solo un thread alla volta
pthread_mutex_unlock(&mutex);

// Cleanup
pthread_mutex_destroy(&mutex);
```

#### Tipi di Mutex
```c
// Normal mutex (default)
pthread_mutexattr_t attr;
pthread_mutexattr_init(&attr);
pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);

// Recursive mutex (stesso thread può ri-acquisire)
pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

// Error checking mutex (controlla ownership)
pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
```

### Condition Variables

#### Quando Usare le Condition Variables
- Aspettare che una condizione diventi vera
- Coordinamento tra thread
- Implementazione di semafori

#### Pattern Producer-Consumer
```c
pthread_mutex_t mutex;
pthread_cond_t condition;
int shared_data;
int data_ready = 0;

// Thread producer
pthread_mutex_lock(&mutex);
shared_data = new_value;
data_ready = 1;
pthread_cond_signal(&condition);  // Sveglia un thread in attesa
pthread_mutex_unlock(&mutex);

// Thread consumer
pthread_mutex_lock(&mutex);
while (!data_ready) {  // SEMPRE while, mai if
    pthread_cond_wait(&condition, &mutex);
}
process(shared_data);
data_ready = 0;
pthread_mutex_unlock(&mutex);
```

#### Differenza signal vs broadcast
```c
pthread_cond_signal(&cond);    // Sveglia UN thread
pthread_cond_broadcast(&cond); // Sveglia TUTTI i thread in attesa
```

---

## Template per Esame

### Template Base Multi-Thread

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 3
#define WORK_DURATION 5

// Struttura dati condivisa
typedef struct {
    int shared_counter;          // Dati condivisi tra thread
    volatile int keep_running;   // Flag di controllo
    pthread_mutex_t mutex;       // Protezione accessi
} SharedData;

// Struttura argomenti per thread
typedef struct {
    SharedData* data;           // Puntatore ai dati condivisi
    int thread_id;             // ID univoco del thread
} ThreadArgs;

// Variabile globale
SharedData global_data;

// Funzione eseguita dai thread worker
void* worker_thread(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    SharedData* data = args->data;
    int id = args->thread_id;
    
    printf("Thread %d: avviato\n", id);
    
    while (data->keep_running) {
        // Lavoro del thread
        
        // Sezione critica
        pthread_mutex_lock(&data->mutex);
        data->shared_counter++;
        printf("Thread %d: counter = %d\n", id, data->shared_counter);
        pthread_mutex_unlock(&data->mutex);
        
        // Simula lavoro
        usleep(100000);  // 100ms
    }
    
    printf("Thread %d: terminato\n", id);
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    ThreadArgs thread_args[NUM_THREADS];
    
    // Inizializza dati condivisi
    global_data.shared_counter = 0;
    global_data.keep_running = 1;
    pthread_mutex_init(&global_data.mutex, NULL);
    
    // Crea thread
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_args[i].data = &global_data;
        thread_args[i].thread_id = i + 1;
        
        if (pthread_create(&threads[i], NULL, worker_thread, &thread_args[i]) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }
    
    // Lavora per un tempo determinato
    printf("Main: esecuzione per %d secondi\n", WORK_DURATION);
    sleep(WORK_DURATION);
    
    // Ferma i thread
    global_data.keep_running = 0;
    
    // Aspetta terminazione
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Risultati finali
    printf("Risultato finale: counter = %d\n", global_data.shared_counter);
    
    // Cleanup
    pthread_mutex_destroy(&global_data.mutex);
    
    return 0;
}
```

### Template Producer-Consumer

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 10
#define NUM_ITEMS 20

typedef struct {
    int buffer[BUFFER_SIZE];
    int in, out, count;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
} ProducerConsumerData;

ProducerConsumerData pc_data;

void* producer(void* arg) {
    int producer_id = *(int*)arg;
    
    for (int i = 1; i <= NUM_ITEMS; i++) {
        pthread_mutex_lock(&pc_data.mutex);
        
        while (pc_data.count == BUFFER_SIZE) {
            pthread_cond_wait(&pc_data.not_full, &pc_data.mutex);
        }
        
        pc_data.buffer[pc_data.in] = i;
        pc_data.in = (pc_data.in + 1) % BUFFER_SIZE;
        pc_data.count++;
        
        printf("Producer %d: prodotto item %d\n", producer_id, i);
        
        pthread_cond_signal(&pc_data.not_empty);
        pthread_mutex_unlock(&pc_data.mutex);
        
        usleep(100000);
    }
    
    return NULL;
}

void* consumer(void* arg) {
    int consumer_id = *(int*)arg;
    int consumed = 0;
    
    while (consumed < NUM_ITEMS) {
        pthread_mutex_lock(&pc_data.mutex);
        
        while (pc_data.count == 0) {
            pthread_cond_wait(&pc_data.not_empty, &pc_data.mutex);
        }
        
        int item = pc_data.buffer[pc_data.out];
        pc_data.out = (pc_data.out + 1) % BUFFER_SIZE;
        pc_data.count--;
        consumed++;
        
        printf("Consumer %d: consumato item %d\n", consumer_id, item);
        
        pthread_cond_signal(&pc_data.not_full);
        pthread_mutex_unlock(&pc_data.mutex);
        
        usleep(150000);
    }
    
    return NULL;
}

int main() {
    pthread_t prod_thread, cons_thread;
    int prod_id = 1, cons_id = 1;
    
    // Inizializza
    pc_data.in = pc_data.out = pc_data.count = 0;
    pthread_mutex_init(&pc_data.mutex, NULL);
    pthread_cond_init(&pc_data.not_full, NULL);
    pthread_cond_init(&pc_data.not_empty, NULL);
    
    // Crea thread
    pthread_create(&prod_thread, NULL, producer, &prod_id);
    pthread_create(&cons_thread, NULL, consumer, &cons_id);
    
    // Aspetta
    pthread_join(prod_thread, NULL);
    pthread_join(cons_thread, NULL);
    
    // Cleanup
    pthread_mutex_destroy(&pc_data.mutex);
    pthread_cond_destroy(&pc_data.not_full);
    pthread_cond_destroy(&pc_data.not_empty);
    
    return 0;
}
```

---

## Casi d'Uso Comuni

### 1. Calcolo Parallelo

**Scenario**: Dividere un calcolo pesante tra più thread

```c
// Esempio: somma di array divisa tra thread
void* sum_array_portion(void* arg) {
    struct {
        int* array;
        int start;
        int end;
        long long* result;
    }* data = arg;
    
    long long sum = 0;
    for (int i = data->start; i < data->end; i++) {
        sum += data->array[i];
    }
    
    *(data->result) = sum;
    return NULL;
}
```

### 2. Thread Pool Pattern

**Scenario**: Pool fisso di thread che processano task da una coda

```c
typedef struct {
    void (*function)(void*);
    void* arg;
} Task;

typedef struct {
    Task* tasks;
    int front, rear, count;
    int size;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    int shutdown;
} ThreadPool;
```

### 3. Reader-Writer Lock Pattern

**Scenario**: Molti lettori, pochi scrittori

```c
typedef struct {
    int readers;
    pthread_mutex_t mutex;
    pthread_mutex_t write_lock;
} RWLock;

void reader_lock(RWLock* rw) {
    pthread_mutex_lock(&rw->mutex);
    rw->readers++;
    if (rw->readers == 1) {
        pthread_mutex_lock(&rw->write_lock);
    }
    pthread_mutex_unlock(&rw->mutex);
}

void reader_unlock(RWLock* rw) {
    pthread_mutex_lock(&rw->mutex);
    rw->readers--;
    if (rw->readers == 0) {
        pthread_mutex_unlock(&rw->write_lock);
    }
    pthread_mutex_unlock(&rw->mutex);
}
```

---

## Best Practices

### Per l'Esame

1. **Sempre controllare return values**
```c
if (pthread_create(&thread, NULL, function, arg) != 0) {
    perror("pthread_create");
    exit(EXIT_FAILURE);
}
```

2. **Inizializzare sempre le variabili**
```c
SharedData data = {0};  // Inizializza tutto a zero
pthread_mutex_init(&data.mutex, NULL);
```

3. **Usare volatile per flag di controllo**
```c
volatile int keep_running = 1;  // Previene ottimizzazioni compilatore
```

4. **Pattern while per condition variables**
```c
while (!condition) {  // MAI usare if
    pthread_cond_wait(&cond, &mutex);
}
```

5. **Cleanup sempre**
```c
pthread_mutex_destroy(&mutex);
pthread_cond_destroy(&cond);
```

### Debugging Thread

```c
// Stampe debug utili
printf("Thread %ld: entering critical section\n", pthread_self());
printf("Thread %ld: counter = %d\n", pthread_self(), shared_counter);
```

### Evitare Deadlock

1. **Ordinamento acquisizione lock**
2. **Timeout su lock**
3. **Evitare lock annidati**

### Checklist Esame

- [ ] Include pthread.h
- [ ] Compilazione con -lpthread
- [ ] Gestione errori pthread_create
- [ ] pthread_join per tutti i thread
- [ ] Mutex per dati condivisi
- [ ] Condition variables se necessario
- [ ] Volatile per flag di controllo
- [ ] Cleanup di mutex e condition variables
- [ ] Inizializzazione corretta strutture dati