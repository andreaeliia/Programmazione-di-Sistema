# Confronto Prestazioni IPC: Pipe, TCP, Shared Memory

## Descrizione del Problema

Un processo genera una sequenza di un milione di numeri interi casuali e la trasferisce ad un altro processo. Il programma confronta i tempi necessari al trasferimento impiegando tre metodi differenti:

1. **Unnamed Pipe** - Comunicazione tramite pipe anonima
2. **TCP Locale** - Trasferimento TCP eseguito localmente (127.0.0.1)
3. **Memoria Condivisa** - Condivisione diretta della memoria tra processi

## Struttura del Progetto

```
ipc_comparison.c    - Programma principale con tutti e 3 i test
README.md          - Questa documentazione
```

## Compilazione

Il programma è compatibile con standard C90 e richiede le librerie di sistema per IPC:

```bash
# Compilazione standard
gcc -ansi -Wall ipc_comparison.c -o ipc_comparison -lrt

# Compilazione con debug
gcc -ansi -Wall -g ipc_comparison.c -o ipc_comparison -lrt

# Compilazione ottimizzata
gcc -ansi -Wall -O2 ipc_comparison.c -o ipc_comparison -lrt
```

## Esecuzione

```bash
./ipc_comparison
```

Il programma esegue automaticamente tutti e tre i test in sequenza e mostra:
- Tempo di esecuzione per ogni metodo
- Verifica dell'integrità dei dati trasferiti
- Classifica ordinata dal metodo più veloce

## Output Esempio

```
=== CONFRONTO PRESTAZIONI IPC ===
Trasferimento di 1000000 numeri interi casuali

Test 1: Unnamed Pipe
  Trasferimento completato correttamente
  Tempo: 45.23 ms

Test 2: TCP Locale
  Trasferimento completato correttamente
  Tempo: 78.91 ms

Test 3: Memoria Condivisa
  Trasferimento completato correttamente
  Tempo: 12.45 ms

=== RIEPILOGO RISULTATI ===
Unnamed Pipe:      45.23 ms
TCP Locale:        78.91 ms
Memoria Condivisa: 12.45 ms

=== CLASSIFICA (dal più veloce) ===
1. Memoria Condivisa (12.45 ms)
2. Unnamed Pipe (45.23 ms)
3. TCP Locale (78.91 ms)

Test completato.
```

## Dettagli Implementazione

### Test 1: Unnamed Pipe

**Meccanismo:**
- Crea una pipe anonima con `pipe()`
- Fork del processo per creare produttore e consumatore
- Il figlio scrive tutti i numeri nella pipe
- Il padre legge tutti i numeri dalla pipe

**Caratteristiche:**
- Comunicazione unidirezionale
- Buffer del kernel limitato
- Sincronizzazione automatica (blocking I/O)
- Efficiente per dati sequenziali

### Test 2: TCP Locale

**Meccanismo:**
- Crea socket TCP server su localhost:12345
- Fork del processo per creare client e server
- Il client si connette e invia tutti i numeri
- Il server accetta la connessione e riceve i dati

**Caratteristiche:**
- Comunicazione bidirezionale
- Overhead del protocollo TCP
- Gestione errori robusta
- Simulazione comunicazione di rete

### Test 3: Memoria Condivisa

**Meccanismo:**
- Crea segmento di memoria condivisa con `shm_open()`
- Mappa la memoria in entrambi i processi con `mmap()`
- Il figlio scrive direttamente in memoria condivisa
- Il padre legge direttamente dalla stessa memoria

**Caratteristiche:**
- Accesso diretto alla memoria
- Massima velocità teorica
- Richiede sincronizzazione esplicita
- Zero-copy data transfer

## Funzionalità Implementate

### Misurazione Tempo
```c
long long get_time_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000 + tv.tv_usec;
}
```
Utilizza `gettimeofday()` per precisione al microsecondo.

### Generazione Dati
```c
void generate_numbers(int *numbers, int count) {
    int i;
    srand(42); /* Seed fisso per riproducibilità */
    for (i = 0; i < count; i++) {
        numbers[i] = rand();
    }
}
```
Seed fisso garantisce risultati riproducibili tra esecuzioni.

### Verifica Integrità
```c
int verify_numbers(int *original, int *received, int count) {
    int i;
    for (i = 0; i < count; i++) {
        if (original[i] != received[i]) {
            return 0; /* Errore */
        }
    }
    return 1; /* OK */
}
```
Confronta array originale e ricevuto per verificare correttezza.

### Gestione Errori
- Controllo return value di tutte le system call
- Cleanup automatico delle risorse allocate
- Messaggi di errore informativi con `perror()`

## Configurazione

### Parametri Modificabili
```c
#define NUM_COUNT 1000000    /* Numero di interi da trasferire */
#define TCP_PORT 12345       /* Porta TCP per test locale */
#define SHM_NAME "/ipc_test_shm"  /* Nome memoria condivisa */
#define CHUNK_SIZE 1024      /* Dimensione chunk (per future estensioni) */
```

### Requisiti Sistema
- Sistema operativo POSIX-compatibile (Linux, macOS, BSD)
- Supporto POSIX shared memory (`/dev/shm` su Linux)
- Libreria real-time (`-lrt`) per shared memory
- Minimo 16MB RAM libera (4MB * 4 copie dei dati)

## Risultati Attesi

**Ordine tipico di prestazioni (dal più veloce):**

1. **Memoria Condivisa** - Accesso diretto, zero overhead
2. **Unnamed Pipe** - Buffer kernel efficiente
3. **TCP Locale** - Overhead protocollo di rete

**Fattori che influenzano le prestazioni:**
- Dimensione buffer del kernel
- Carico del sistema
- Implementazione TCP stack
- Velocità della memoria

## Note di Compatibilità C90

Il codice rispetta rigorosamente lo standard C90:
- Tutte le dichiarazioni variabili all'inizio delle funzioni
- Solo commenti `/* */`
- Nessuna funzionalità C99/C11
- Compatibile con flag `-ansi`

## Troubleshooting

### Errore "Address already in use"
```bash
# Il processo precedente non ha rilasciato la porta TCP
# Attendere 30 secondi o cambiare TCP_PORT
```

### Errore "No space left on device" 
```bash
# Shared memory piena, cleanup manuale:
ls /dev/shm/
rm /dev/shm/ipc_test_shm
```

### Errore di compilazione "cannot find -lrt"
```bash
# Su sistemi più vecchi, prova senza -lrt:
gcc -ansi -Wall ipc_comparison.c -o ipc_comparison
```

## Estensioni Possibili

- Test con diverse dimensioni dati
- Misurazione throughput (MB/s)
- Test con multiple connessioni TCP simultanee
- Confronto con named pipe (FIFO)
- Test con message queue POSIX
- Analisi CPU usage durante trasferimento