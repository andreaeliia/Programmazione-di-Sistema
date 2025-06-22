# THREAD - Guida Completa Thread Programming

## LIBRERIE NECESSARIE

```c
#include <pthread.h>     // pthread_create, pthread_join, pthread_mutex_*
#include <stdio.h>       // printf, perror
#include <stdlib.h>      // exit, malloc, free
#include <unistd.h>      // sleep, usleep
#include <string.h>      // memset, strerror
#include <errno.h>       // errno
#include <time.h>        // time, clock_gettime (per timestamp)
#include <signal.h>      // signal, pthread_kill (per gestione segnali)
```

**COMPILAZIONE:** Sempre includere `-lpthread`
```bash
gcc -o program program.c -lpthread
```

## COS'È UN THREAD

### Definizione
Un **thread** (filo) è un flusso di esecuzione leggero all'interno di un processo. Più thread condividono lo stesso spazio di memoria ma possono eseguire codice diverso contemporaneamente.

### Differenza Processo vs Thread
```
PROCESSO                    THREAD
├─ Memoria separata        ├─ Memoria condivisa
├─ PID unico               ├─ Stesso PID, TID diverso  
├─ Comunicazione IPC       ├─ Variabili globali
├─ Fork pesante            ├─ Creazione leggera
├─ Isolamento completo     ├─ Condivisione risorse
└─ Crash indipendenti      └─ Crash affetta tutti
```

### Vantaggi Thread
- **Performance**: Creazione più veloce dei processi
- **Memoria condivisa**: Comunicazione semplice tra thread
- **Parallelismo**: Sfrutta CPU multi-core
- **Responsività**: UI non si blocca durante elaborazioni

### Svantaggi Thread
- **Sincronizzazione**: Complessità gestione accesso condiviso
- **Race conditions**: Risultati non deterministici
- **Debugging difficile**: Problemi non riproducibili
- **Crash propagati**: Un thread può far crashare tutto

## CONCETTI FONDAMENTALI

### Stati di un Thread
```
CREATED → RUNNABLE → RUNNING → BLOCKED → TERMINATED
   ↑         ↑          ↑         ↑         ↑
pthread_ → Scheduler → CPU → Wait → pthread_
create                        mutex   join
```

### Memoria Condivisa vs Privata
```c
// CONDIVISO tra tutti i thread:
int global_var = 42;           // Variabili globali
static int static_var = 0;     // Variabili statiche  
int *heap_ptr = malloc(100);   // Memoria heap

// PRIVATO per ogni thread:
void thread_function() {
    int local_var = 10;        // Variabili locali (stack)
    static int call_count = 0; // ATTENZIONE: static è condiviso!
}
```

### Race Condition
```c
// PROBLEMA: Due thread modificano la stessa variabile
int counter = 0;

void* increment_thread(void* arg) {
    for (int i = 0; i < 1000000; i++) {
        counter++;  // NON ATOMICO!
        // 1. Leggi counter
        // 2. Incrementa  
        // 3. Scrivi counter
        // → Un altro thread può interrompere tra 1 e 3!
    }
    return NULL;
}

// Risultato: counter != 2000000 (valore atteso)
```

## FUNZIONI PTHREAD ESSENZIALI

### pthread_create() - Creazione Thread
```c
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg);

// PARAMETRI:
// thread: Puntatore a pthread_t per ID thread (OUTPUT)
// attr: Attributi thread (NULL = default)
// start_routine: Funzione che il thread eseguirà
// arg: Argomento da passare alla funzione

// RITORNA:
// 0: Successo
// != 0: Codice errore (non setta errno!)

// ESEMPIO:
pthread_t thread_id;
int result = pthread_create(&thread_id, NULL, worker_function, &data);
if (result != 0) {
    printf("Errore pthread_create: %s\n", strerror(result));
    exit(1);
}
```

### Funzione Thread
```c
// PROTOTIPO OBBLIGATORIO:
void* function_name(void* arg);

// ESEMPIO:
void* worker_function(void* arg) {
    int *number = (int*)arg;  // Cast del parametro
    
    printf("Thread %lu: ricevuto %d\n", pthread_self(), *number);
    
    // Lavoro del thread...
    
    // Valore di ritorno (opzionale)
    int *result = malloc(sizeof(int));
    *result = 42;
    return result;  // Deve essere void*
}
```

### pthread_join() - Attesa Terminazione
```c
int pthread_join(pthread_t thread, void **retval);

// PARAMETRI:
// thread: ID del thread da aspettare
// retval: Puntatore per ricevere valore ritorno (NULL se non serve)

// COMPORTAMENTO:
// - BLOCCA fino a terminazione del thread specificato
// - Recupera risorse del thread (come wait() per processi)
// - Un thread può essere "joined" solo una volta

// ESEMPIO:
void *return_value;
int result = pthread_join(thread_id, &return_value);
if (result != 0) {
    printf("Errore pthread_join: %s\n", strerror(result));
}

if (return_value != NULL) {