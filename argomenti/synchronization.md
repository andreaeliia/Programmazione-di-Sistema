# Synchronization Avanzata - Riferimento Completo

## 🎯 Concetti Fondamentali

### **Problemi di Concorrenza**
- **Race Condition**: risultato dipende dal timing di esecuzione
- **Deadlock**: processi si bloccano a vicenda
- **Starvation**: processo non riesce mai ad accedere alla risorsa
- **Livelock**: processi cambiano stato ma non progrediscono

### **Primitive di Sincronizzazione**
- **Mutex**: Mutual Exclusion (accesso esclusivo)
- **Semaforo**: contatore per risorse multiple
- **Condition Variable**: attesa condizioni specifiche
- **Barrier**: sincronizzazione punti di controllo
- **Spinlock**: attesa attiva (busy waiting)

### **Tipi di Sincronizzazione**
- **Thread-level**: pthread mutex, condition variables
- **Process-level**: POSIX semafori, memoria condivisa
- **Atomic operations**: operazioni indivisibili
- **Lock-free**: algoritmi senza lock

---

## 🔒 Mutex - Mutual Exclusion

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>

// Struttura dati condivisa
typedef struct {
    int counter;                    // Contatore condiviso
    pthread_mutex_t mutex;          // Mutex per protezione
    long operations_count;          // Numero operazioni totali
    time_t start_time;             // Timestamp inizio
} SharedCounter;

// Struttura per thread worker
typedef struct {
    int thread_id;                 // ID identificativo thread
    SharedCounter* shared;         // Riferimento dati condivisi
    int iterations;                // Numero iterazioni da fare
    int increment_value;           // Valore da aggiungere
    long local_operations;         // Contatore operazioni locali
} ThreadData;

// Funzione thread che incrementa counter
void* increment_thread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    
    printf("🟢 Thread %d avviato: %d iterazioni, incremento %d\n", 
           data->thread_id, data->iterations, data->increment_value);
    
    for (int i = 0; i < data->iterations; i++) {
        // SEZIONE CRITICA CON MUTEX
        pthread_mutex_lock(&data->shared->mutex);
        
        // Operazioni critiche
        int old_value = data->shared->counter;
        usleep(1);  // Simula elaborazione (aumenta probabilità race condition)
        data->shared->counter = old_value + data->increment_value;
        data->shared->operations_count++;
        
        pthread_mutex_unlock(&data->shared->mutex);
        
        data->local_operations++;
        
        // Lavoro non critico
        usleep(10);  // Simula altro lavoro
    }
    
    printf("🔴 Thread %d completato: %ld operazioni locali\n", 
           data->thread_id, data->local_operations);
    
    return NULL;
}

// Test race condition senza mutex
void* increment_thread_unsafe(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    
    printf("⚠️  Thread unsafe %d avviato\n", data->thread_id);
    
    for (int i = 0; i < data->iterations; i++) {
        // SEZIONE CRITICA SENZA PROTEZIONE (PERICOLOSO!)
        int old_value = data->shared->counter;
        usleep(1);  // Simula elaborazione
        data->shared->counter = old_value + data->increment_value;
        data->shared->operations_count++;
        
        data->local_operations++;
        usleep(10);
    }
    
    printf("⚠️  Thread unsafe %d completato\n", data->thread_id);
    return NULL;
}

// Esempio mutex base
void basic_mutex_example() {
    printf("=== ESEMPIO MUTEX BASE ===\n");
    
    const int num_threads = 4;
    const int iterations_per_thread = 1000;
    
    // Inizializza dati condivisi
    SharedCounter shared;
    shared.counter = 0;
    shared.operations_count = 0;
    shared.start_time = time(NULL);
    
    if (pthread_mutex_init(&shared.mutex, NULL) != 0) {
        perror("pthread_mutex_init");
        return;
    }
    
    // Crea thread
    pthread_t threads[num_threads];
    ThreadData thread_data[num_threads];
    
    printf("🚀 Avvio %d thread con %d iterazioni ciascuno\n", 
           num_threads, iterations_per_thread);
    
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].shared = &shared;
        thread_data[i].iterations = iterations_per_thread;
        thread_data[i].increment_value = 1;
        thread_data[i].local_operations = 0;
        
        int result = pthread_create(&threads[i], NULL, increment_thread, &thread_data[i]);
        if (result != 0) {
            printf("❌ Errore creazione thread %d: %s\n", i, strerror(result));
        }
    }
    
    // Aspetta completamento
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Risultati
    time_t elapsed = time(NULL) - shared.start_time;
    int expected_value = num_threads * iterations_per_thread;
    
    printf("\n📊 Risultati con MUTEX:\n");
    printf("   Valore finale counter: %d\n", shared.counter);
    printf("   Valore atteso: %d\n", expected_value);
    printf("   Operazioni totali: %ld\n", shared.operations_count);
    printf("   Tempo elapsed: %ld secondi\n", elapsed);
    printf("   Risultato: %s\n", (shared.counter == expected_value) ? "✅ CORRETTO" : "❌ RACE CONDITION");
    
    pthread_mutex_destroy(&shared.mutex);
}

// Confronto con e senza mutex
void race_condition_demo() {
    printf("\n=== DEMO RACE CONDITION ===\n");
    
    const int num_threads = 3;
    const int iterations = 100;
    
    // Test 1: SENZA MUTEX (race condition)
    printf("🔥 Test 1: SENZA protezione mutex\n");
    
    SharedCounter shared_unsafe;
    shared_unsafe.counter = 0;
    shared_unsafe.operations_count = 0;
    
    pthread_t threads_unsafe[num_threads];
    ThreadData data_unsafe[num_threads];
    
    for (int i = 0; i < num_threads; i++) {
        data_unsafe[i].thread_id = i;
        data_unsafe[i].shared = &shared_unsafe;
        data_unsafe[i].iterations = iterations;
        data_unsafe[i].increment_value = 1;
        data_unsafe[i].local_operations = 0;
        
        pthread_create(&threads_unsafe[i], NULL, increment_thread_unsafe, &data_unsafe[i]);
    }
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads_unsafe[i], NULL);
    }
    
    int expected = num_threads * iterations;
    printf("   Risultato unsafe: %d (atteso: %d) - %s\n", 
           shared_unsafe.counter, expected,
           (shared_unsafe.counter == expected) ? "✅ Fortunato!" : "❌ Race condition!");
    
    // Test 2: CON MUTEX (sicuro)
    printf("\n🛡️  Test 2: CON protezione mutex\n");
    
    SharedCounter shared_safe;
    shared_safe.counter = 0;
    shared_safe.operations_count = 0;
    pthread_mutex_init(&shared_safe.mutex, NULL);
    
    pthread_t threads_safe[num_threads];
    ThreadData data_safe[num_threads];
    
    for (int i = 0; i < num_threads; i++) {
        data_safe[i].thread_id = i;
        data_safe[i].shared = &shared_safe;
        data_safe[i].iterations = iterations;
        data_safe[i].increment_value = 1;
        data_safe[i].local_operations = 0;
        
        pthread_create(&threads_safe[i], NULL, increment_thread, &data_safe[i]);
    }
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads_safe[i], NULL);
    }
    
    printf("   Risultato safe: %d (atteso: %d) - %s\n", 
           shared_safe.counter, expected,
           (shared_safe.counter == expected) ? "✅ Sicuro!" : "❌ Errore imprevisto!");
    
    pthread_mutex_destroy(&shared_safe.mutex);
}

// Mutex con timeout
int mutex_lock_timeout(pthread_mutex_t* mutex, int timeout_ms) {
    struct timespec abs_timeout;
    clock_gettime(CLOCK_REALTIME, &abs_timeout);
    
    abs_timeout.tv_sec += timeout_ms / 1000;
    abs_timeout.tv_nsec += (timeout_ms % 1000) * 1000000;
    
    if (abs_timeout.tv_nsec >= 1000000000) {
        abs_timeout.tv_sec++;
        abs_timeout.tv_nsec -= 1000000000;
    }
    
    return pthread_mutex_timedlock(mutex, &abs_timeout);
}

// Esempio deadlock prevention
typedef struct {
    pthread_mutex_t mutex1;
    pthread_mutex_t mutex2;
    int resource1;
    int resource2;
} TwoResources;

void* thread_order1(void* arg) {
    TwoResources* res = (TwoResources*)arg;
    
    for (int i = 0; i < 5; i++) {
        printf("🔵 Thread 1: lock mutex1\n");
        pthread_mutex_lock(&res->mutex1);
        
        sleep(1);  // Simula lavoro
        
        printf("🔵 Thread 1: lock mutex2\n");
        pthread_mutex_lock(&res->mutex2);
        
        // Lavoro con entrambe risorse
        res->resource1++;
        res->resource2++;
        printf("🔵 Thread 1: r1=%d, r2=%d\n", res->resource1, res->resource2);
        
        pthread_mutex_unlock(&res->mutex2);
        pthread_mutex_unlock(&res->mutex1);
        
        sleep(1);
    }
    return NULL;
}

void* thread_order2(void* arg) {
    TwoResources* res = (TwoResources*)arg;
    
    for (int i = 0; i < 5; i++) {
        printf("🔴 Thread 2: lock mutex1 (stesso ordine)\n");
        pthread_mutex_lock(&res->mutex1);  // Stesso ordine = no deadlock
        
        sleep(1);
        
        printf("🔴 Thread 2: lock mutex2\n");
        pthread_mutex_lock(&res->mutex2);
        
        res->resource1 += 10;
        res->resource2 += 10;
        printf("🔴 Thread 2: r1=%d, r2=%d\n", res->resource1, res->resource2);
        
        pthread_mutex_unlock(&res->mutex2);
        pthread_mutex_unlock(&res->mutex1);
        
        sleep(1);
    }
    return NULL;
}

void deadlock_prevention_example() {
    printf("\n=== PREVENZIONE DEADLOCK ===\n");
    
    TwoResources resources;
    resources.resource1 = 0;
    resources.resource2 = 0;
    
    pthread_mutex_init(&resources.mutex1, NULL);
    pthread_mutex_init(&resources.mutex2, NULL);
    
    pthread_t t1, t2;
    
    printf("🚀 Avvio thread con STESSO ORDINE di lock (previene deadlock)\n");
    
    pthread_create(&t1, NULL, thread_order1, &resources);
    pthread_create(&t2, NULL, thread_order2, &resources);
    
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    
    printf("✅ Completato senza deadlock: r1=%d, r2=%d\n", 
           resources.resource1, resources.resource2);
    
    pthread_mutex_destroy(&resources.mutex1);
    pthread_mutex_destroy(&resources.mutex2);
}
```

---

## 🚦 Semafori

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

// Struttura per simulare pool di risorse
typedef struct {
    sem_t semaphore;               // Semaforo per controllo accesso
    int resource_count;            // Numero risorse disponibili
    int max_resources;             // Massimo risorse
    int *resource_ids;             // ID delle risorse
    pthread_mutex_t pool_mutex;    // Mutex per operazioni pool
    int total_acquisitions;       // Contatore acquisizioni totali
} ResourcePool;

// Struttura thread worker
typedef struct {
    int worker_id;
    ResourcePool* pool;
    int work_cycles;
    int work_duration;
} WorkerData;

// Crea pool di risorse
ResourcePool* create_resource_pool(int max_resources) {
    ResourcePool* pool = malloc(sizeof(ResourcePool));
    
    // Inizializza semaforo con numero risorse disponibili
    if (sem_init(&pool->semaphore, 0, max_resources) != 0) {
        perror("sem_init");
        free(pool);
        return NULL;
    }
    
    pool->max_resources = max_resources;
    pool->resource_count = max_resources;
    pool->resource_ids = malloc(max_resources * sizeof(int));
    
    // Inizializza ID risorse
    for (int i = 0; i < max_resources; i++) {
        pool->resource_ids[i] = i + 1;
    }
    
    pthread_mutex_init(&pool->pool_mutex, NULL);
    pool->total_acquisitions = 0;
    
    printf("🏭 Pool risorse creato: %d risorse disponibili\n", max_resources);
    return pool;
}

// Acquisisci risorsa dal pool
int acquire_resource(ResourcePool* pool, int worker_id) {
    printf("⏳ Worker %d: richiedo risorsa...\n", worker_id);
    
    // Wait sul semaforo (decrementa counter)
    if (sem_wait(&pool->semaphore) != 0) {
        perror("sem_wait");
        return -1;
    }
    
    // Sezione critica per gestione pool
    pthread_mutex_lock(&pool->pool_mutex);
    
    if (pool->resource_count > 0) {
        pool->resource_count--;
        int resource_id = pool->resource_ids[pool->resource_count];
        pool->total_acquisitions++;
        
        pthread_mutex_unlock(&pool->pool_mutex);
        
        printf("✅ Worker %d: acquisita risorsa #%d (rimangono %d)\n", 
               worker_id, resource_id, pool->resource_count);
        return resource_id;
    } else {
        // Non dovrebbe mai succedere se semaforo funziona correttamente
        pthread_mutex_unlock(&pool->pool_mutex);
        printf("❌ Worker %d: errore - nessuna risorsa disponibile!\n", worker_id);
        return -1;
    }
}

// Rilascia risorsa al pool
void release_resource(ResourcePool* pool, int worker_id, int resource_id) {
    // Sezione critica per gestione pool
    pthread_mutex_lock(&pool->pool_mutex);
    
    pool->resource_ids[pool->resource_count] = resource_id;
    pool->resource_count++;
    
    pthread_mutex_unlock(&pool->pool_mutex);
    
    // Signal sul semaforo (incrementa counter)
    if (sem_post(&pool->semaphore) != 0) {
        perror("sem_post");
    }
    
    printf("🔄 Worker %d: rilasciata risorsa #%d (disponibili %d)\n", 
           worker_id, resource_id, pool->resource_count);
}

// Thread worker che usa risorse
void* resource_worker(void* arg) {
    WorkerData* data = (WorkerData*)arg;
    
    printf("🚀 Worker %d avviato: %d cicli di lavoro\n", 
           data->worker_id, data->work_cycles);
    
    for (int cycle = 0; cycle < data->work_cycles; cycle++) {
        // Acquisisci risorsa
        int resource = acquire_resource(data->pool, data->worker_id);
        
        if (resource > 0) {
            // Usa risorsa
            printf("⚙️  Worker %d: uso risorsa #%d per %d secondi\n", 
                   data->worker_id, resource, data->work_duration);
            sleep(data->work_duration);
            
            // Rilascia risorsa
            release_resource(data->pool, data->worker_id, resource);
        }
        
        // Pausa tra cicli
        sleep(1);
    }
    
    printf("🏁 Worker %d completato\n", data->worker_id);
    return NULL;
}

// Esempio semafori base
void basic_semaphore_example() {
    printf("=== ESEMPIO SEMAFORI BASE ===\n");
    
    const int num_resources = 3;
    const int num_workers = 6;
    const int work_cycles = 2;
    
    // Crea pool risorse
    ResourcePool* pool = create_resource_pool(num_resources);
    if (!pool) return;
    
    // Crea worker thread
    pthread_t workers[num_workers];
    WorkerData worker_data[num_workers];
    
    printf("👷 Avvio %d worker per %d risorse\n", num_workers, num_resources);
    
    for (int i = 0; i < num_workers; i++) {
        worker_data[i].worker_id = i + 1;
        worker_data[i].pool = pool;
        worker_data[i].work_cycles = work_cycles;
        worker_data[i].work_duration = 2 + (rand() % 3);  // 2-4 secondi
        
        pthread_create(&workers[i], NULL, resource_worker, &worker_data[i]);
    }
    
    // Aspetta completamento
    for (int i = 0; i < num_workers; i++) {
        pthread_join(workers[i], NULL);
    }
    
    printf("📊 Risultati:\n");
    printf("   Acquisizioni totali: %d\n", pool->total_acquisitions);
    printf("   Risorse nel pool: %d/%d\n", pool->resource_count, pool->max_resources);
    
    // Cleanup
    sem_destroy(&pool->semaphore);
    pthread_mutex_destroy(&pool->pool_mutex);
    free(pool->resource_ids);
    free(pool);
}

// Producer-Consumer con semafori
#define BUFFER_SIZE 5

typedef struct {
    int buffer[BUFFER_SIZE];
    int head, tail;
    sem_t empty_slots;    // Semaforo slot vuoti
    sem_t filled_slots;   // Semaforo slot pieni
    pthread_mutex_t buffer_mutex;  // Mutex per accesso buffer
    int produced_count;
    int consumed_count;
} ProducerConsumerBuffer;

ProducerConsumerBuffer* create_pc_buffer() {
    ProducerConsumerBuffer* buf = malloc(sizeof(ProducerConsumerBuffer));
    
    buf->head = buf->tail = 0;
    buf->produced_count = buf->consumed_count = 0;
    
    // Semafori: inizialmente tutti slot vuoti
    sem_init(&buf->empty_slots, 0, BUFFER_SIZE);
    sem_init(&buf->filled_slots, 0, 0);
    
    pthread_mutex_init(&buf->buffer_mutex, NULL);
    
    return buf;
}

void* producer(void* arg) {
    ProducerConsumerBuffer* buf = (ProducerConsumerBuffer*)arg;
    
    for (int i = 1; i <= 10; i++) {
        int item = rand() % 100;
        
        // Wait per slot vuoto
        sem_wait(&buf->empty_slots);
        
        // Sezione critica: inserisci nel buffer
        pthread_mutex_lock(&buf->buffer_mutex);
        
        buf->buffer[buf->head] = item;
        buf->head = (buf->head + 1) % BUFFER_SIZE;
        buf->produced_count++;
        
        printf("📦 Producer: item %d inserito (prodotti: %d)\n", item, buf->produced_count);
        
        pthread_mutex_unlock(&buf->buffer_mutex);
        
        // Signal slot pieno
        sem_post(&buf->filled_slots);
        
        sleep(1);  // Simula tempo produzione
    }
    
    printf("🏭 Producer terminato\n");
    return NULL;
}

void* consumer(void* arg) {
    ProducerConsumerBuffer* buf = (ProducerConsumerBuffer*)arg;
    
    for (int i = 1; i <= 5; i++) {  // Consuma metà degli item
        // Wait per slot pieno
        sem_wait(&buf->filled_slots);
        
        // Sezione critica: rimuovi dal buffer
        pthread_mutex_lock(&buf->buffer_mutex);
        
        int item = buf->buffer[buf->tail];
        buf->tail = (buf->tail + 1) % BUFFER_SIZE;
        buf->consumed_count++;
        
        printf("📥 Consumer: item %d consumato (consumati: %d)\n", item, buf->consumed_count);
        
        pthread_mutex_unlock(&buf->buffer_mutex);
        
        // Signal slot vuoto
        sem_post(&buf->empty_slots);
        
        sleep(2);  // Simula tempo consumo (più lento del producer)
    }
    
    printf("🍽️  Consumer terminato\n");
    return NULL;
}

void producer_consumer_example() {
    printf("\n=== PRODUCER-CONSUMER CON SEMAFORI ===\n");
    
    ProducerConsumerBuffer* buffer = create_pc_buffer();
    
    pthread_t producer_thread, consumer_thread;
    
    printf("🏭 Avvio Producer-Consumer (buffer size: %d)\n", BUFFER_SIZE);
    
    pthread_create(&producer_thread, NULL, producer, buffer);
    pthread_create(&consumer_thread, NULL, consumer, buffer);
    
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);
    
    printf("📊 Risultati: prodotti %d, consumati %d\n", 
           buffer->produced_count, buffer->consumed_count);
    
    // Cleanup
    sem_destroy(&buffer->empty_slots);
    sem_destroy(&buffer->filled_slots);
    pthread_mutex_destroy(&buffer->buffer_mutex);
    free(buffer);
}

// Semafori POSIX named (tra processi)
void named_semaphore_example() {
    printf("\n=== SEMAFORI NAMED (INTER-PROCESSO) ===\n");
    
    const char* sem_name = "/example_semaphore";
    const int initial_value = 2;
    
    // Crea/apri semaforo named
    sem_t* named_sem = sem_open(sem_name, O_CREAT, 0644, initial_value);
    if (named_sem == SEM_FAILED) {
        perror("sem_open");
        return;
    }
    
    printf("📛 Semaforo named creato: %s (valore iniziale: %d)\n", sem_name, initial_value);
    
    // Test operazioni
    int sem_value;
    sem_getvalue(named_sem, &sem_value);
    printf("🔢 Valore corrente: %d\n", sem_value);
    
    // Wait (decrementa)
    printf("⬇️  sem_wait()...\n");
    sem_wait(named_sem);
    sem_getvalue(named_sem, &sem_value);
    printf("🔢 Valore dopo wait: %d\n", sem_value);
    
    // Post (incrementa)
    printf("⬆️  sem_post()...\n");
    sem_post(named_sem);
    sem_getvalue(named_sem, &sem_value);
    printf("🔢 Valore dopo post: %d\n", sem_value);
    
    // Cleanup
    sem_close(named_sem);
    sem_unlink(sem_name);  // Rimuovi dal sistema
    
    printf("🗑️  Semaforo named rimosso\n");
}
```

---

## 🚥 Condition Variables

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

// Struttura per controllo condizioni
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    bool condition_met;
    int waiting_threads;
    int total_signals;
} ConditionController;

// Struttura buffer con condition variables
typedef struct {
    int *buffer;
    int size;
    int count;
    int head, tail;
    
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;    // Condizione: buffer non vuoto
    pthread_cond_t not_full;     // Condizione: buffer non pieno
    
    int producers_active;
    int consumers_active;
    long items_produced;
    long items_consumed;
} ConditionBuffer;

// Thread che aspetta condizione
void* waiting_thread(void* arg) {
    ConditionController* ctrl = (ConditionController*)arg;
    int thread_id = (int)(long)arg;  // Hack per demo
    
    pthread_mutex_lock(&ctrl->mutex);
    
    ctrl->waiting_threads++;
    printf("⏳ Thread %d: in attesa condizione (waiting: %d)\n", 
           thread_id, ctrl->waiting_threads);
    
    // ASPETTA CONDIZIONE
    while (!ctrl->condition_met) {
        pthread_cond_wait(&ctrl->condition, &ctrl->mutex);
        printf("🔔 Thread %d: risvegliato! Controllo condizione...\n", thread_id);
    }
    
    printf("✅ Thread %d: condizione soddisfatta!\n", thread_id);
    ctrl->waiting_threads--;
    
    pthread_mutex_unlock(&ctrl->mutex);
    
    // Simula lavoro dopo condizione
    sleep(1);
    printf("🏁 Thread %d: completato\n", thread_id);
    
    return NULL;
}

// Thread che segnala condizione
void* signaling_thread(void* arg) {
    ConditionController* ctrl = (ConditionController*)arg;
    
    printf("🚀 Thread segnalatore avviato\n");
    sleep(3);  // Aspetta che thread si mettano in attesa
    
    pthread_mutex_lock(&ctrl->mutex);
    
    printf("📢 Segnalatore: imposto condizione e notifico\n");
    ctrl->condition_met = true;
    ctrl->total_signals++;
    
    // SEGNALA A TUTTI I THREAD IN ATTESA
    pthread_cond_broadcast(&ctrl->condition);
    
    pthread_mutex_unlock(&ctrl->mutex);
    
    printf("📡 Segnalatore: segnale inviato\n");
    return NULL;
}

// Esempio condition variables base
void basic_condition_example() {
    printf("=== ESEMPIO CONDITION VARIABLES BASE ===\n");
    
    const int num_waiting_threads = 4;
    
    ConditionController ctrl;
    ctrl.condition_met = false;
    ctrl.waiting_threads = 0;
    ctrl.total_signals = 0;
    
    pthread_mutex_init(&ctrl.mutex, NULL);
    pthread_cond_init(&ctrl.condition, NULL);
    
    pthread_t waiting_threads[num_waiting_threads];
    pthread_t signaler;
    
    // Crea thread in attesa
    for (int i = 0; i < num_waiting_threads; i++) {
        pthread_create(&waiting_threads[i], NULL, waiting_thread, 
                      (void*)(long)(i + 1));
        usleep(100000);  // Piccola pausa tra creazioni
    }
    
    // Crea thread segnalatore
    pthread_create(&signaler, NULL, signaling_thread, &ctrl);
    
    // Aspetta completamento
    pthread_join(signaler, NULL);
    
    for (int i = 0; i < num_waiting_threads; i++) {
        pthread_join(waiting_threads[i], NULL);
    }
    
    printf("📊 Completato: %d thread risvegliati con %d segnali\n", 
           num_waiting_threads, ctrl.total_signals);
    
    pthread_cond_destroy(&ctrl.condition);
    pthread_mutex_destroy(&ctrl.mutex);
}

// Buffer con condition variables
ConditionBuffer* create_condition_buffer(int size) {
    ConditionBuffer* buf = malloc(sizeof(ConditionBuffer));
    
    buf->buffer = malloc(size * sizeof(int));
    buf->size = size;
    buf->count = 0;
    buf->head = buf->tail = 0;
    buf->producers_active = 0;
    buf->consumers_active = 0;
    buf->items_produced = 0;
    buf->items_consumed = 0;
    
    pthread_mutex_init(&buf->mutex, NULL);
    pthread_cond_init(&buf->not_empty, NULL);
    pthread_cond_init(&buf->not_full, NULL);
    
    return buf;
}

void buffer_put(ConditionBuffer* buf, int item) {
    pthread_mutex_lock(&buf->mutex);
    
    // Aspetta finché buffer non è pieno
    while (buf->count == buf->size) {
        printf("⏳ Producer: buffer pieno, aspetto...\n");
        pthread_cond_wait(&buf->not_full, &buf->mutex);
    }
    
    // Inserisci item
    buf->buffer[buf->head] = item;
    buf->head = (buf->head + 1) % buf->size;
    buf->count++;
    buf->items_produced++;
    
    printf("📦 Producer: inserito item %d (buffer: %d/%d)\n", 
           item, buf->count, buf->size);
    
    // Segnala che buffer non è vuoto
    pthread_cond_signal(&buf->not_empty);
    
    pthread_mutex_unlock(&buf->mutex);
}

int buffer_get(ConditionBuffer* buf) {
    pthread_mutex_lock(&buf->mutex);
    
    // Aspetta finché buffer non è vuoto
    while (buf->count == 0) {
        printf("⏳ Consumer: buffer vuoto, aspetto...\n");
        pthread_cond_wait(&buf->not_empty, &buf->mutex);
    }
    
    // Rimuovi item
    int item = buf->buffer[buf->tail];
    buf->tail = (buf->tail + 1) % buf->size;
    buf->count--;
    buf->items_consumed++;
    
    printf("📥 Consumer: prelevato item %d (buffer: %d/%d)\n", 
           item, buf->count, buf->size);
    
    // Segnala che buffer non è pieno
    pthread_cond_signal(&buf->not_full);
    
    pthread_mutex_unlock(&buf->mutex);
    
    return item;
}

void* condition_producer(void* arg) {
    ConditionBuffer* buf = (ConditionBuffer*)arg;
    
    buf->producers_active++;
    printf("🏭 Producer avviato\n");
    
    for (int i = 1; i <= 8; i++) {
        int item = rand() % 100;
        buffer_put(buf, item);
        sleep(1);  // Simula tempo produzione
    }
    
    buf->producers_active--;
    printf("🏭 Producer terminato\n");
    return NULL;
}

void* condition_consumer(void* arg) {
    ConditionBuffer* buf = (ConditionBuffer*)arg;
    
    buf->consumers_active++;
    printf("🍽️  Consumer avviato\n");
    
    for (int i = 1; i <= 4; i++) {
        int item = buffer_get(buf);
        printf("🍽️  Consumer: elaboro item %d\n", item);
        sleep(2);  // Simula tempo elaborazione
    }
    
    buf->consumers_active--;
    printf("🍽️  Consumer terminato\n");
    return NULL;
}

void producer_consumer_condition_example() {
    printf("\n=== PRODUCER-CONSUMER CON CONDITION VARIABLES ===\n");
    
    ConditionBuffer* buffer = create_condition_buffer(3);
    
    pthread_t producers[2], consumers[2];
    
    printf("🚀 Avvio 2 producer e 2 consumer (buffer size: %d)\n", buffer->size);
    
    // Avvia consumer prima (per vedere wait su buffer vuoto)
    for (int i = 0; i < 2; i++) {
        pthread_create(&consumers[i], NULL, condition_consumer, buffer);
    }
    
    sleep(1);
    
    // Avvia producer
    for (int i = 0; i < 2; i++) {
        pthread_create(&producers[i], NULL, condition_producer, buffer);
    }
    
    // Aspetta completamento
    for (int i = 0; i < 2; i++) {
        pthread_join(producers[i], NULL);
        pthread_join(consumers[i], NULL);
    }
    
    printf("📊 Risultati: prodotti %ld, consumati %ld, buffer finale: %d\n",
           buffer->items_produced, buffer->items_consumed, buffer->count);
    
    // Cleanup
    pthread_cond_destroy(&buffer->not_empty);
    pthread_cond_destroy(&buffer->not_full);
    pthread_mutex_destroy(&buffer->mutex);
    free(buffer->buffer);
    free(buffer);
}

// Condition variables con timeout
int condition_wait_timeout(pthread_cond_t* cond, pthread_mutex_t* mutex, int timeout_ms) {
    struct timespec abs_timeout;
    clock_gettime(CLOCK_REALTIME, &abs_timeout);
    
    abs_timeout.tv_sec += timeout_ms / 1000;
    abs_timeout.tv_nsec += (timeout_ms % 1000) * 1000000;
    
    if (abs_timeout.tv_nsec >= 1000000000) {
        abs_timeout.tv_sec++;
        abs_timeout.tv_nsec -= 1000000000;
    }
    
    return pthread_cond_timedwait(cond, mutex, &abs_timeout);
}

void condition_timeout_example() {
    printf("\n=== CONDITION VARIABLES CON TIMEOUT ===\n");
    
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    bool condition_met = false;
    
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&condition, NULL);
    
    pthread_mutex_lock(&mutex);
    
    printf("⏰ Aspetto condizione con timeout 3 secondi...\n");
    
    while (!condition_met) {
        int result = condition_wait_timeout(&condition, &mutex, 3000);
        
        if (result == ETIMEDOUT) {
            printf("⏰ Timeout raggiunto - condizione non soddisfatta\n");
            break;
        } else if (result == 0) {
            printf("✅ Condizione soddisfatta!\n");
            break;
        } else {
            printf("❌ Errore wait: %s\n", strerror(result));
            break;
        }
    }
    
    pthread_mutex_unlock(&mutex);
    
    pthread_cond_destroy(&condition);
    pthread_mutex_destroy(&mutex);
}
```

---

## 🛠️ Utility Sincronizzazione Avanzate

```c
// Barrier semplice
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    int thread_count;
    int waiting_threads;
    int barrier_generation;
} SimpleBarrier;

SimpleBarrier* create_barrier(int thread_count) {
    SimpleBarrier* barrier = malloc(sizeof(SimpleBarrier));
    
    pthread_mutex_init(&barrier->mutex, NULL);
    pthread_cond_init(&barrier->condition, NULL);
    barrier->thread_count = thread_count;
    barrier->waiting_threads = 0;
    barrier->barrier_generation = 0;
    
    return barrier;
}

void barrier_wait(SimpleBarrier* barrier) {
    pthread_mutex_lock(&barrier->mutex);
    
    int current_generation = barrier->barrier_generation;
    barrier->waiting_threads++;
    
    if (barrier->waiting_threads == barrier->thread_count) {
        // Ultimo thread - risveglia tutti
        barrier->waiting_threads = 0;
        barrier->barrier_generation++;
        pthread_cond_broadcast(&barrier->condition);
    } else {
        // Aspetta che tutti arrivino
        while (current_generation == barrier->barrier_generation) {
            pthread_cond_wait(&barrier->condition, &barrier->mutex);
        }
    }
    
    pthread_mutex_unlock(&barrier->mutex);
}

// Read-Write Lock
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t read_cond;
    pthread_cond_t write_cond;
    int readers;
    int writers;
    int waiting_writers;
} RWLock;

RWLock* create_rwlock() {
    RWLock* rw = malloc(sizeof(RWLock));
    
    pthread_mutex_init(&rw->mutex, NULL);
    pthread_cond_init(&rw->read_cond, NULL);
    pthread_cond_init(&rw->write_cond, NULL);
    rw->readers = rw->writers = rw->waiting_writers = 0;
    
    return rw;
}

void read_lock(RWLock* rw) {
    pthread_mutex_lock(&rw->mutex);
    
    while (rw->writers > 0 || rw->waiting_writers > 0) {
        pthread_cond_wait(&rw->read_cond, &rw->mutex);
    }
    
    rw->readers++;
    pthread_mutex_unlock(&rw->mutex);
}

void read_unlock(RWLock* rw) {
    pthread_mutex_lock(&rw->mutex);
    
    rw->readers--;
    if (rw->readers == 0) {
        pthread_cond_signal(&rw->write_cond);
    }
    
    pthread_mutex_unlock(&rw->mutex);
}

void write_lock(RWLock* rw) {
    pthread_mutex_lock(&rw->mutex);
    
    rw->waiting_writers++;
    while (rw->readers > 0 || rw->writers > 0) {
        pthread_cond_wait(&rw->write_cond, &rw->mutex);
    }
    rw->waiting_writers--;
    rw->writers++;
    
    pthread_mutex_unlock(&rw->mutex);
}

void write_unlock(RWLock* rw) {
    pthread_mutex_lock(&rw->mutex);
    
    rw->writers--;
    pthread_cond_broadcast(&rw->read_cond);
    pthread_cond_signal(&rw->write_cond);
    
    pthread_mutex_unlock(&rw->mutex);
}

// Spinlock semplice (busy waiting)
typedef struct {
    volatile int locked;
} Spinlock;

void spinlock_init(Spinlock* lock) {
    lock->locked = 0;
}

void spinlock_lock(Spinlock* lock) {
    while (__sync_lock_test_and_set(&lock->locked, 1)) {
        // Busy wait
        while (lock->locked) {
            // Pause instruction per efficienza su x86
            asm volatile("pause" ::: "memory");
        }
    }
}

void spinlock_unlock(Spinlock* lock) {
    __sync_lock_release(&lock->locked);
}

// Operazioni atomiche
typedef struct {
    volatile long value;
} AtomicLong;

void atomic_init(AtomicLong* atomic, long initial_value) {
    atomic->value = initial_value;
}

long atomic_increment(AtomicLong* atomic) {
    return __sync_add_and_fetch(&atomic->value, 1);
}

long atomic_decrement(AtomicLong* atomic) {
    return __sync_sub_and_fetch(&atomic->value, 1);
}

long atomic_get(AtomicLong* atomic) {
    return __sync_fetch_and_add(&atomic->value, 0);
}

bool atomic_compare_and_swap(AtomicLong* atomic, long expected, long new_value) {
    return __sync_bool_compare_and_swap(&atomic->value, expected, new_value);
}
```

---

## 📋 Checklist Synchronization

### ✅ **Mutex**
- [ ] Inizializza con `pthread_mutex_init()`
- [ ] Lock prima sezione critica, unlock dopo
- [ ] Stesso thread che fa lock deve fare unlock
- [ ] Evita deadlock con ordine consistente dei lock
- [ ] Destroy con `pthread_mutex_destroy()`

### ✅ **Semafori**
- [ ] Inizializza con valore appropriato
- [ ] `sem_wait()` per decrementare (P operation)
- [ ] `sem_post()` per incrementare (V operation)
- [ ] Named semaphori per comunicazione inter-processo
- [ ] Cleanup con `sem_destroy()` o `sem_unlink()`

### ✅ **Condition Variables**
- [ ] Sempre usate insieme a mutex
- [ ] Loop while per verificare condizione
- [ ] `pthread_cond_wait()` rilascia mutex automaticamente
- [ ] `pthread_cond_signal()` per single thread
- [ ] `pthread_cond_broadcast()` per tutti i thread

### ✅ **Best Practices**
- [ ] Minimizza durata sezioni critiche
- [ ] Evita operazioni bloccanti in sezioni critiche
- [ ] Gestisci timeout per evitare deadlock
- [ ] Usa atomic operations per contatori semplici
- [ ] Test con thread sanitizer per race conditions

---

## 🎯 Test e Debug

```bash
# Compila con thread support
gcc -o sync_test sync.c -lpthread

# Compila con sanitizer per race conditions
gcc -fsanitize=thread -g -o sync_test sync.c -lpthread

# Test con valgrind
valgrind --tool=helgrind ./sync_test

# Test stress
for i in {1..100}; do ./sync_test; done

# Monitor thread attivi
ps -eLf | grep sync_test
```

## 🚀 Performance Synchronization

| **Primitive** | **Overhead** | **Uso Ideale** |
|---------------|--------------|----------------|
| **Atomic** | Molto basso | Contatori, flag |
| **Spinlock** | Basso (CPU intensivo) | Sezioni critiche brevi |
| **Mutex** | Medio | Sezioni critiche generali |
| **Semaforo** | Medio | Pool risorse |
| **Condition Var** | Alto | Coordinazione complessa |