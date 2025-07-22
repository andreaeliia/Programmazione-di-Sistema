================================================================================
    CALCOLO PARALLELO DELLE MANTISSE CON OTTIMIZZAZIONE DELLE PRESTAZIONI
================================================================================

DESCRIZIONE GENERALE
====================

Questo progetto implementa un sistema di calcolo parallelo per determinare le 
mantisse dei numeri interi da 1 a 10.000.000, utilizzando una architettura 
multi-processo e multi-thread con comunicazione tramite pipe. L'obiettivo è 
sperimentare diverse configurazioni per trovare la combinazione ottimale di 
parametri che massimizza lo speed-up.

ARCHITETTURA DEL SISTEMA
========================

Il sistema è composto da due processi principali che comunicano tramite pipe:

PROCESSO PADRE (Calcolatore):
- Esegue N thread di calcolo in parallelo
- Ogni thread calcola le mantisse per un range specifico di numeri
- I risultati vengono distribuiti tra P pipe usando round-robin
- Sincronizzazione tramite mutex per accesso esclusivo alle pipe

PROCESSO FIGLIO (Raccoglitore):
- Esegue M thread di lettura in parallelo  
- Ogni thread legge da una delle P pipe disponibili
- I risultati vengono inseriti in una tabella condivisa
- Sincronizzazione tramite mutex per accesso esclusivo alla tabella

PARAMETRI DI SPERIMENTAZIONE
============================

N = Numero di thread di calcolo nel processo padre (1, 2, 4, 8)
M = Numero di thread di lettura nel processo figlio (1, 2, 4)
P = Numero di pipe per comunicazione inter-processo (1, 2, 4)

L'esperimento testa tutte le combinazioni possibili per determinare la 
configurazione che produce il massimo speed-up.

ALGORITMO DI CALCOLO MANTISSA
=============================

La mantissa di un numero intero viene calcolata utilizzando la funzione 
frexp() della libreria matematica standard:

    mantissa = frexp(numero, &esponente)

Questa funzione estrae la mantissa normalizzata (parte frazionaria) dalla 
rappresentazione in virgola mobile del numero secondo lo standard IEEE 754.

Esempio:
- Numero: 1024
- Rappresentazione: 1024 = 0.5 × 2^11  
- Mantissa: 0.5

FILES DEL PROGETTO
==================

mantissa_calculator.c  - Implementazione principale con threading e pipe
Makefile              - Script di compilazione multi-piattaforma
README.txt            - Questa documentazione

REQUISITI DI SISTEMA
====================

- Compilatore: GCC con supporto ANSI C (C90)
- Sistema: Linux o Mac OS
- Librerie: 
  * apue.h (Advanced Programming in UNIX Environment)
  * pthread (POSIX Threads)
  * libm (Math Library)
- RAM: Almeno 512 MB per gestire la tabella risultati
- CPU: Multi-core raccomandato per sfruttare il parallelismo

COMPILAZIONE
============

Il progetto usa un Makefile compatibile con Linux e Mac OS.

Compilazione standard:
    make

Compilazione con informazioni di debug:
    make info

Test del programma:
    make test

Benchmark completo:
    make benchmark

Controllo memoria (se valgrind disponibile):
    make memcheck

Compilazione per profiling:
    make profile

Pulizia file compilati:
    make clean

ESECUZIONE
==========

Esecuzione standard:
    ./mantissa_calculator

Il programma eseguirà automaticamente tutti gli esperimenti con le diverse
configurazioni di N, M, e P, stampando:

1. Log di avvio e completamento dei thread
2. Tempo di esecuzione per ogni configurazione
3. Calcolo dello speed-up rispetto alla configurazione baseline
4. Tabella riassuntiva dei risultati
5. Identificazione della configurazione ottimale

OUTPUT TIPICO
=============

=== CALCOLO PARALLELO MANTISSE ===
Range numeri: 1 - 10000000
Avvio esperimenti per determinare configurazione ottimale...

=== ESPERIMENTO: N=1, M=1, P=1 ===
Thread calcolo 0: numeri 1-10000000
Thread lettura 0 avviato
...
Tempo di esecuzione: 15.234 secondi

=== RISULTATI ESPERIMENTI ===
N    M    P    Tempo(s)   Speedup
----------------------------------------
1    1    1    15.234     1.00
2    1    1    8.456      1.80
4    2    2    4.123      3.69
...

=== CONFIGURAZIONE OTTIMALE ===
N (thread calcolo): 4
M (thread lettura): 2  
P (pipe): 2
Tempo: 4.123 secondi
Speedup: 3.69x

ANALISI DELLE PRESTAZIONI
==========================

FATTORI CHE INFLUENZANO LO SPEED-UP:

1. NUMERO DI THREAD DI CALCOLO (N):
   - Più thread = maggiore parallelismo nel calcolo
   - Limitato dal numero di core CPU disponibili
   - Overhead di sincronizzazione per valori troppo alti

2. NUMERO DI THREAD DI LETTURA (M):  
   - Più thread = maggiore parallelismo nella lettura pipe
   - Limitato dalla velocità di I/O del sistema
   - Contesa per accesso alla tabella condivisa

3. NUMERO DI PIPE (P):
   - Più pipe = riduzione della contesa tra thread di scrittura
   - Maggiore parallelismo nella comunicazione inter-processo
   - Overhead di gestione per valori eccessivi

CONFIGURAZIONI TIPICHE OTTIMALI:
- CPU dual-core: N=2, M=1, P=2
- CPU quad-core: N=4, M=2, P=2  
- CPU octa-core: N=8, M=2, P=4

IMPLEMENTAZIONE THREADING
=========================

SINCRONIZZAZIONE:
- Mutex per accesso esclusivo alle pipe (per thread di scrittura)
- Mutex per accesso esclusivo alla tabella (per thread di lettura)
- Join per attesa terminazione thread

DISTRIBUZIONE DEL CARICO:
- Round-robin per assegnazione pipe ai thread di calcolo
- Range numerici equamente distribuiti tra thread di calcolo
- Terminazione graceful tramite chiusura pipe

GESTIONE MEMORIA:
- Allocazione dinamica per tabella risultati (80 MB circa)
- Allocazione dinamica per array di mutex
- Pulizia automatica alla terminazione

COMUNICAZIONE INTER-PROCESSO
============================

PIPE ANONIME:
- Comunicazione unidirezionale padre -> figlio
- Buffer interno del kernel per gestire I/O asincrono
- Chiusura pipe per segnalare fine trasmissione

PROTOCOLLO DATI:
- Struttura mantissa_result: {numero, mantissa}
- Dimensione fissa per letture/scritture atomiche
- Controllo errori su partial read/write

GESTIONE ERRORI
===============

Il programma include gestione completa degli errori:

- Controllo fallimenti fork() per creazione processo figlio
- Controllo errori pthread_create() per creazione thread
- Controllo errori pipe() per creazione pipe
- Controllo errori I/O su read/write delle pipe
- Controllo errori allocazione memoria dinamica

In caso di errore:
1. Stampa messaggio descrittivo utilizzando funzioni apue.h
2. Esegue pulizia delle risorse allocate (thread, pipe, memoria)
3. Termina con codice di uscita appropriato

LIMITAZIONI
===========

1. Range fisso di numeri (1-10.000.000) per test riproducibili
2. Numero massimo di thread limitato a 32 per configurazione
3. Numero massimo di pipe limitato a 16
4. Tabella risultati mantenuta in memoria (non persistente)
5. Nessuna ottimizzazione CPU-specific (SSE, AVX)

OTTIMIZZAZIONI POSSIBILI
========================

PRESTAZIONI:
- Utilizzo di SIMD per calcoli vettoriali
- Memory pool per ridurre allocazioni dinamiche
- Lock-free data structures per ridurre contesa
- NUMA-aware memory allocation per sistemi multi-socket

SCALABILITA':
- Suddivisione automatica in base al numero di core
- Load balancing dinamico tra thread
- Implementazione con memoria condivisa per dataset molto grandi
- Distribuzione su più macchine (MPI)

ARCHITETTURA:
- Implementazione con thread pool per evitare overhead creazione
- Pipeline multi-stage per overlap calcolo/comunicazione
- Compressione dati per ridurre traffico pipe
- Checkpointing per recovery da errori

BENCHMARK E PROFILING
=====================

Per analizzare le prestazioni in dettaglio:

1. PROFILING CPU:
   make profile
   ./mantissa_calculator_prof
   gprof mantissa_calculator_prof gmon.out > profile.txt

2. MEMORY PROFILING:
   make memcheck

3. SYSTEM MONITORING:
   htop o top durante esecuzione per monitorare utilizzo CPU/memoria
   iostat per monitorare I/O del sistema

VARIAZIONI SPERIMENTALI
=======================

Per test personalizzati, modificare le costanti nel codice:

#define MAX_NUMBERS 10000000  // Cambiare range numeri
#define MAX_THREADS 32        // Aumentare limite thread  
#define MAX_PIPES 16          // Aumentare limite pipe

Ricompilare dopo le modifiche:
    make clean && make

TROUBLESHOOTING
===============

Problema: "pthread_create failed"
Soluzione: Ridurre numero thread o aumentare limite sistema (ulimit -u)

Problema: "pipe creation error: Too many open files"  
Soluzione: Aumentare limite file descriptor (ulimit -n)

Problema: Prestazioni scarse su multi-core
Soluzione: Verificare CPU affinity e NUMA topology

Problema: Segmentation fault
Soluzione: Verificare con valgrind, controllare accesso array

Problema: Deadlock apparente
Soluzione: Controllare ordine acquisizione mutex, verificare terminazione thread

CONFORMITA' STANDARD
=====================

Il codice è conforme a:
- Standard ANSI C (C90/C89)
- POSIX.1 per threading (pthread)
- POSIX.1 per pipe e system call
- IEEE 754 per rappresentazione floating point

BIBLIOGRAFIA E RIFERIMENTI
==========================

- "Advanced Programming in the UNIX Environment" - W. Richard Stevens
- "Programming with POSIX Threads" - David R. Butenhof  
- "The Art of Multiprocessor Programming" - Maurice Herlihy
- POSIX.1-2008 Standard Documentation
- IEEE 754-2008 Floating Point Standard

================================================================================
                              FINE DOCUMENTAZIONE  
================================================================================