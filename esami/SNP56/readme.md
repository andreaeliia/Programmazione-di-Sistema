# Profiling Prestazioni: Fork vs Thread

## Descrizione del Problema

Due programmi C che misurano e confrontano le prestazioni di esecuzione utilizzando approcci diversi:

1. **profile_fork.c**: Lancia 10 volte un processo figlio che esegue una funzione "dummy", misurando i tempi clock, system e user per ogni esecuzione e il totale

2. **profile_thread.c**: Esegue la stessa funzione "dummy" in sequenza utilizzando 10 thread, con le stesse misurazioni temporali

Entrambi i programmi permettono di confrontare l'overhead e le prestazioni dei due approcci di programmazione parallela.

## Struttura del Progetto

```
profile_fork.c     - Implementazione con processi fork
profile_thread.c   - Implementazione con thread POSIX
README.md         - Questa documentazione
```

## Compilazione

Entrambi i programmi sono compatibili con standard C90:

```bash
# Compila versione fork
gcc -ansi -Wall profile_fork.c -o profile_fork

# Compila versione thread  
gcc -ansi -Wall profile_thread.c -o profile_thread -lpthread

# Compilazione con debug
gcc -ansi -Wall -g profile_fork.c -o profile_fork
gcc -ansi -Wall -g profile_thread.c -o profile_thread -lpthread
```

## Esecuzione

```bash
# Esegui profiling con fork
./profile_fork

# Esegui profiling con thread
./profile_thread
```

Entrambi i programmi eseguono automaticamente tutte le misurazioni e mostrano:
- Risultati per ogni iterazione
- Statistiche riassuntive (totale, media, min, max)
- Analisi overhead
- Tempo totale del programma

## Output Esempio

### profile_fork.c
```
=== PROFILING PRESTAZIONI CON FORK ===
Esecuzioni programmate: 10
Dimensione lavoro dummy: 1000000 iterazioni
PID processo principale: 12345

--- Iterazione 1 ---
  Processo figlio PID 12346: Inizio esecuzione dummy
  Processo figlio PID 12346: Dummy completato (risultato: 1570796.33)
  Processo figlio terminato con codice 0
  Tempi iterazione 1:
    Clock time:  0.0156 s
    Wall time:   0.0234 s
    User time:   0.0198 s
    System time: 0.0012 s

...

=== ANALISI STATISTICA ===
┌─────────────┬────────┬────────┬────────┬────────┐
│ Metrica     │ Totale │ Media  │ Min    │ Max    │
├─────────────┼────────┼────────┼────────┼────────┤
│ Clock time  │  0.168 │  0.017 │  0.015 │  0.019 │
│ Wall time   │  0.245 │  0.025 │  0.023 │  0.028 │
│ User time   │  0.201 │  0.020 │  0.018 │  0.022 │
│ System time │  0.031 │  0.003 │  0.001 │  0.005 │
└─────────────┴────────┴────────┴────────┴────────┘

ANALISI OVERHEAD:
CPU totale utilizzata: 0.232 s (User: 0.201 + System: 0.031)
Tempo reale totale: 0.245 s
Overhead fork/wait: 0.013 s (5.3%)
```

### profile_thread.c
```
=== PROFILING PRESTAZIONI CON THREAD ===
Esecuzioni programmate: 10
Dimensione lavoro dummy: 1000000 iterazioni
PID processo principale: 12347
Modalità: Esecuzione sequenziale thread

--- Iterazione 1 ---
  Thread 1: Inizio esecuzione dummy
  Thread 1: Dummy completato (risultato: 1570796.33)
  Tempi thread 1:
    Clock time:  0.0187 s
    Wall time:   0.0201 s
    User time:   0.0195 s
    System time: 0.0003 s
  Thread 1 terminato con successo

...

=== ANALISI STATISTICA ===
┌─────────────┬────────┬────────┬────────┬────────┐
│ Metrica     │ Totale │ Media  │ Min    │ Max    │
├─────────────┼────────┼────────┼────────┼────────┤
│ Clock time  │  0.189 │  0.019 │  0.018 │  0.021 │
│ Wall time   │  0.203 │  0.020 │  0.019 │  0.022 │
│ User time   │  0.196 │  0.020 │  0.018 │  0.021 │
│ System time │  0.008 │  0.001 │  0.000 │  0.002 │
└─────────────┴────────┴────────┴────────┴────────┘

ANALISI OVERHEAD:
CPU totale utilizzata: 0.204 s (User: 0.196 + System: 0.008)
Tempo reale totale: 0.203 s
Overhead thread creation/join: -0.001 s (-0.5%)
```

## Dettagli Implementazione

### Funzione Dummy Comune

Entrambi i programmi utilizzano la stessa funzione dummy per garantire comparabilità:

```c
void dummy_function() {
    int i, j;
    volatile double result = 0.0;
    volatile int array[1000];
    
    /* Calcolo intensivo con operazioni floating point */
    for (i = 0; i < 1000000; i++) {
        result += (double)i * 3.14159 / (i + 1);
        
        /* Accesso memoria ogni 1000 iterazioni */
        if (i % 1000 == 0) {
            for (j = 0; j < 1000; j++) {
                array[j] = i + j;
            }
        }
        
        /* System call occasionale */
        if (i % 100000 == 0) {
            getpid();
        }
    }
}
```

**Caratteristiche del carico di lavoro:**
- Calcoli floating point intensivi
- Accesso frequente alla memoria
- System call periodiche per overhead kernel
- Risultato volatile per prevenire ottimizzazioni compiler

### Misurazioni Temporali

#### Tipi di Tempo Misurati

**Clock Time**
```c
clock_t start = clock();
/* ... lavoro ... */
clock_t end = clock();
double seconds = (double)(end - start) / CLOCKS_PER_SEC;
```
- Tempo CPU allocato al processo/thread
- Include context switching
- Può essere > wall time su sistemi multicore

**Wall Time (Real Time)**
```c
struct timespec start, end;
clock_gettime(CLOCK_REALTIME, &start);
/* ... lavoro ... */
clock_gettime(CLOCK_REALTIME, &end);
```
- Tempo reale trascorso (orologio da parete)
- Include attese I/O e context switching
- Sempre crescente linearmente

**User Time / System Time**
```c
struct rusage usage;
getrusage(RUSAGE_CHILDREN, &usage);  /* Fork version */
getrusage(RUSAGE_THREAD, &usage);    /* Thread version */
```
- **User time**: Tempo speso in user mode
- **System time**: Tempo speso in kernel mode
- Sommati danno il tempo CPU effettivo utilizzato

### Differenze di Implementazione

#### profile_fork.c
```c
pid_t pid = fork();
if (pid == 0) {
    /* Processo figlio */
    dummy_function();
    exit(EXIT_SUCCESS);
} else {
    /* Processo padre */
    waitpid(pid, &status, 0);
    getrusage(RUSAGE_CHILDREN, &usage);
}
```

**Caratteristiche:**
- Ogni iterazione crea un nuovo processo
- Spazio di indirizzamento separato
- Overhead di fork/exec/wait
- Isolamento completo tra esecuzioni
- Misurazioni tramite RUSAGE_CHILDREN

#### profile_thread.c
```c
pthread_create(&thread, NULL, thread_function, &data);
pthread_join(thread, NULL);
```

**Caratteristiche:**
- Ogni iterazione crea un nuovo thread
- Spazio di indirizzamento condiviso
- Overhead di creazione/distruzione thread
- Possibili interferenze cache/memoria
- Misurazioni tramite RUSAGE_THREAD (se disponibile)

## Analisi Risultati Attesi

### Overhead Teorico

**Fork (Processi)**
- **Pro**: Isolamento completo, no interferenze
- **Contro**: Overhead fork/exec elevato, memoria duplicata
- **System time**: Elevato per gestione processi

**Thread**
- **Pro**: Creazione veloce, memoria condivisa
- **Contro**: Possibili interferenze cache, sincronizzazione
- **System time**: Basso, principalmente per scheduling

### Confronto Prestazioni Tipiche

**Situazioni Favorevoli a Fork:**
- Carico di lavoro con molto I/O
- Necessità di isolamento
- Processi long-running

**Situazioni Favorevoli a Thread:**
- Carico computazionale puro
- Condivisione dati frequente
- Task di breve durata

## Configurazione e Parametri

### Parametri Modificabili
```c
#define NUM_ITERATIONS 10      /* Numero esecuzioni */
#define DUMMY_WORK_SIZE 1000000 /* Dimensione carico lavoro */
```

### Adattamento Carico di Lavoro
```c
/* Per sistemi più lenti */
#define DUMMY_WORK_SIZE 100000

/* Per sistemi più veloci */  
#define DUMMY_WORK_SIZE 10000000

/* Per test di overhead puro */
#define DUMMY_WORK_SIZE 1000
```

## Requisiti di Sistema

### Dipendenze C90
- **Standard C**: `stdlib.h`, `stdio.h`, `time.h`
- **POSIX**: `unistd.h`, `sys/times.h`, `sys/resource.h`
- **Thread**: `pthread.h` (solo profile_thread.c)

### Supporto Sistema Operativo
- **Linux**: Supporto completo per tutte le misurazioni
- **macOS**: Supporto completo, possibili differenze nei valori
- **Unix variants**: RUSAGE_THREAD potrebbe non essere disponibile

### Compilazione Avanzata
```bash
# Ottimizzazioni disabilitate per misurazioni accurate
gcc -ansi -Wall -O0 profile_fork.c -o profile_fork

# Debug con simboli
gcc -ansi -Wall -g -DDEBUG profile_thread.c -o profile_thread -lpthread

# Profiling con gprof
gcc -ansi -Wall -pg profile_fork.c -o profile_fork
./profile_fork
gprof profile_fork gmon.out > analysis.txt
```

## Interpretazione Risultati

### Metriche Chiave
- **Overhead Ratio**: (Wall time - CPU time) / Wall time
- **Efficiency**: User time / Wall time
- **Variabilità**: (Max time - Min time) / Average time

### Confronto Diretto
```bash
# Esegui entrambi e confronta
./profile_fork > results_fork.txt
./profile_thread > results_thread.txt
diff -u results_fork.txt results_thread.txt
```

### Red Flags
- **System time elevato**: Possibile problema risorse
- **Variabilità alta**: Sistema sotto carico
- **Wall time >> CPU time**: Contention o I/O wait

## Troubleshooting

### Problemi Comuni

**Compilation Error: "pthread.h not found"**
```bash
# Installa pthread development
sudo apt-get install libc6-dev  # Ubuntu/Debian
sudo yum install glibc-devel    # CentOS/RHEL
```

**Runtime Error: "RUSAGE_THREAD not defined"**
```c
/* Fallback per sistemi senza RUSAGE_THREAD */
#ifndef RUSAGE_THREAD
#define RUSAGE_THREAD RUSAGE_SELF
#endif
```

**Risultati Inconsistenti**
```bash
# Disabilita frequency scaling
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Aumenta priorità processo
nice -n -10 ./profile_fork
```

**Overhead Negativo**
- Normale per thread con carico leggero
- Clock time può essere impreciso su sistemi multicore
- Usare wall time per confronti

### Debug e Verifica

**Verifica Implementazione**
```c
/* Aggiungi per debug dettagliato */
#define DEBUG 1
#ifdef DEBUG
printf("DEBUG: PID=%d, Clock=%ld, Wall=%.6f\n", getpid(), clock(), wall_time);
#endif
```

**Test di Sanità**
```bash
# Verifica che i risultati siano ragionevoli
./profile_fork | grep "Clock time totale"
./profile_thread | grep "Clock time totale"

# I valori dovrebbero essere simili per CPU time
```

## Note di Compatibilità C90

### Conformità Standard
- Tutte le dichiarazioni variabili all'inizio delle funzioni
- Solo commenti `/* */`
- Nessuna funzionalità C99 (VLA, mixed declarations)
- Inizializzazione esplicita di tutte le strutture

### Portabilità
- Testato su Linux x86_64 e ARM
- Compatibile con gcc 4.x e versioni successive
- Richiede sistema POSIX per misurazioni temporali

## Estensioni Possibili

### Miglioramenti Analitici
- Calcolo deviazione standard
- Grafici prestazioni nel tempo
- Correlazione con carico sistema
- Profiling dettagliato per funzione

### Varianti Sperimentali
- Thread pool vs creazione dinamica
- Confronto con process pool
- Misurazioni cache miss
- Analisi scalabilità multicore

### Integrazione Tools
- Export dati per gnuplot
- Integrazione con perf
- Output formato JSON
- Dashboard web real-time