===============================================================================
                    SERVER CONCORRENTE CON THREAD POOL E CODA FIFO
===============================================================================

DESCRIZIONE:
-----------
Implementazione di un server concorrente in C90 che riceve richieste dai client
via rete e le elabora utilizzando un pool di thread worker e una coda FIFO 
thread-safe.

CARATTERISTICHE:
---------------
- Server TCP che ascolta sulla porta 8080
- Pool di 5 thread worker per l'elaborazione concorrente
- Coda FIFO thread-safe per gestire le richieste
- Tempi di elaborazione e servizio casuali (0-1 secondo)
- Logging dettagliato delle operazioni
- Compatibilita con Linux e macOS
- Utilizzo della libreria APUE

ARCHITETTURA:
------------
1. Thread principale: accetta connessioni client
2. Thread client handler: riceve richieste e le inserisce nella coda
3. Thread worker pool: elabora richieste dalla coda con disciplina FIFO
4. Coda thread-safe: gestisce sincronizzazione tra thread

COMPILAZIONE:
------------
Assicurarsi di avere:
- Libreria APUE installata in ./lib/
- Header APUE in ./include/
- Compilatore GCC

Comandi:
  make            # Compila server e client
  make server     # Compila solo il server
  make client     # Compila solo il client
  make clean      # Rimuove eseguibili

UTILIZZO:
--------
1. Avviare il server:
   ./server

2. In un altro terminale, avviare il client:
   ./client [messaggio]

   Esempi:
   ./client
   ./client "Ciao server!"

3. Per testare con piu client contemporaneamente:
   make test

4. Fermare il server con Ctrl+C

STRUTTURA FILES:
---------------
server.c        - Codice sorgente del server
client.c        - Codice sorgente del client di test  
Makefile        - Script di compilazione
README.txt      - Questo file

FUNZIONALITA DEL SERVER:
-----------------------
- Accetta fino a 10 connessioni simultanee
- Utilizza 5 thread worker per elaborare richieste
- Ogni richiesta viene elaborata con tempo casuale 0-1 sec
- Risposta inviata con tempo di servizio casuale 0-1 sec
- Log dettagliato con timestamp di tutte le operazioni
- Gestione graceful delle connessioni client

SINCRONIZZAZIONE:
----------------
- Mutex per proteggere la coda delle richieste
- Condition variable per notificare thread worker
- Mutex per logging thread-safe
- Gestione corretta della memoria dinamica

TESTING:
-------
Il server puo essere testato:
1. Con il client fornito
2. Con telnet: telnet localhost 8080
3. Con curl: curl -d "test" localhost:8080
4. Con script per caricare il server

LOGGING:
-------
Il server stampa informazioni dettagliate su:
- Avvio e configurazione
- Connessioni client
- Stato della coda
- Elaborazione richieste
- Tempi di processing e servizio
- Chiusura connessioni

COMPATIBILITA:
-------------
- Standard C90 (ANSI C)
- Linux (testato su distribuzioni recenti)
- macOS (testato su versioni recenti)
- Utilizza libreria APUE per portabilita

LIMITAZIONI:
-----------
- Numero massimo client: 10 contemporaneamente
- Numero thread worker fisso: 5
- Porta fissa: 8080
- Dimensione buffer: 256 bytes

PERSONALIZZAZIONI:
-----------------
Per modificare parametri, editare le costanti in server.c:
- MAX_CLIENTS: numero massimo connessioni
- WORKER_THREADS: numero thread worker
- PORT: porta di ascolto
- BUFFER_SIZE: dimensione buffer messaggi

TROUBLESHOOTING:
---------------
- Se la porta 8080 e occupata, cambiare PORT in server.c
- Se il server non si avvia, verificare permessi e libreria APUE
- Per debug dettagliato, aggiungere flag -DDEBUG alla compilazione

AUTORE:
------
Implementazione basata su specifica di progetto
Utilizza libreria APUE (Advanced Programming in the UNIX Environment)

===============================================================================