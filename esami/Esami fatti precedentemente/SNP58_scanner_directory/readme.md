# Directory Scanner Multi-Thread

## Descrizione del Problema

Programma che implementa due thread con funzionalità distinte:

1. **Thread Scanner**: Visita ricorsivamente tutti i nodi all'interno di una directory, registrando per ogni nodo la dimensione in byte e il numero di hard link associati

2. **Thread Counter**: Accumula il numero totale di byte di tutti i nodi, omettendo l'incremento quando l'inode è già stato considerato (per evitare doppi conteggi con hard link)

## Struttura del Progetto

```
directory_scanner.c    - Programma principale con implementazione completa
README.md             - Questa documentazione
```

## Compilazione

Il programma è compatibile con standard C90 e utilizza POSIX threads:

```bash
# Compilazione standard
gcc -ansi -Wall directory_scanner.c -o directory_scanner -lpthread

# Compilazione con debug
gcc -ansi -Wall -g directory_scanner.c -o directory_scanner -lpthread

# Compilazione ottimizzata
gcc -ansi -Wall -O2 directory_scanner.c -o directory_scanner -lpthread
```

## Esecuzione

```bash
./directory_scanner <directory>
```

Esempi:
```bash
./directory_scanner /home/user/Documents
./directory_scanner /tmp
./directory_scanner .
```

## Output Esempio

```
=== SCANNER DIRECTORY CON DUE THREAD ===
Directory da scansionare: /home/user/test

Thread Scanner: Inizio scansione di /home/user/test
Thread Counter: Inizio conteggio
Scanner: /home/user/test/file1.txt (inode=12345, size=1024, links=1)
Counter: +1024 byte da /home/user/test/file1.txt (totale: 1024)
Scanner: /home/user/test/link_to_file1.txt (inode=12345, size=1024, links=2)
Counter: Saltato /home/user/test/link_to_file1.txt (inode 12345 già conteggiato)
Scanner: /home/user/test/subdir/file2.txt (inode=12346, size=2048, links=1)
Counter: +2048 byte da /home/user/test/subdir/file2.txt (totale: 3072)
Thread Scanner: Scansione completata
Thread Counter: Completato
  Nodi processati: 3
  Nodi saltati (hard link): 1
  Inode unici visitati: 2

=== RISULTATI FINALI ===
Byte totali (senza doppi conteggi): 3072
Errori durante scansione: 0
Dimensione totale: 3.00 KB

Scansione completata.
```

## Architettura del Programma

### Thread 1: Scanner
**Responsabilità:**
- Visita ricorsiva della directory con `opendir()`/`readdir()`
- Ottenimento informazioni sui file con `lstat()`
- Inserimento risultati nel buffer condiviso
- Gestione di directory, file regolari, link simbolici

**Funzioni principali:**
```c
void* scanner_thread(void *arg)           - Funzione principale del thread
void scan_directory_recursive(const char *dir_path)  - Scansione ricorsiva
void add_node_to_buffer(const NodeInfo *node)        - Aggiunta al buffer
```

### Thread 2: Counter
**Responsabilità:**
- Lettura risultati dal buffer condiviso
- Tracking inode già visitati per evitare doppi conteggi
- Accumulo del totale dei byte
- Stampa statistiche finali

**Funzioni principali:**
```c
void* counter_thread(void *arg)           - Funzione principale del thread
int get_node_from_buffer(NodeInfo *node) - Lettura dal buffer
int is_inode_visited(ino_t inode)         - Verifica inode già visitato
```

## Strutture Dati

### NodeInfo
```c
typedef struct {
    char path[MAX_PATH_LEN];  - Percorso completo del file
    off_t size;               - Dimensione in byte
    nlink_t hard_links;       - Numero di hard link
    ino_t inode;              - Numero inode
    int valid;                - Flag validità entry
} NodeInfo;
```

### SharedBuffer (Producer-Consumer)
```c
typedef struct {
    NodeInfo nodes[BUFFER_SIZE];  - Buffer circolare
    int write_index;              - Indice scrittura (producer)
    int read_index;               - Indice lettura (consumer)
    int count;                    - Elementi presenti
    int finished;                 - Flag fine scansione
    pthread_mutex_t mutex;        - Mutex per accesso esclusivo
    pthread_cond_t not_full;      - Condizione buffer non pieno
    pthread_cond_t not_empty;     - Condizione buffer non vuoto
} SharedBuffer;
```

### InodeTracker
```c
typedef struct {
    ino_t inodes[MAX_INODES];     - Array inode visitati
    int count;                    - Numero inode tracciati
    pthread_mutex_t mutex;        - Mutex per accesso esclusivo
} InodeTracker;
```

## Sincronizzazione

### Pattern Producer-Consumer
- **Producer** (Scanner): Inserisce `NodeInfo` nel buffer circolare
- **Consumer** (Counter): Estrae `NodeInfo` dal buffer circolare
- **Sincronizzazione**: Mutex + condition variables per evitare busy waiting

### Gestione Hard Link
- Utilizza numero `inode` per identificare univocamente i file
- Array `InodeTracker` mantiene lista inode già conteggiati
- Primo accesso all'inode: contribuisce al totale byte
- Accessi successivi: saltati per evitare doppi conteggi

### Terminazione Coordinata
```c
/* Scanner segnala fine lavoro */
signal_scan_finished();

/* Counter termina quando buffer vuoto E scanner finito */
while (get_node_from_buffer(&node)) { ... }
```

## Configurazione

### Parametri Modificabili
```c
#define MAX_PATH_LEN 4096     /* Lunghezza massima percorso */
#define BUFFER_SIZE 1000      /* Dimensione buffer circolare */
#define MAX_INODES 10000      /* Massimo numero inode tracciati */
```

### Gestione Errori
- **Controllo accessi**: Verifica permessi su directory e file
- **Gestione `errno`**: Messaggi di errore dettagliati con `strerror()`
- **Contatore errori**: Tracking errori durante scansione
- **Cleanup automatico**: Rilascio mutex e condition variables

## Funzionalità Implementate

### Tipo di File Supportati
- **File regolari**: Conteggiati normalmente
- **Directory**: Attraversate ricorsivamente
- **Link simbolici**: Seguiti con `lstat()` (non `stat()`)
- **Device files, FIFO, socket**: Rilevati e processati

### Ottimizzazioni
- **Buffer circolare**: Evita allocazioni dinamiche
- **Condition variables**: Elimina busy waiting
- **Lookup inode efficiente**: Array semplice per tracking
- **Percorsi assoluti**: Evita problemi con `chdir()`

### Statistiche Dettagliate
- Numero nodi processati vs saltati
- Conteggio errori durante scansione
- Numero inode unici visitati
- Conversione automatica unità (B/KB/MB/GB)

## Esempio Casi d'Uso

### Directory con Hard Link
```bash
# Crea test con hard link
mkdir test_dir
echo "contenuto" > test_dir/file_originale.txt
ln test_dir/file_originale.txt test_dir/hard_link.txt

# Esegui scanner
./directory_scanner test_dir

# Output atteso: dimensione contata una sola volta
```

### Directory di Sistema
```bash
# Scansione directory di sistema (richiede permessi)
./directory_scanner /etc
./directory_scanner /var/log
```

### Directory con Link Simbolici
```bash
# I link simbolici sono seguiti ma non attraversati ricorsivamente
mkdir test_symlink
echo "test" > test_symlink/file.txt
ln -s file.txt test_symlink/symbolic_link.txt
./directory_scanner test_symlink
```

## Note di Compatibilità C90

### Conformità Standard
- Tutte le dichiarazioni variabili all'inizio delle funzioni
- Solo commenti `/* */`
- Nessuna funzionalità C99 (VLA, dichiarazioni miste, etc.)
- Compatibile con flag `-ansi`

### Dipendenze POSIX
- **POSIX Threads**: `pthread_create()`, `pthread_join()`
- **POSIX Filesystem**: `opendir()`, `readdir()`, `lstat()`
- **POSIX IPC**: `pthread_mutex_t`, `pthread_cond_t`

## Limitazioni

### Dimensioni Massime
- **Percorsi**: 4096 caratteri (PATH_MAX)
- **Inode tracking**: 10000 inode unici
- **Buffer size**: 1000 elementi simultanei

### Considerazioni Prestazioni
- **Memoria**: O(n) per tracking inode dove n = numero file unici
- **I/O**: Un thread per I/O filesystem, un thread per elaborazione
- **Sincronizzazione**: Overhead minimo con condition variables

## Troubleshooting

### Errori Comuni

**"Permission denied"**
```bash
# Esegui con permessi sufficienti o cambia directory
sudo ./directory_scanner /root
# oppure
./directory_scanner ~/Documents
```

**"Too many open files"**
```bash
# Aumenta limite file descriptor
ulimit -n 4096
```

**Segmentation fault su directory molto grandi**
```bash
# Aumenta MAX_INODES nel codice
#define MAX_INODES 50000
```

### Debug
```bash
# Compilazione debug
gcc -ansi -Wall -g -DDEBUG directory_scanner.c -o directory_scanner -lpthread

# Esecuzione con gdb
gdb ./directory_scanner
(gdb) run /path/to/directory
```

### Verifica Risultati
```bash
# Confronta con du per verifica
du -sb /path/to/directory
./directory_scanner /path/to/directory

# I risultati dovrebbero coincidere
```

## Estensioni Possibili

- Supporto per filtri file (estensioni, dimensione)
- Threading pool per directory molto grandi
- Interfaccia progress bar
- Output formato JSON/XML
- Confronto tra due directory
- Individuazione file duplicati (stesso contenuto)
- Integrazione con database per persistenza risultati