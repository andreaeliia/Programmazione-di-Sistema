===============================================================================
                    CONCURRENT PROCESSES CON RECORD LOCKING
===============================================================================

DESCRIZIONE
-----------
Questo programma implementa un sistema di tre processi concorrenti (A, B, C)
che scrivono continuamente record in una memoria condivisa. La sincronizzazione
è gestita tramite record locking su file per evitare condizioni di corsa.

Ogni processo aggiunge record contenenti:
1. PID del processo
2. Numero intero casuale (0-999)
3. Timestamp dell'istante di scrittura

CARATTERISTICHE
---------------
- Tre processi figli concorrenti
- Scrittura ogni 3 secondi in memoria condivisa System V IPC
- Sincronizzazione con record locking (fcntl)
- Gestione terminazione con SIGINT (Ctrl-C)
- Stampa finale di tutti i record scritti
- Compatibilità Linux e macOS
- Codice conforme C90 con libreria APUE

ARCHITETTURA
------------
Il programma è strutturato con:

- Processo padre: Coordina i figli e gestisce la terminazione
- Tre processi figli (A, B, C): Scrivono record continuamente
- Memoria condivisa: Contiene array di record con header
- File di lock: /tmp/record_lock.tmp per sincronizzazione

STRUTTURE DATI
--------------
struct shared_record {
    pid_t pid;           // PID del processo che ha scritto
    int random_number;   // Numero casuale 0-999
    time_t timestamp;    // Momento della scrittura
};

struct shared_memory {
    int record_count;                    // Contatore record totali
    struct shared_record records[1000];  // Array dei record
};

COMPILAZIONE
------------
Per compilare il programma:

    make

Per pulire i file binari e temporanei:

    make clean

UTILIZZO
--------
Eseguire il programma:

    ./concurrent_writer

Output esempio:
    === CONCURRENT PROCESSES CON RECORD LOCKING ===
    Avvio di 3 processi concorrenti...
    Premere Ctrl-C per terminare e vedere i risultati.

    Processo A (PID: 12345) avviato
    Processo B (PID: 12346) avviato  
    Processo C (PID: 12347) avviato
    Processo padre in attesa... (PID: 12344)
    
    Processo A: scritto record (PID=12345, NUM=123, TIME=1640995200)
    Processo B: scritto record (PID=12346, NUM=456, TIME=1640995203)
    ...

Terminazione con Ctrl-C:
    ^C
    Segnale di terminazione ricevuto. Terminazione in corso...
    Processo A (PID: 12345) terminato
    ...

    === CONTENUTO MEMORIA CONDIVISA ===
    Numero totale di record: 15

    Indice PID      Numero       Timestamp
    ------ ---      ------       ---------
    1      12345    123          Mon Jan 01 12:00:00 2024
    2      12346    456          Mon Jan 01 12:00:03 2024
    ...

MECCANISMO DI SINCRONIZZAZIONE
------------------------------
Il programma utilizza record locking POSIX per garantire accesso esclusivo
alla memoria condivisa:

1. Ogni processo apre /tmp/record_lock.tmp
2. Prima di scrivere: acquisisce lock esclusivo (F_WRLCK)
3. Scrive il record in memoria condivisa
4. Rilascia il lock (F_UNLCK)
5. Attende 3 secondi prima della prossima scrittura

Questo meccanismo previene:
- Scritture simultanee corrotte
- Inconsistenza del contatore record_count
- Perdita di dati per condizioni di corsa

GESTIONE SEGNALI
----------------
Il programma gestisce SIGINT (Ctrl-C):

1. Il processo padre riceve SIGINT
2. Imposta flag di terminazione should_terminate
3. Invia SIGTERM a tutti i processi figli
4. Attende la terminazione di tutti i figli
5. Stampa il contenuto finale della memoria condivisa
6. Pulisce tutte le risorse

FUNZIONI PRINCIPALI
-------------------
- main(): Processo padre, crea figli e coordina terminazione
- child_process_main(): Loop principale dei processi figli
- setup_signals(): Configura gestione SIGINT
- create_shared_memory(): Crea e inizializza memoria condivisa
- create_lock_file(): Crea file per record locking
- acquire_record_lock(): Acquisisce lock esclusivo
- release_record_lock(): Rilascia lock
- add_record_to_shared_memory(): Aggiunge record (sezione critica)
- print_shared_memory_contents(): Stampa risultati finali
- cleanup_resources(): Pulisce memoria condivisa e file temporanei

GESTIONE ERRORI
---------------
Il programma utilizza le funzioni APUE per error handling:
- err_sys(): Per errori fatali che terminano il programma
- err_ret(): Per errori non fatali con continuazione

Tutti gli errori di sistema (fork, shmget, fcntl, ecc.) sono
gestiti appropriatamente.

CARATTERISTICHE TECNICHE
------------------------
- Memoria condivisa: System V IPC con chiave 0x2345
- Record locking: POSIX fcntl() con F_SETLKW/F_SETLK
- Generazione casuali: srand() basato su PID + time
- Intervallo scrittura: 3 secondi fisso
- Capacità massima: 1000 record
- File temporaneo: /tmp/record_lock.tmp

LIMITAZIONI
-----------
- Numero fisso di 3 processi (A, B, C)
- Capacità massima 1000 record
- Intervallo scrittura fisso di 3 secondi  
- Chiave memoria condivisa fissa (0x2345)
- File di lock in posizione fissa (/tmp)

COMPATIBILITÀ
-------------
Testato su:
- GNU/Linux (Ubuntu, CentOS, Debian)
- macOS (10.15+)
- Compilatori GCC compatibili C90

Il codice è portabile tra sistemi POSIX/Unix-like.

ESEMPI DI OUTPUT
----------------
Durante l'esecuzione:
    Processo A: scritto record (PID=1234, NUM=789, TIME=1640995200)
    Processo C: scritto record (PID=1236, NUM=111, TIME=1640995201)  
    Processo B: scritto record (PID=1235, NUM=333, TIME=1640995202)

Risultato finale dopo Ctrl-C:
    === CONTENUTO MEMORIA CONDIVISA ===
    Numero totale di record: 12

    Indice PID      Numero       Timestamp
    ------ ---      ------       ---------
    1      1234     789          Mon Jan 01 12:00:00 2024
    2      1236     111          Mon Jan 01 12:00:01 2024
    3      1235     333          Mon Jan 01 12:00:02 2024
    ...

PULIZIA AUTOMATICA
------------------
Il programma pulisce automaticamente:
- Memoria condivisa System V (shmctl IPC_RMID)
- File di lock temporaneo (/tmp/record_lock.tmp)
- Terminazione ordinata di tutti i processi figli

Anche in caso di terminazione anomala, le risorse vengono liberate.

DIPENDENZE
----------
- Libreria APUE (apue.h)
- System V IPC (sys/shm.h, sys/ipc.h)  
- POSIX file locking (fcntl.h)
- Signal handling (signal.h)
- Funzioni standard C90

===============================================================================