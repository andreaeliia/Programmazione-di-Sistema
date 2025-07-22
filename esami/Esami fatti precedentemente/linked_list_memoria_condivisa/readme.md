# Lista Concatenata in Memoria Condivisa - Traccia Esame

## 📋 Richiesta della Traccia

Si permetta a due processi di gestire, congiuntamente in un'area di memoria condivisa, la stessa lista concatenata (linked list) di elementi descritti da una struttura dati, (con le funzioni pop, insert, count). Per la condivisione della memoria e per la gestione della concorrenza, si usi la POSIX IPC. Si fornisca a entrambi i processi un'interfaccia utente testuale per ricevere dall'utente i comandi.

## 🎯 Obiettivi dell'Esercizio

- ✅ **Memoria condivisa POSIX** tra due processi
- ✅ **Lista concatenata** implementata correttamente
- ✅ **Sincronizzazione** per evitare race conditions
- ✅ **Funzioni richieste**: insert, pop, count
- ✅ **Interfaccia utente testuale** per entrambi i processi

## 🏗️ Architettura della Soluzione

### Componenti Principali

1. **Memoria Condivisa POSIX**
   - `shm_open()` - Crea/apre segmento di memoria condivisa
   - `mmap()` - Mappa il segmento nella memoria del processo
   - `shm_unlink()` - Rimuove il segmento quando non più necessario

2. **Lista Concatenata con Offset**
   - Pool fisso di nodi (array)
   - Offset invece di puntatori (portabili tra processi)
   - Gestione allocazione/deallocazione nodi

3. **Sincronizzazione**
   - Semafori POSIX per mutua esclusione
   - Protezione sezioni critiche
   - Prevenzione race conditions

4. **Interfaccia Utente**
   - Menu testuale interattivo
   - Gestione input utente
   - Feedback operazioni

## 🔧 Problemi Risolti e Soluzioni

### Problema 1: Puntatori tra Processi
**❌ Problema**: I puntatori normali non funzionano tra processi diversi
```c
// ❌ NON FUNZIONA
struct Node {
    int data;
    struct Node* next;  // Indirizzo assoluto!
};
```

**✅ Soluzione**: Usare offset relativi
```c
// ✅ FUNZIONA
typedef struct {
    int data;
    int next_offset;    // Posizione relativa!
    int is_free;
} Node;
```

### Problema 2: Allocazione Dinamica in Memoria Condivisa
**❌ Problema**: `malloc()` alloca nell'heap privato del processo

**✅ Soluzione**: Pool fisso di nodi
```c
typedef struct {
    Node nodes[MAX_NODES];  // Pool fisso
    int head_offset;
    int count;
    sem_t mutex;
} SharedMemory;
```

### Problema 3: Race Conditions
**❌ Problema**: Due processi modificano simultaneamente la lista

**✅ Soluzione**: Semafori POSIX
```c
void list_insert(SharedMemory* shm, int data, const char* process_name) {
    sem_wait(&shm->mutex);  // 🔒 Lock
    // ... operazioni sulla lista ...
    sem_post(&shm->mutex);  // 🔓 Unlock
}
```

### Problema 4: Output Sovrapposto
**❌ Problema**: fork() causa output mescolato nel terminale

**✅ Soluzione**: Gestione multi-terminale
- Prima istanza crea memoria condivisa
- Seconda istanza si connette alla memoria esistente
- Ogni processo ha il suo terminale pulito

## 📂 Struttura del Progetto

```
progetto/
├── traccia_completa.c    # Codice sorgente principale
├── README.md            # Questa documentazione
├── Makefile            # Script di compilazione
└── .gitignore          # File git ignore (opzionale)
```

## 🚀 Compilazione ed Esecuzione

### Compilazione
```bash
# Metodo 1: Con Makefile
make

# Metodo 2: Manuale
gcc -o traccia traccia_completa.c -lrt -lpthread
```

### Esecuzione
```bash
# Terminale 1
./traccia
# Scegli: 1 (PROCESSO-A)

# Terminale 2 (apri nuovo terminale)
./traccia
# Scegli: 2 (PROCESSO-B)
```

### Pulizia
```bash
make clean
# oppure
rm -f traccia *.o
```

## 🎮 Utilizzo del Programma

### Menu Principale
```
=== MENU PROCESSO-A ===
1. Inserisci elemento
2. Rimuovi elemento (pop)
3. Conta elementi
4. Stampa lista
5. Inserisci multipli elementi
6. Stato memoria (debug)
0. Esci
```

### Esempio di Utilizzo

**Terminale 1 (PROCESSO-A):**
```bash
Scelta: 1
Inserisci valore: 10
✅ Elemento 10 inserito!

Scelta: 1
Inserisci valore: 20
✅ Elemento 20 inserito!

Scelta: 4
PROCESSO-A: Lista: [20 -> 10] (count: 2)
```

**Terminale 2 (PROCESSO-B):**
```bash
Scelta: 4
PROCESSO-B: Lista: [20 -> 10] (count: 2)

Scelta: 2
✅ Elemento 20 rimosso!

Scelta: 3
📊 La lista contiene 1 elementi
```

## 🔍 Dettagli Tecnici

### Conversioni Offset ↔ Puntatore

```c
// Offset → Puntatore
Node* get_node_by_offset(SharedMemory* shm, int offset) {
    if (offset == -1) return NULL;
    return &shm->nodes[offset];
}

// Puntatore → Offset
int get_offset_from_node(SharedMemory* shm, Node* node) {
    if (node == NULL) return -1;
    return node - shm->nodes;
}
```

### Gestione Pool di Nodi

```c
// Allocazione
int allocate_node(SharedMemory* shm) {
    for (int i = 0; i < MAX_NODES; i++) {
        if (shm->nodes[i].is_free) {
            shm->nodes[i].is_free = 0;
            return i;
        }
    }
    return -1;  // Pool esaurito
}

// Deallocazione
void free_node(SharedMemory* shm, int offset) {
    if (offset >= 0 && offset < MAX_NODES) {
        shm->nodes[offset].is_free = 1;
    }
}
```

### Operazioni Lista Thread-Safe

Tutte le operazioni sono protette da semafori:

```c
void list_operation(SharedMemory* shm, ...) {
    sem_wait(&shm->mutex);    // 🔒 Acquisisce lock
    // === SEZIONE CRITICA ===
    // Modifica strutture dati condivise
    // === FINE SEZIONE CRITICA ===
    sem_post(&shm->mutex);    // 🔓 Rilascia lock
}
```

## 📚 Concetti di Programmazione di Sistema

### 1. Memoria Condivisa POSIX
- **shm_open()**: Crea/apre segmento di memoria condivisa con nome
- **ftruncate()**: Imposta dimensione del segmento
- **mmap()**: Mappa segmento nella memoria virtuale del processo
- **munmap()**: Smappa segmento dalla memoria del processo
- **shm_unlink()**: Rimuove segmento dal sistema

### 2. Sincronizzazione
- **sem_init()**: Inizializza semaforo (valore iniziale = 1 per mutex)
- **sem_wait()**: Decrementa semaforo (blocca se valore = 0)
- **sem_post()**: Incrementa semaforo (sblocca processi in attesa)
- **sem_destroy()**: Distrugge semaforo

### 3. Gestione Processi
- **fork()**: Crea processo figlio (non usato in questa versione)
- **wait()**: Processo padre aspetta terminazione figlio
- **getpid()**: Ottiene Process ID del processo corrente

## 🐛 Debugging e Risoluzione Problemi

### Errori Comuni

1. **Errore di compilazione: "undefined reference to shm_open"**
   ```bash
   # Soluzione: Aggiungi -lrt
   gcc traccia_completa.c -lrt -lpthread
   ```

2. **Errore di compilazione: "undefined reference to sem_init"**
   ```bash
   # Soluzione: Aggiungi -lpthread
   gcc traccia_completa.c -lrt -lpthread
   ```

3. **Errore runtime: "Permission denied" su shm_open**
   ```bash
   # Soluzione: Verifica permessi o usa nome diverso
   # Cambia SHM_NAME nel codice
   ```

4. **Memoria condivisa rimane attiva dopo crash**
   ```bash
   # Pulizia manuale
   ipcrm -M /linked_list_shm
   # oppure
   ls /dev/shm/
   rm /dev/shm/linked_list_shm
   ```

### Debug con Opzione 6 (Stato Memoria)

Il menu include un'opzione di debug che mostra:
- Offset del primo nodo
- Numero elementi nella lista
- Utilizzo del pool di nodi
- Percentuale memoria utilizzata

## 📖 Librerie e Dipendenze

### Header Files Necessari
```c
#include <stdio.h>          // I/O standard
#include <stdlib.h>         // Funzioni utility
#include <unistd.h>         // System calls POSIX
#include <sys/wait.h>       // Gestione processi
#include <sys/mman.h>       // Memory mapping
#include <sys/stat.h>       // Costanti file
#include <fcntl.h>          // File control
#include <string.h>         // Manipolazione stringhe
#include <semaphore.h>      // Semafori POSIX
```

### Librerie da Linkare
- **-lrt**: POSIX Real-Time library (shm_*, timer, message queues)
- **-lpthread**: POSIX Threads library (pthread_*, sem_*)

## 🎓 Preparazione Esame

### Domande Tipiche

1. **Perché usare offset invece di puntatori?**
   - I puntatori sono indirizzi assoluti validi solo nel processo che li crea
   - Gli offset sono posizioni relative valide per tutti i processi
   - Ogni processo mappa la memoria condivisa a indirizzi diversi

2. **Come funziona la sincronizzazione?**
   - Semaforo inizializzato a valore 1 (binary semaphore = mutex)
   - sem_wait() decrementa: se valore = 0, il processo si blocca
   - sem_post() incrementa: sblocca eventuale processo in attesa
   - Garantisce che solo un processo alla volta acceda alla lista

3. **Perché un pool fisso invece di malloc?**
   - malloc() alloca nell'heap privato del processo
   - La memoria condivisa deve contenere tutto ciò che è condiviso
   - Un pool fisso permette allocazione/deallocazione in memoria condivisa

4. **Gestione race conditions?**
   - Senza sincronizzazione: due processi potrebbero leggere stesso valore, modificare, uno sovrascrive l'altro
   - Con semafori: accesso esclusivo alle sezioni critiche
   - Operazioni atomiche garantite

### Varianti Possibili
- Lista bidirezionale (prev_offset)
- Operazioni aggiuntive (search, delete specifico)
- Più di due processi
- Code o stack invece di lista
- Uso di message queues invece di semafori

## 📝 Note per l'Esame

### Punti Salienti da Ricordare
1. **POSIX IPC** è il requisito specifico della traccia
2. **Offset vs puntatori** è la chiave per far funzionare liste tra processi
3. **Semafori** sono essenziali per evitare corruption dei dati
4. **Pool fisso** risolve il problema dell'allocazione dinamica
5. **Interfaccia utente** deve essere presente in entrambi i processi

### Estensioni Possibili
- Salvataggio/caricamento lista da file
- Statistiche utilizzo (tempo operazioni, contatori)
- Gestione priorità elementi
- Ordinamento automatico
- Limite massimo elementi configurabile

---

**Autore**: [Il tuo nome]  
**Data**: [Data]  
**Corso**: Programmazione di Sistema  
**Traccia**: Lista Concatenata in Memoria Condivisa