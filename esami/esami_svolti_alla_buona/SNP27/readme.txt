================================================================================
                    SISTEMA IPC CON CODA CIRCOLARE THREAD-SAFE
                        Pacchetti a Lunghezza Variabile
================================================================================

DESCRIZIONE
-----------
Sistema di comunicazione inter-processo che implementa il trasferimento di
pacchetti a lunghezza variabile tramite socket UNIX domain e gestisce la
ricezione con una coda circolare thread-safe gestita da thread multiple.

Componenti del sistema:
• Processo SENDER: genera e invia pacchetti con contenuto casuale
• Processo RECEIVER: riceve pacchetti e li gestisce con 3 thread
• Protocollo pacchetti: header lunghezza + payload variabile
• Coda circolare FIFO thread-safe con capacità limitata
• Sincronizzazione con mutex e condition variables POSIX

ARCHITETTURA SISTEMA
--------------------

PROCESSO SENDER (packet_sender):
├── Generazione Pacchetti
│   ├── Header con lunghezza totale (4 byte int)
│   ├── Payload casuale di dimensione variabile
│   ├── Range dimensioni: 16 - 4096 byte
│   └── Contenuto completamente randomico
│
├── Trasmissione Socket
│   ├── Socket UNIX domain (/tmp/packet_socket)
│   ├── Connessione TCP-like affidabile
│   ├── Invio pacchetto completo [header + data]
│   └── Controllo delay configurabile tra pacchetti

PROCESSO RECEIVER (packet_receiver):
├── Server Socket
│   ├── Listening su socket UNIX domain
│   ├── Accept connessione da sender
│   └── Gestione disconnessioni client
│
├── Thread Architecture (3 thread totali)
│   ├── RECEIVER THREAD
│   │   ├── Legge pacchetti dal socket
│   │   ├── Parsing header per lunghezza
│   │   ├── Lettura payload completo
│   │   └── Inserimento in coda circolare
│   │
│   ├── CONSUMER THREAD 1
│   │   ├── Estrazione pacchetti dalla coda
│   │   ├── Elaborazione/processing simulato
│   │   └── Logging attività
│   │
│   └── CONSUMER THREAD 2
│       ├── Estrazione pacchetti dalla coda
│       ├── Elaborazione parallela al consumer 1
│       └── Competizione per accesso coda
│
└── Coda Circolare FIFO
    ├── Buffer di 50 slot (configurabile)
    ├── Indici head/tail per FIFO
    ├── Mutex per accesso esclusivo
    ├── Condition variables per sincronizzazione
    └── Gestione overflow/underflow automatica

PROTOCOLLO PACCHETTI
--------------------

FORMATO PACCHETTO:
┌─────────────────┬─────────────────────────────────┐
│  HEADER (4 byte)│        PAYLOAD (variabile)      │
│   [length]      │           [random data]         │
└─────────────────┴─────────────────────────────────┘

HEADER:
• Tipo: int (4 byte)
• Contenuto: lunghezza totale pacchetto (header + payload)
• Endianness: nativo del sistema
• Range valori: 16 - 4096 byte

PAYLOAD:
• Contenuto: byte casuali (0-255)
• Dimensione: header.length - sizeof(int)
• Scopo: simulazione dati reali applicativi

ESEMPI PACCHETTI:
• Pacchetto 100 byte: header=100, payload=96 byte random
• Pacchetto 500 byte: header=500, payload=496 byte random  
• Pacchetto 4096 byte: header=4096, payload=4092 byte random

CODA CIRCOLARE THREAD-SAFE
---------------------------

IMPLEMENTAZIONE BUFFER CIRCOLARE:
```
buffer[QUEUE_MAX_SIZE]     Slot 0    Slot 1    Slot 2    ...    Slot 49
                            ↑                               ↑
                          head                            tail
                        (read)                           (write)
```

OPERAZIONI THREAD-SAFE:

INSERIMENTO (queue_put):
1. Lock mutex
2. While coda piena: wait su condition not_full
3. Inserisci pacchetto in posizione tail
4. Aggiorna tail = (tail + 1) % QUEUE_MAX_SIZE
5. Incrementa count
6. Signal condition not_empty
7. Unlock mutex

ESTRAZIONE (queue_get):
1. Lock mutex
2. While coda vuota: wait su condition not_empty
3. Estrai pacchetto da posizione head
4. Aggiorna head = (head + 1) % QUEUE_MAX_SIZE
5. Decrementa count
6. Signal condition not_full
7. Unlock mutex

SINCRONIZZAZIONE:
• pthread_mutex_t: accesso esclusivo alla struttura coda
• pthread_cond_t not_empty: sveglia consumer quando arrivano dati
• pthread_cond_t not_full: sveglia receiver quando si libera spazio
• Gestione shutdown graceful per terminazione thread

REQUISITI SISTEMA
-----------------
• Sistema operativo: Linux o macOS
• Compilatore: GCC con supporto ANSI C (C90)
• Libreria APUE (Advanced Programming in UNIX Environment)
• Threading: Supporto POSIX pthread
• Socket: UNIX domain socket support
• Memoria: ~200KB per buffer coda (50 slot × 4KB max)

STRUTTURA FILE
--------------
packet_protocol.h    - Header condiviso con definizioni protocolli
packet_sender.c      - Codice sorgente processo sender
packet_receiver.c    - Codice sorgente processo receiver con 3 thread
Makefile            - Script compilazione cross-platform
README.txt          - Documentazione completa

COMPILAZIONE
------------
Il sistema rileva automaticamente la piattaforma:

    make all              # Compila sender e receiver
    make packet_sender    # Compila solo sender
    make packet_receiver  # Compila solo receiver
    make test             # Mostra istruzioni test
    make demo             # Demo automatica
    make clean            # Rimuove binari e socket
    make help             # Mostra tutti i comandi

Compilazione manuale Linux:
    gcc -ansi -Wall -I./include -DLINUX -D_GNU_SOURCE \
        packet_sender.c -o packet_sender -L./lib -lapueLinux -lpthread

Compilazione manuale macOS:
    gcc -ansi -Wall -I./include -DMACOS -D_DARWIN_SOURCE \
        packet_sender.c -o packet_sender -L./lib -lapueMacOS -lpthread

UTILIZZO
--------

AVVIO RECEIVER (sempre per primo):
    ./packet_receiver

Il receiver:
- Crea socket UNIX domain su /tmp/packet_socket
- Inizializza coda circolare da 50 slot
- Avvia 3 thread (receiver + 2 consumer)
- Attende connessione da sender

Output receiver tipico:
    === Packet Receiver with Circular Queue ===
    Circular queue initialized (capacity: 50)
    Server socket listening on /tmp/packet_socket
    Waiting for client connection...
    Client connected!
    All threads started successfully!

AVVIO SENDER:
    ./packet_sender [num_packets] [delay_ms]

Parametri:
    num_packets - Numero pacchetti da inviare (default: 50)
    delay_ms    - Millisecondi tra pacchetti (default: 100)

Esempi:
    ./packet_sender                 # 50 pacchetti, 100ms delay
    ./packet_sender 100 50          # 100 pacchetti, 50ms delay
    ./packet_sender 200 0           # 200 pacchetti, no delay

Output sender tipico:
    === Packet Sender ===
    Configuration:
      Packets to send: 100
      Delay between packets: 50 ms
    Connecting to receiver...
    Packet #1: length=1024 bytes (header=4 + payload=1020)
    Packet #2: length=512 bytes (header=4 + payload=508)

PROCEDURA TEST COMPLETA
-----------------------

1. AVVIO SISTEMA:
   Terminal 1 (Receiver):
   $ ./packet_receiver
   
   Attendi output:
   Server socket listening on /tmp/packet_socket
   Waiting for client connection...

2. INVIO PACCHETTI:
   Terminal 2 (Sender):
   $ ./packet_sender 50 100
   
   Nel terminal receiver vedrai:
   Client connected!
   Receiver thread started
   Consumer thread 1 started
   Consumer thread 2 started

3. MONITORAGGIO FLUSSO:
   Receiver mostrerà in tempo reale:
   Received packet #1 (length: 1024 bytes)
   Consumer 1 processed packet #1 (payload: 1020 bytes)
   Queue status: 2/50 packets (head=1, tail=3)
   Consumer 2 processed packet #2 (payload: 508 bytes)

4. TERMINAZIONE:
   Sender termina automaticamente dopo invio
   Receiver: Ctrl+C per shutdown graceful
   
   Statistiche finali:
   === Final Statistics ===
   Packets received: 50
   Packets processed by consumer 1: 25
   Packets processed by consumer 2: 25

COMPORTAMENTO CODA CIRCOLARE
-----------------------------

SCENARIO NORMALE:
• Receiver inserisce pacchetti alla velocità di rete
• Consumer 1 e 2 rimuovono pacchetti competitivamente
• Coda funziona da buffer per variazioni di velocità

SCENARIO OVERFLOW (sender veloce):
• Coda si riempie rapidamente (50/50 slot)
• Receiver thread si blocca su queue_put()
• Attende che consumer liberino spazio
• Backpressure automatica verso sender

SCENARIO UNDERFLOW (sender lento):
• Coda si svuota spesso (0/50 slot)
• Consumer thread si bloccano su queue_get()
• Attendono arrivo nuovi pacchetti
• Sistema si adatta automaticamente

BILANCIAMENTO CARICO:
• Due consumer thread competono per gli stessi pacchetti
• Distribuzione circa 50/50 tra consumer 1 e 2
• Nessuna preferenza, puro scheduling OS
• Migliore throughput complessivo

OUTPUT E LOGGING
----------------

Receiver output dettagliato:
    === Packet Receiver with Circular Queue ===
    Circular queue initialized (capacity: 50)
    Server socket listening on /tmp/packet_socket
    Client connected!
    Receiver thread started
    Consumer thread 1 started  
    Consumer thread 2 started
    
    Received packet #1 (length: 1024 bytes)
    Consumer 1 processed packet #1 (payload: 1020 bytes)
    Received packet #2 (length: 512 bytes)
    Consumer 2 processed packet #2 (payload: 508 bytes)
    
    Queue status: 1/50 packets (head=2, tail=3)
    
    Signal 2 received. Shutting down...
    Waiting for threads to terminate...
    Receiver thread terminated
    Consumer thread 1 terminated
    Consumer thread 2 terminated
    
    === Final Statistics ===
    Packets received: 50
    Packets processed by consumer 1: 24
    Packets processed by consumer 2: 26
    Total packets processed: 50
    Queue status: 0/50 packets (head=0, tail=0)

Sender output dettagliato:
    === Packet Sender ===
    Configuration:
      Packets to send: 50
      Delay between packets: 100 ms
      Packet size range: 16 - 4096 bytes
    
    Connecting to receiver at /tmp/packet_socket...
    Connected successfully!
    
    Sending packets...
    Packet #1: length=1024 bytes (header=4 + payload=1020)
    Packet #2: length=512 bytes (header=4 + payload=508)
    ...
    Packet #50: length=2048 bytes (header=4 + payload=2044)
    
    Sending completed. Sent 50 packets.
    Closing connection...

TROUBLESHOOTING
---------------

Errore "socket creation failed":
- Sistema non supporta UNIX domain socket
- Permessi insufficienti per /tmp/
- Verificare supporto: ls -la /tmp/

Errore "connection to receiver failed":
- Receiver non avviato o terminato
- Socket path /tmp/packet_socket non esistente
- Avviare prima receiver, poi sender

Errore "bind failed":
- Socket già in uso da altro processo
- File /tmp/packet_socket già presente
- Rimuovere: rm -f /tmp/packet_socket

Errore "pthread_create failed":
- Sistema senza supporto threading
- Limite massimo thread raggiunto
- Memoria insufficiente per stack thread

Performance degradate:
- Troppi pacchetti/secondo (ridurre delay sender)
- Sistema sotto carico CPU/memoria
- Verificare con: top, htop

Pacchetti persi:
- Buffer overflow (aumentare QUEUE_MAX_SIZE)
- Consumer troppo lenti (ottimizzare processing)
- Disconnessione improvvisa sender

Coda sempre piena:
- Consumer non riescono a tenere il passo
- Aumentare numero consumer thread
- Ridurre processing time per pacchetto
- Aumentare capacità buffer

PERSONALIZZAZIONE
-----------------

Costanti modificabili in packet_protocol.h:

Dimensioni pacchetti:
    #define MAX_PACKET_SIZE 4096    /* Max 4KB per pacchetto */
    #define MIN_PACKET_SIZE 16      /* Min 16 byte per pacchetto */

Capacità coda:
    #define QUEUE_MAX_SIZE 50       /* 50 slot buffer circolare */

Socket path:
    #define SOCKET_PATH "/tmp/packet_socket"  /* Path socket UNIX */

Numero consumer thread:
Modificare packet_receiver.c per aggiungere più thread consumer:
    pthread_t g_consumer3_thread;  /* Aggiungi terzo consumer */

Processing delay:
In consumer_thread_func(), modificare:
    usleep(10000);  /* 10ms processing time */

TESTING E DEBUG
---------------

Test di carico:
    # Invio rapido 1000 pacchetti
    ./packet_sender 1000 0

Test overflow coda:
    # Sender veloce con receiver che processa lentamente
    # Modificare usleep(10000) → usleep(100000) in consumer

Test underflow coda:
    # Sender lento con consumer veloci
    ./packet_sender 100 2000

Debug con strace:
    strace -f -e trace=network,write,read ./packet_receiver

Debug threading:
    # Compilare con -g e usare gdb
    gdb ./packet_receiver
    (gdb) thread apply all bt

Verifica memoria:
    valgrind --tool=memcheck ./packet_receiver

LIMITAZIONI
-----------

• Socket UNIX domain (comunicazione locale only)
• Nessuna autenticazione o sicurezza
• Pacchetti non persistenti (memoria volatile)
• Gestione errori di rete semplificata
• Nessuna compressione o checksumming
• Threading model fisso (3 thread)
• Capacità coda fissa (ricompilazione per cambiare)

ESTENSIONI POSSIBILI
--------------------

• Supporto TCP/IP per comunicazione remota
• Persistenza pacchetti su disco
• Compressione payload per efficienza
• Checksum/CRC per integrità dati
• Pool dinamico di thread consumer
• Coda multipla per priorità pacchetti
• Interfaccia di monitoring web
• Statistiche avanzate e metriche
• Load balancing intelligente

RIFERIMENTI
-----------

• Stevens, W. Richard. "Advanced Programming in the UNIX Environment"
• Stevens, W. Richard. "UNIX Network Programming, Volume 1"
• Butenhof, David. "Programming with POSIX Threads"
• POSIX.1-2008 Thread Programming Standard

AUTORE E VERSIONE
-----------------

Implementazione basata su esempi libreria APUE
Compatibilità: Linux, macOS
Standard: ANSI C (C90)
Threading: POSIX pthread
IPC: UNIX Domain Socket

================================================================================