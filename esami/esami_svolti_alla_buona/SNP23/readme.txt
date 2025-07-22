================================================================================
                    SERVER TCP MULTI-THREAD CON VETTORE CONDIVISO
                        Gestione Concorrenza e Sincronizzazione Thread
================================================================================

DESCRIZIONE
-----------
Questo programma implementa un server TCP multi-thread che gestisce simultaneamente
tre porte TCP diverse. Ogni porta è gestita da un thread dedicato che riceve
interi dai client e li salva in un vettore condiviso thread-safe.

Caratteristiche principali:
• Server TCP con 3 thread, uno per ogni porta
• Vettore condiviso di interi protetto da mutex POSIX  
• Gestione concorrenza per prevenire race conditions
• Inserimento thread-safe nel primo slot libero del vettore
• Client di test per verifica funzionalità
• Logging dettagliato per debugging e monitoraggio
• Gestione graceful shutdown con Ctrl+C

ARCHITETTURA
------------
Il sistema è composto da due programmi principali:

SERVER (tcp_server):
├── Thread Principale
│   ├── Inizializzazione vettore condiviso
│   ├── Creazione 3 thread per gestione porte
│   ├── Monitoring periodico stato vettore
│   └── Gestione segnali di terminazione
│
├── Thread Porta 1 (porta configurabile)
│   ├── Binding socket TCP
│   ├── Accept connessioni client
│   ├── Ricezione interi dai client
│   └── Inserimento thread-safe nel vettore
│
├── Thread Porta 2 (porta configurabile)
│   └── [Stessa logica Thread Porta 1]
│
└── Thread Porta 3 (porta configurabile)
    └── [Stessa logica Thread Porta 1]

CLIENT (tcp_client):
├── Connessione TCP al server
├── Invio intero al server
├── Ricezione conferma
└── Chiusura connessione

VETTORE CONDIVISO:
├── Array di 1000 interi
├── Mutex POSIX per accesso esclusivo
├── Ricerca primo slot libero (valore -1)
├── Contatore elementi presenti
└── Logging operazioni per debugging

REQUISITI SISTEMA
-----------------
• Sistema operativo: Linux o macOS
• Compilatore: GCC con supporto ANSI C (C90)
• Libreria APUE (Advanced Programming in UNIX Environment)
• Threading: Supporto POSIX pthread
• Rete: Disponibilità porte TCP (raccomandato range 8000-9000)

STRUTTURA FILE
--------------
tcp_server.c       - Codice sorgente server multi-thread
tcp_client.c       - Codice sorgente client di test  
Makefile          - Script compilazione cross-platform
README.txt        - Documentazione completa

COMPILAZIONE
------------
Il Makefile detecta automaticamente il sistema operativo:

    make all            # Compila server e client
    make tcp_server     # Compila solo server
    make tcp_client     # Compila solo client
    make test           # Mostra istruzioni test
    make clean          # Rimuove binari
    make help           # Mostra tutti i comandi

Compilazione manuale Linux:
    gcc -ansi -Wall -I./include -DLINUX -D_GNU_SOURCE \
        tcp_server.c -o tcp_server -L./lib -lapueLinux -lpthread

Compilazione manuale macOS:
    gcc -ansi -Wall -I./include -DMACOS -D_DARWIN_SOURCE \
        tcp_server.c -o tcp_server -L./lib -lapueMacOS -lpthread

UTILIZZO
--------

AVVIO SERVER:
    ./tcp_server <porta1> <porta2> <porta3>

Parametri server:
    porta1, porta2, porta3 - Tre porte TCP diverse (1024-65535)
    
Esempio server:
    ./tcp_server 8001 8002 8003

UTILIZZO CLIENT:
    ./tcp_client <hostname> <porta> <valore>

Parametri client:
    hostname - Indirizzo server (localhost, IP, nome dominio)
    porta    - Una delle porte del server
    valore   - Intero da inviare al server

Esempi client:
    ./tcp_client localhost 8001 42
    ./tcp_client 127.0.0.1 8002 100  
    ./tcp_client server.domain.com 8003 -50

PROCEDURA TEST COMPLETA
-----------------------

1. AVVIO SERVER:
   Terminal 1:
   $ ./tcp_server 8001 8002 8003
   
   Output atteso:
   === TCP Multi-Thread Server ===
   Shared vector initialized (capacity: 1000)
   TCP socket created and listening on port 8001
   TCP socket created and listening on port 8002  
   TCP socket created and listening on port 8003
   All threads started successfully!

2. TEST CLIENT MULTIPLI:
   Terminal 2:
   $ ./tcp_client localhost 8001 100
   
   Terminal 3:
   $ ./tcp_client localhost 8002 200
   
   Terminal 4:
   $ ./tcp_client localhost 8003 300

3. MONITORAGGIO SERVER:
   Il server mostrerà automaticamente:
   Thread 0: Added value 100 at position 0 (total: 1/1000)
   Thread 1: Added value 200 at position 1 (total: 2/1000)
   Thread 2: Added value 300 at position 2 (total: 3/1000)

4. TERMINAZIONE:
   Nel terminal del server: Ctrl+C
   
   Output atteso:
   Signal 2 received. Shutting down server...
   Waiting for threads to terminate...
   Final vector status: [...]

GESTIONE CONCORRENZA
---------------------

Il server implementa sincronizzazione thread-safe attraverso:

MUTEX PROTECTION:
• pthread_mutex_lock() prima di accedere al vettore
• Operazioni atomiche di ricerca e inserimento
• pthread_mutex_unlock() dopo modifica vettore
• Prevenzione race conditions tra thread

RICERCA PRIMO SLOT LIBERO:
• Scansione sequenziale del vettore
• Identificazione slot con valore EMPTY_SLOT (-1)
• Inserimento atomico del nuovo valore
• Aggiornamento contatore elementi

LOGGING THREAD-SAFE:
• Stampe sincronizzate con informazioni thread ID
• Monitoraggio operazioni per debugging
• Status periodico del vettore ogni 10 secondi

SCENARIO TIPICO:
1. Client si connette a Thread 1 (porta 8001)
2. Thread 1 riceve valore 42
3. Thread 1 acquisisce mutex del vettore
4. Thread 1 cerca primo slot libero (posizione 5)
5. Thread 1 inserisce 42 in posizione 5
6. Thread 1 incrementa contatore (size = 6)
7. Thread 1 rilascia mutex
8. Thread 1 invia "OK" al client

OUTPUT E LOGGING
----------------

Server output esempio:
    === TCP Multi-Thread Server ===
    Shared vector initialized (capacity: 1000)
    TCP socket created and listening on port 8001
    TCP socket created and listening on port 8002
    TCP socket created and listening on port 8003
    Thread 0 starting on port 8001
    Thread 1 starting on port 8002
    Thread 2 starting on port 8003
    All threads started successfully!
    
    Thread 0: Client connected from 127.0.0.1:54321
    Thread 0: Received value: 42
    Thread 0: Added value 42 at position 0 (total: 1/1000)
    
    === Vector Status ===
    Used slots: 1/1000
    Values: [0]=42
    =====================

Client output esempio:
    === TCP Client Test ===
    Target: localhost:8001
    Value to send: 42
    
    Connecting to localhost:8001...
    Connected successfully!
    Sending value: 42
    Server response: OK: Value added to vector
    Connection closed.

GESTIONE ERRORI
---------------

SERVER:
• Porte già in uso: "bind failed"
• Vettore pieno: "Vector is full!" 
• Client disconnesso: gestione automatica
• Errori socket: logging e continuazione servizio

CLIENT:
• Server non raggiungibile: "Connection failed"
• Hostname non valido: "Failed to resolve hostname"
• Porta non valida: validazione range
• Risposta mancante: "No response received"

PERSONALIZZAZIONE
-----------------

Costanti modificabili in tcp_server.c:

Dimensione vettore:
    #define MAX_VECTOR_SIZE 1000        /* Default: 1000 elementi */

Client simultanei per porta:
    #define MAX_CLIENTS_PER_PORT 10     /* Default: 10 client */

Coda listen socket:
    #define LISTEN_BACKLOG 5            /* Default: 5 connessioni */

Valore slot libero:
    #define EMPTY_SLOT -1               /* Default: -1 */

Frequenza status monitoring:
    sleep(10);  /* Default: 10 secondi nel main loop */

TESTING E DEBUG
---------------

Test di carico:
    # Genera 100 client simultanei
    for i in {1..100}; do
        ./tcp_client localhost 8001 $i &
    done

Test stress concorrenza:
    # Test su tutte e 3 le porte simultaneamente
    ./tcp_client localhost 8001 100 &
    ./tcp_client localhost 8002 200 &
    ./tcp_client localhost 8003 300 &
    wait

Verifica stato vettore:
    # Il server stampa automaticamente ogni 10 secondi
    # Oppure invia SIGUSR1 per dump immediato (se implementato)

Test vettore pieno:
    # Invia più di MAX_VECTOR_SIZE valori
    # Server dovrebbe rispondere "ERROR: Vector is full"

TROUBLESHOOTING
---------------

Errore "Address already in use":
- Porta già utilizzata da altro processo
- Attendere timeout sistema (2-4 minuti) 
- Utilizzare porte diverse
- Verificare con: netstat -tulpn | grep <porta>

Errore "Connection refused":
- Server non avviato su quella porta
- Firewall che blocca connessioni
- Verifica binding con: lsof -i :<porta>

Errore pthread_create:
- Sistema senza supporto threading
- Limiti sistema per numero thread
- Memoria insufficiente

Performance degradata:
- Troppe connessioni simultanee
- Contention sul mutex del vettore
- Sistema sotto carico elevato

Vettore corrotto:
- Race condition non gestita (bug nel codice)
- Overflow buffer nelle operazioni
- Verifica con tool come Valgrind

LIMITAZIONI
-----------

• Dimensione vettore fissa (1000 elementi)
• Un solo vettore condiviso per tutti i thread
• Nessuna persistenza dati (memoria volatile)
• Gestione client mono-messaggio (no sessioni)
• Nessuna autenticazione o sicurezza
• Threading model semplificato (no pool di worker)

SICUREZZA
---------

Considerazioni di sicurezza:
• Server accetta connessioni da qualsiasi IP (INADDR_ANY)
• Nessuna validazione input oltre conversione atoi()
• Nessuna autenticazione client
• Buffer fissi potrebbero essere vulnerabili
• Log potrebbero contenere informazioni sensibili

ESTENSIONI POSSIBILI
--------------------

• Persistenza vettore su file
• Interfaccia web per monitoring
• Pool di thread worker
• Autenticazione client
• Crittografia comunicazioni
• Load balancing tra porte
• Configurazione runtime (file config)
• Metriche prestazioni avanzate

RIFERIMENTI
-----------

• Stevens, W. Richard. "Advanced Programming in the UNIX Environment"
• Stevens, W. Richard. "UNIX Network Programming, Volume 1"
• POSIX.1-2008 Thread Programming
• RFC 793 - Transmission Control Protocol

AUTORE E VERSIONE
-----------------

Implementazione basata su esempi libreria APUE
Compatibilità: Linux, macOS  
Standard: ANSI C (C90)
Threading: POSIX pthread
Networking: BSD Socket API

================================================================================