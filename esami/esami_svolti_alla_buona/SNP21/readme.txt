================================================================================
                    BENCHMARK IPC: MEMORY MAPPING vs PIPE
================================================================================

DESCRIZIONE
-----------
Questo programma confronta le prestazioni di diverse tecniche di comunicazione
inter-processo (IPC) misurando la velocità di trasferimento dati tra processo
padre e figlio.

Tecniche IPC testate:
• Memory Mapping Anonymous Shared (mmap con MAP_SHARED | MAP_ANONYMOUS)
• Memory Mapping File-Backed (mmap su file temporaneo)  
• Pipe Anonime (pipe())
• Named Pipe/FIFO (mkfifo)
• Socket Pair UNIX Domain (socketpair)

ARCHITETTURA
------------
Il programma crea un processo figlio che comunica con il padre utilizzando
ciascuna tecnica IPC. Per ogni test:

1. Il processo padre prepara dati di test (1MB)
2. Viene misurato il tempo di inizio
3. Il padre trasmette i dati al figlio per 100 iterazioni
4. Il figlio verifica l'integrità dei dati ricevuti
5. Viene misurato il tempo di fine e calcolato il throughput

REQUISITI SISTEMA
-----------------
• Sistema operativo: Linux o macOS
• Compilatore: GCC con supporto ANSI C (C90)
• Libreria APUE (Advanced Programming in UNIX Environment)
• Memoria disponibile: almeno 100MB per i test

STRUTTURA FILE
--------------
ipc_benchmark.c    - Codice sorgente principale
Makefile          - Script di compilazione cross-platform
README.txt        - Questo file di documentazione

COMPILAZIONE
------------
Il Makefile detecta automaticamente il sistema operativo e usa i flag
appropriati:

    make all          # Compila il programma
    make test         # Compila ed esegue il benchmark
    make clean        # Rimuove binari e file temporanei
    make help         # Mostra comandi disponibili

Compilazione manuale:
    gcc -ansi -Wall -I./include -DLINUX -D_GNU_SOURCE \
        ipc_benchmark.c -o ipc_benchmark -L./lib -lapueLinux

ESECUZIONE
----------
    ./ipc_benchmark

Il programma eseguirà automaticamente tutti i test e mostrerà i risultati
in formato tabellare con tempi di esecuzione e throughput.

OUTPUT ESEMPIO
--------------
=== BENCHMARK IPC: MEMORY MAPPING vs PIPE ===
Testing Memory Mapping (Anonymous Shared)...
Testing Memory Mapping (File-Backed)...
Testing Pipe (Anonymous)...
Testing Named Pipe (FIFO)...
Testing Socket Pair (UNIX Domain)...

=== RISULTATI BENCHMARK IPC ===
Test Data Size: 1.00 MB
Iterations: 100
Total Data Transferred per Test: 100.00 MB

Method                         Time (ms)   Throughput (KB/s)
------                         ---------   ----------------
Memory Mapping (Anonymous)        125.34         819200.50
Memory Mapping (File-Backed)      156.78         654321.10
Pipe (Anonymous)                   234.56         437890.25
Named Pipe (FIFO)                 267.89         383456.75
Socket Pair (UNIX Domain)         289.12         355432.18

INTERPRETAZIONE RISULTATI
-------------------------
• Memory Mapping generalmente offre prestazioni superiori per trasferimenti
  di grandi quantità di dati
• Le pipe anonime sono più veloci delle named pipe per overhead minore
• I socket pair hanno overhead maggiore ma offrono più flessibilità
• Le prestazioni variano significativamente tra Linux e macOS

DETTAGLI IMPLEMENTAZIONE
------------------------

Memory Mapping Anonymous:
- Usa mmap() con MAP_SHARED | MAP_ANONYMOUS
- Sincronizzazione tramite polling su byte speciali
- Zero overhead di I/O, accesso diretto alla memoria

Memory Mapping File-Backed:
- Crea file temporaneo e lo mappa in memoria
- Persiste su disco (overhead aggiuntivo)
- Utile per condivisione dati persistenti

Pipe Anonime:
- Usa pipe() per creare descrittori di file
- Comunicazione unidirezionale tramite buffer kernel
- Sincronizzazione automatica (blocking I/O)

Named Pipe (FIFO):
- Crea pipe con nome nel filesystem
- Permette comunicazione tra processi non imparentati
- Overhead di creazione file system

Socket Pair:
- Usa socketpair() per socket UNIX domain
- Comunicazione bidirezionale
- Overhead di protocollo socket

SINCRONIZZAZIONE
----------------
Per garantire misurazioni accurate:
• Memory mapping usa polling su byte di controllo
• Pipe/socket sfruttano il blocking I/O automatico
• Verifica integrità dati con pattern predefiniti
• Gestione cleanup automatica file temporanei

LIMITAZIONI
-----------
• Test single-threaded (un solo processo figlio)
• Dimensione dati fissa (1MB per iterazione)
• Numero iterazioni fisso (100)
• Sincronizzazione semplificata per memory mapping

PERSONALIZZAZIONE
-----------------
Per modificare i parametri di test, editare le costanti in ipc_benchmark.c:

#define TEST_DATA_SIZE (1024 * 1024)  /* Dimensione dati per test */
#define NUM_ITERATIONS 100             /* Numero iterazioni */

TROUBLESHOOTING
---------------

Errore "Permission denied":
- Verificare permessi directory /tmp
- Eseguire con: sudo ./ipc_benchmark

Errore "mmap failed":
- Sistema con memoria insufficiente
- Ridurre TEST_DATA_SIZE

Errore compilazione libreria APUE:
- Verificare presenza libreria in ./lib/
- Controllare header files in ./include/

Prestazioni inaspettate:
- Sistema sotto carico (chiudere altre applicazioni)
- Filesystem montato con opzioni particolari
- Differenze tra Linux e macOS

RIFERIMENTI
-----------
• Stevens, W. Richard. "Advanced Programming in the UNIX Environment"
• POSIX.1-2008 Standard
• Linux man pages: mmap(2), pipe(2), mkfifo(3), socketpair(2)

AUTORE
------
Implementazione basata su esempi APUE
Compatibilità cross-platform Linux/macOS

================================================================================