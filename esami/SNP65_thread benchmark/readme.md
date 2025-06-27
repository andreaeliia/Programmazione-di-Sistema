# Esperimento Performance Threading

## Descrizione

Questo programma implementa un esperimento per verificare se l'uso di multiple thread in un processo porta vantaggi rispetto all'uso di una singola thread su architetture multi-core.

### Obiettivo dell'Esperimento

L'esperimento confronta le performance di calcolo tra due scenari:
- **Scenario 1**: 1 thread worker esegue moltiplicazioni per un tempo prefissato
- **Scenario 2**: 3 thread worker eseguono moltiplicazioni contemporaneamente per lo stesso tempo

### Implementazione della Traccia

Il programma rispetta fedelmente la traccia assegnata:

1. **Thread di misura dedicata**: Controlla i tempi e gestisce l'esperimento
2. **Thread worker**: Eseguono moltiplicazioni tra numeri casuali
3. **Area di memoria condivisa**: Struttura `ExperimentData` per risultati
4. **Misurazione accurata**: Conta le operazioni completate in ogni scenario

---

## Architettura del Programma

### Strutture Dati

```c
typedef struct {
    long long scenario1_count;      // Contatore operazioni scenario 1
    long long scenario2_count;      // Contatore operazioni scenario 2
    volatile int running_scenario1; // Flag controllo scenario 1
    volatile int running_scenario2; // Flag controllo scenario 2
    pthread_mutex_t counter_mutex;  // Mutex per accesso thread-safe
    pthread_cond_t start_condition; // Condition variable per sincronizzazione
    int duration;                   // Durata test in secondi
} ExperimentData;
```

### Thread Utilizzati

1. **Main Thread**: Coordina l'esperimento e gestisce i worker
2. **Measurement Thread**: Controlla i tempi e gestisce start/stop
3. **Worker Thread(s)**: Eseguono le moltiplicazioni (1 o 3 a seconda dello scenario)

### Flusso di Esecuzione

```
Main Thread
├── Crea Measurement Thread
├── Scenario 1:
│   ├── Crea 1 Worker Thread
│   ├── Measurement Thread controlla 5 secondi
│   └── Raccoglie risultati
├── Scenario 2:
│   ├── Crea 3 Worker Thread
│   ├── Measurement Thread controlla 5 secondi
│   └── Raccoglie risultati
└── Analizza e confronta i risultati
```

---

## Compilazione ed Esecuzione

### Requisiti
- Compilatore GCC con supporto pthread
- Sistema operativo POSIX-compatible (Linux, macOS)
- Libreria pthread

### Compilazione
```bash
gcc -o thread_experiment thread_experiment.c -lpthread
```

### Esecuzione
```bash
./thread_experiment
```

### Opzioni di Compilazione Avanzate
```bash
# Versione ottimizzata
gcc -O2 -o thread_experiment thread_experiment.c -lpthread

# Versione debug
gcc -g -DDEBUG -o thread_experiment thread_experiment.c -lpthread

# Con warning estesi
gcc -Wall -Wextra -o thread_experiment thread_experiment.c -lpthread
```

---

## Output Atteso

### Esempio di Esecuzione Tipica

```
=== ESPERIMENTO PERFORMANCE THREADING ===
Durata ogni test: 5 secondi
Numero thread Scenario 2: 3

Thread di Misura: avviato

=== SCENARIO 1: Single Thread ===
Worker Thread 1 (Scenario 1): avviato
Scenario 1 completato in 5.003 secondi
Moltiplicazioni eseguite: 18425000
Throughput: 3682 moltiplicazioni/secondo
Worker Thread 1 (Scenario 1): terminato

=== SCENARIO 2: 3 Threads ===
Worker Thread 1 (Scenario 2): avviato
Worker Thread 2 (Scenario 2): avviato
Worker Thread 3 (Scenario 2): avviato
Scenario 2 completato in 5.001 secondi
Moltiplicazioni eseguite: 51240000
Throughput: 10245 moltiplicazioni/secondo
Worker Thread 1 (Scenario 2): terminato
Worker Thread 2 (Scenario 2): terminato
Worker Thread 3 (Scenario 2): terminato

=== ANALISI RISULTATI ===
Speedup: 2.78x
Efficienza: 92.67% (2.78/3 threads)
RISULTATO: Il multithreading porta vantaggi significativi

Esperimento completato.
```

### Interpretazione Risultati

**Speedup**: Rapporto tra performance multi-thread e single-thread
- `Speedup = Operazioni_Scenario2 / Operazioni_Scenario1`

**Efficienza**: Percentuale di utilizzo ideale dei thread
- `Efficienza = Speedup / Numero_Thread`

**Valori Tipici Attesi**:
- **CPU Single-Core**: Speedup ≈ 1.0x (nessun vantaggio)
- **CPU Dual-Core**: Speedup ≈ 1.8x (efficienza ~60%)
- **CPU Quad-Core+**: Speedup ≈ 2.5-2.8x (efficienza ~85-95%)

---

## Configurazione

### Parametri Modificabili

```c
#define MEASUREMENT_DURATION 5  // Durata test (secondi)
#define NUM_THREADS_SCENARIO2 3 // Numero thread scenario 2
```

### Personalizzazioni Possibili

**Durata test diversa**:
```c
#define MEASUREMENT_DURATION 10  // Test di 10 secondi
```

**Numero thread diverso**:
```c
#define NUM_THREADS_SCENARIO2 4  // Confronto 1 vs 4 thread
```

**Algoritmo di calcolo diverso**:
```c
// Nel worker_thread(), sostituire:
volatile int result = a * b;
// Con:
volatile double result = sin(a) * cos(b);  // Operazioni trigonometriche
// O:
volatile long long result = fibonacci(a % 30);  // Calcolo ricorsivo
```

---

## Caratteristiche Tecniche

### Sincronizzazione
- **Mutex**: Protezione accesso ai contatori condivisi
- **Condition Variables**: Coordinamento start/stop tra thread
- **Volatile**: Prevenzione ottimizzazioni compilatore sui flag

### Ottimizzazioni Performance
- **Batch Updates**: Aggiornamento contatori ogni 1000 operazioni
- **Local Counting**: Riduzione accessi al mutex
- **Thread-Safe Random**: Seed unici per ogni thread

### Misurazione Accurata
- **Clock Monotonic**: Timer ad alta risoluzione
- **Timing Preciso**: Misurazione start/stop accurata
- **Overhead Minimizzato**: Sincronizzazione ottimizzata

---

## Troubleshooting

### Problemi Comuni

**Speedup negativo o molto basso**:
- Possibile CPU single-core o overhead eccessivo
- Verificare con `nproc` il numero di core disponibili

**Risultati inconsistenti**:
- Altri processi che consumano CPU
- Eseguire con priorità alta: `nice -n -10 ./thread_experiment`

**Errori di compilazione**:
```bash
# Se manca pthread.h
sudo apt-get install libc6-dev

# Se problemi con linking
gcc -o thread_experiment thread_experiment.c -lpthread -lrt
```

**Performance troppo basse**:
```bash
# Compilazione ottimizzata
gcc -O3 -march=native -o thread_experiment thread_experiment.c -lpthread
```

### Debugging

**Versione debug**:
```bash
gcc -g -DDEBUG -o thread_experiment_debug thread_experiment.c -lpthread
gdb ./thread_experiment_debug
```

**Profilazione**:
```bash
gcc -pg -o thread_experiment_profile thread_experiment.c -lpthread
./thread_experiment_profile
gprof thread_experiment_profile gmon.out > analysis.txt
```

---

## Estensioni Possibili

### Miglioramenti Semplici
1. **Multiple runs**: Eseguire test multipli e calcolare media
2. **CPU affinity**: Legare thread a core specifici
3. **Timing variabile**: Test con durate diverse

### Miglioramenti Avanzati
1. **Algoritmi diversi**: Test con operazioni I/O-bound
2. **Monitoraggio real-time**: Thread che stampa progress
3. **Statistiche dettagliate**: Deviazione standard, percentili

### Esempio Estensione - Multiple Runs
```c
// Aggiungere nel main():
#define NUM_RUNS 5
long long results_s1[NUM_RUNS], results_s2[NUM_RUNS];

for (int run = 0; run < NUM_RUNS; run++) {
    printf("=== RUN %d/%d ===\n", run + 1, NUM_RUNS);
    // ... eseguire esperimenti
    results_s1[run] = experiment.scenario1_count;
    results_s2[run] = experiment.scenario2_count;
}

// Calcolare media e deviazione standard
```

---

## File Correlati

- `thread_experiment.c` - Codice sorgente principale
- `Makefile` - Script di compilazione (opzionale)
- `results.txt` - Log dei risultati (generato durante esecuzione)

### Makefile Esempio
```makefile
CC=gcc
CFLAGS=-Wall -Wextra -O2
LIBS=-lpthread

thread_experiment: thread_experiment.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

clean:
	rm -f thread_experiment

.PHONY: clean
```

---

## Note per l'Esame

### Punti Chiave da Ricordare
1. **Thread di misura separata** - Implementa specificamente la richiesta della traccia
2. **Sincronizzazione corretta** - Mutex + condition variables
3. **Misurazione accurata** - Timer precisi e gestione overhead
4. **Analisi completa** - Speedup ed efficienza

### Possibili Varianti della Traccia
- Numero diverso di thread worker
- Algoritmi di calcolo diversi
- Durata di test variabile
- Più scenari di confronto

Questo codice fornisce una base solida per rispondere a tracce simili modificando solo i parametri necessari.