===============================================================================
                    TRASFERIMENTO FILE TRA PROCESSI CON THREAD
===============================================================================

DESCRIZIONE
-----------
Implementazione di un sistema di trasferimento file tra due processi:
- Processo A: contiene 3 thread che inviano ciascuno un file di 100KB
- Processo B: riceve i dati e li salva in file separati

Il trasferimento avviene tramite:
- Area di memoria condivisa di 20 byte (2 byte sigla + 18 byte dati)
- Pipe per segnali di sincronizzazione
- Meccanismo di turnazione equa tra thread

ARCHITETTURA
------------
processo_a.c:
- Crea 3 thread, ognuno con sigla identificativa (T1, T2, T3)
- Ogni thread trasferisce un file di 100KB
- Utilizza mutex e condition variable per turnazione equa
- Semaforo per accesso esclusivo alla memoria condivisa

processo_b.c:
- Legge sequenze dalla memoria condivisa
- Identifica il thread mittente tramite sigla
- Salva i dati in file separati
- Invia segnali di conferma tramite pipe

COMPILAZIONE
------------
make all        # Compila entrambi i processi
make clean      # Rimuove binari e file temporanei
make test       # Mostra istruzioni per il test

ESECUZIONE
----------
1. Aprire due terminali

2. Nel primo terminale:
   ./processo_a

3. Annotare il file descriptor della pipe mostrato dal processo A

4. Nel secondo terminale:
   ./processo_b

5. Inserire il file descriptor quando richiesto

FILE GENERATI
-------------
Durante l'esecuzione vengono creati:

Input (processo A):
- file_0.dat, file_1.dat, file_2.dat (100KB ciascuno)

Output (processo B):
- received_from_thread_0.dat
- received_from_thread_1.dat  
- received_from_thread_2.dat

MECCANISMI DI SINCRONIZZAZIONE
------------------------------
1. Mutex + Condition Variable: per turnazione equa tra thread
2. Semaforo POSIX: per accesso esclusivo alla memoria condivisa
3. Pipe: per segnali di conferma dal processo B
4. Memoria condivisa POSIX: per trasferimento dati

STRUTTURA DATI CONDIVISI
------------------------
Memoria condivisa (20 byte):
[2 byte sigla][18 byte dati]

Sigla thread:
- Thread 0: "T1"
- Thread 1: "T2" 
- Thread 2: "T3"

CARATTERISTICHE IMPLEMENTAZIONE
-------------------------------
- Standard C90 (ANSI C)
- Compatibile Linux e macOS
- Gestione errori con funzioni apue.h
- Creazione automatica file di test
- Statistiche di trasferimento
- Cleanup risorse alla terminazione

LIMITAZIONI
-----------
- File di dimensione fissa (100KB)
- Area condivisa fissa (20 byte)
- Numero fisso di thread (3)
- Terminazione manuale processo B (Ctrl+C)

DEBUGGING
---------
I processi mostrano informazioni dettagliate durante l'esecuzione:
- Creazione e stato dei thread
- Byte trasferiti per ogni sequenza
- Statistiche periodiche
- Identificazione mittente per ogni sequenza

NOTE TECNICHE
-------------
- La memoria condivisa usa shm_open/mmap POSIX
- I semafori usano sem_open POSIX
- Le pipe sono anonime (pipe system call)
- La turnazione usa il pattern producer/consumer
- Gestione portabile tra Linux (-lrt) e macOS

TESTING
-------
Per verificare la correttezza:
1. Controllare che i file di output abbiano dimensione ~100KB
2. Verificare che ogni thread riceva dati in modo equo
3. Confrontare contenuto file originali vs ricevuti (opzionale)

===============================================================================