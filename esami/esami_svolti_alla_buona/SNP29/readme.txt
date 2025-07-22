================================================================================
                        NETWORK DATA COLLECTOR
                    Sistema Multi-Thread con Condition Variables
================================================================================

DESCRIZIONE
-----------
Sistema multi-thread che implementa la raccolta periodica di dati da servizi
di rete e il salvataggio automatico su file. Due thread lavorano in coordinazione
utilizzando condition variables per la sincronizzazione immediata.

Funzionalità principali:
• Thread network: accesso periodico a servizi di rete HTTP
• Thread writer: salvataggio immediato delle risposte in file
• Sincronizzazione con condition variable per notifica real-time
• Accumulo sequenziale di tutte le risposte ricevute
• Configurazione flessibile di intervallo, file output e servizio target
• Gestione robusta errori di rete e disconnessioni

ARCHITETTURA SISTEMA
--------------------

Il sistema è composto da due thread principali che operano concorrentemente:

THREAD NETWORK (Producer):
├── Timing periodico configurabile
├── Richieste HTTP GET a servizio remoto
├── Parsing risposta e estrazione body
├── Memorizzazione in buffer condiviso thread-safe
└── Notifica immediata tramite condition variable

THREAD WRITER (Consumer):
├── Attesa dati con pthread_cond_wait()
├── Estrazione dati dal buffer condiviso
├── Scrittura formattata su file con timestamp
├── Flush immediato per persistenza
└── Loop continuo fino a terminazione

SINCRONIZZAZIONE:
├── pthread_mutex_t: accesso esclusivo buffer condiviso
├── pthread_cond_t: notifica immediata disponibilità dati
├── Flag data_ready: stato presenza nuovi dati
└── Gestione graceful shutdown coordinata

DATI CONDIVISI:
├── Buffer response_data[4096]: risposta di rete
├── Flag data_ready: segnalazione nuovi dati
├── Flag thread_running: controllo terminazione
└── Protezione mutex per accesso thread-safe

PROTOCOLLO HTTP SEMPLIFICATO
-----------------------------

Il sistema implementa un client HTTP basico per massima compatibilità:

RICHIESTA HTTP:
    GET /path HTTP/1.1
    Host: hostname
    Connection: close
    User-Agent: NetworkDataCollector/1.0

PARSING RISPOSTA:
• Lettura completa risposta HTTP
• Estrazione body dopo header (cerca "\r\n\r\n")
• Pulizia newline finali per formattazione
• Gestione risposte fino a 4KB

SERVIZI SUPPORTATI:
• httpbin.org/uuid - Genera UUID casuali
• httpbin.org/time - Timestamp server corrente  
• httpbin.org/ip - IP pubblico del client
• Qualsiasi servizio HTTP che risponda con testo

REQUISITI SISTEMA
-----------------
• Sistema operativo: Linux o macOS
• Compilatore: GCC con supporto ANSI C (C90)
• Libreria APUE (Advanced Programming in UNIX Environment)
• Threading: Supporto POSIX pthread
• Rete: Accesso HTTP esterno (porta 80)
• DNS: Risoluzione nomi per servizi remoti

STRUTTURA FILE
--------------
network_data_collector.c  - Codice sorgente completo
Makefile                  - Script compilazione cross-platform
README.txt               - Documentazione completa

COMPILAZIONE
------------
Il Makefile rileva automaticamente il sistema operativo:

    make all        # Compila il programma
    make test       # Esegue test con parametri default
    make test-fast  # Test con intervallo 5 secondi
    make test-uuid  # Test con servizio UUID
    make test-time  # Test con servizio time
    make clean      # Rimuove binari e file output
    make help       # Mostra tutti i comandi

Compilazione manuale Linux:
    gcc -ansi -Wall -I./include -DLINUX -D_GNU_SOURCE \
        network_data_collector.c -o network_data_collector \
        -L./lib -lapueLinux -lpthread

Compilazione manuale macOS:
    gcc -ansi -Wall -I./include -DMACOS -D_DARWIN_SOURCE \
        network_data_collector.c -o network_data_collector \
        -L./lib -lapueMacOS -lpthread

UTILIZZO
--------

SINTASSI:
    ./network_data_collector [intervallo] [file_output] [url_servizio]

PARAMETRI:
    intervallo   - Secondi tra richieste di rete (default: 10)
    file_output  - File per salvare risposte (default: network_responses.txt)
    url_servizio - URL servizio HTTP (default: httpbin.org/uuid)

ESEMPI D'USO:
    # Configurazione default (10 sec, network_responses.txt, UUID)
    ./network_data_collector
    
    # Intervallo personalizzato
    ./network_data_collector 5
    
    # Intervallo e file personalizzati
    ./network_data_collector 15 my_responses.txt
    
    # Configurazione completa personalizzata
    ./network_data_collector 8 time_data.txt "httpbin.org/time"
    
    # Test con diversi servizi
    ./network_data_collector 5 uuid.txt "httpbin.org/uuid"
    ./network_data_collector 10 ip.txt "httpbin.org/ip"
    ./network_data_collector 20 headers.txt "httpbin.org/headers"

AVVIO E FUNZIONAMENTO
---------------------

OUTPUT INIZIALE:
    === Network Data Collector ===
    Configuration:
      Request interval: 10 seconds
      Output file: network_responses.txt
      Service URL: httpbin.org/uuid
      Synchronization: condition variable
    
    Shared data structure initialized
    Starting threads...
    Network thread started (interval: 10 seconds)
    Writer thread started (output file: network_responses.txt)
    Both threads started successfully!

OPERAZIONE NORMALE:
    [2024-01-15 14:30:00] Making network request to: httpbin.org/uuid
    [2024-01-15 14:30:01] Received 36 bytes: {"uuid": "f47ac10b-58cc-4372-a567-0e02b2c3d479"}
    Saved response #1 to file
    
    [2024-01-15 14:30:10] Making network request to: httpbin.org/uuid
    [2024-01-15 14:30:11] Received 36 bytes: {"uuid": "6ba7b810-9dad-11d1-80b4-00c04fd430c8"}
    Saved response #2 to file

TERMINAZIONE (Ctrl+C):
    Signal 2 received. Shutting down...
    Network thread terminated
    Writer thread terminated
    
    === Final Statistics ===
    Network requests made: 5
    Responses saved to file: 5
    Output file: network_responses.txt

FORMATO FILE OUTPUT
-------------------

Il file di output contiene le risposte con timestamp e formattazione strutturata:

    === Network Data Collection Session Started: 2024-01-15 14:30:00 ===
    [2024-01-15 14:30:01] {"uuid": "f47ac10b-58cc-4372-a567-0e02b2c3d479"}
    [2024-01-15 14:30:11] {"uuid": "6ba7b810-9dad-11d1-80b4-00c04fd430c8"}
    [2024-01-15 14:30:21] {"uuid": "12345678-1234-5678-9abc-123456789abc"}
    [2024-01-15 14:30:31] {"uuid": "87654321-4321-8765-cba9-876543210fed"}
    [2024-01-15 14:30:41] {"uuid": "abcdef12-3456-7890-abcd-ef1234567890"}
    === Session Ended: 2024-01-15 14:31:00 (Responses saved: 5) ===

CARATTERISTICHE:
• Header sessione con timestamp inizio
• Entry per ogni risposta con timestamp preciso
• Footer con timestamp fine e conteggio totale
• Append mode: sessioni multiple nello stesso file
• Flush immediato: dati persistenti anche in caso di crash

SINCRONIZZAZIONE CONDITION VARIABLE
-----------------------------------

MECCANISMO DI FUNZIONAMENTO:

THREAD NETWORK (Producer):
1. Effettua richiesta HTTP al servizio
2. Riceve e processa risposta
3. Lock mutex buffer condiviso
4. Copia dati nel buffer
5. Imposta flag data_ready = 1
6. pthread_cond_signal() sveglia writer
7. Unlock mutex
8. Attende intervallo configurato

THREAD WRITER (Consumer):  
1. Lock mutex buffer condiviso
2. While (!data_ready): pthread_cond_wait()
3. Quando svegliato: copia dati dal buffer
4. Imposta flag data_ready = 0
5. Unlock mutex
6. Scrive dati su file con timestamp
7. Torna al punto 1 (loop continuo)

VANTAGGI CONDITION VARIABLE:
• Notifica immediata: zero latenza tra ricezione e salvataggio
• Efficienza: thread writer dorme quando non ci sono dati
• Sincronizzazione: accesso sicuro al buffer condiviso
• Scalabilità: facilmente estendibile a più producer/consumer

CONFRONTO CON ALTERNATIVE:
• Polling: spreco CPU, latenza variabile
• Pipe/socket: overhead sistema, complessità extra
• Semafori: meno controllo fine, semantica diversa
• Condition variable: soluzione ottimale per questo scenario

GESTIONE ERRORI E ROBUSTEZZA
-----------------------------

ERRORI DI RETE:
• Risoluzione DNS fallita: log errore, riprova prossimo ciclo
• Connessione TCP fallita: log errore, mantiene scheduling
• Timeout HTTP: gestito con socket non-bloccante
• Risposta malformata: skip entry, continua operazioni

GESTIONE FILE:
• File non apribile: termina writer thread con errore
• Disco pieno: errore write detectato e loggato
• Permessi insufficienti: errore all'apertura file
• Path non valido: validazione e fallback

TERMINAZIONE GRACEFUL:
• SIGINT (Ctrl+C): catturato e gestito pulitamente
• Flag thread_running: coordinate shutdown  
• pthread_cond_signal(): sveglia thread in attesa
• Join thread: attende terminazione completa
• Cleanup mutex/condition: libera risorse

RECOVERY:
• Errori di rete temporanei: retry automatico
• File temporaneamente bloccato: retry con delay
• Crash sistema: file già scritto preservato (flush)
• Restart programma: append mode mantiene storico

CONFIGURAZIONI AVANZATE
-----------------------

MODIFICA PARAMETRI NETWORK:
• Timeout connessione: modificare socket options
• User-Agent: personalizzare in send_http_request()
• Header HTTP extra: aggiungere in richiesta
• Porta diversa: modificare HTTP_PORT constant

OTTIMIZZAZIONE PERFORMANCE:
• Buffer size: aumentare MAX_RESPONSE_SIZE
• Batch write: accumulare più risposte prima di scrivere
• Thread pool: più thread network per servizi multipli
• Compressione: abilitare gzip nelle richieste HTTP

PERSONALIZZAZIONE OUTPUT:
• Formato timestamp: modificare strftime() format
• Separatori: cambiare formato entry file
• Metadati extra: aggiungere informazioni richiesta
• Rotazione file: implementare logica split per dimensione

SERVIZI CUSTOM:
• API REST: modificare header per API key
• HTTPS: aggiungere supporto SSL/TLS  
• POST requests: modificare metodo HTTP
• JSON parsing: estrarre campi specifici

TROUBLESHOOTING
---------------

Errore "Failed to resolve hostname":
- Verificare connessione internet
- Testare risoluzione DNS: nslookup httpbin.org
- Controllare file /etc/resolv.conf
- Provare con IP diretto invece di hostname

Errore "Connection failed":
- Verificare raggiungibilità servizio: telnet httpbin.org 80
- Controllare firewall locale
- Verificare proxy aziendale se presente
- Testare con curl per confronto

Errore "Failed to open output file":
- Verificare permessi directory
- Controllare spazio disco disponibile: df -h
- Verificare path assoluto vs relativo
- Testare creazione file manuale: touch filename

Errore "pthread_create failed":
- Verificare limiti sistema: ulimit -u
- Controllare memoria disponibile: free -h
- Verificare supporto pthread nella libreria APUE
- Controllare linking -lpthread nel Makefile

Performance basse:
- Monitorare utilizzo CPU: top, htop
- Verificare latenza rete: ping servizio
- Controllare I/O disco: iotop
- Ridurre frequenza richieste se necessario

File output corrotto:
- Verificare terminazione pulita con Ctrl+C
- Non uccidere processo con kill -9
- Controllare spazio disco durante esecuzione
- Usare sync per forzare scrittura

TESTING E VALIDAZIONE
---------------------

TEST FUNZIONALE BASE:
    # Test 30 secondi con salvataggio ogni 5 secondi
    ./network_data_collector 5 test.txt
    # Dopo 30 secondi Ctrl+C
    # Verificare file test.txt contiene ~6 entry

TEST SERVIZI DIVERSI:
    # UUID service
    ./network_data_collector 3 uuid.txt "httpbin.org/uuid"
    
    # Time service  
    ./network_data_collector 5 time.txt "httpbin.org/now"
    
    # IP service
    ./network_data_collector 10 ip.txt "httpbin.org/ip"

TEST ROBUSTEZZA:
    # Disconnessione rete durante test
    # Verifica: log errori ma programma continua
    
    # Riempimento disco
    # Verifica: errori write detectati
    
    # Terminazione brusca
    # Verifica: file parziale preservato

VALIDAZIONE SINCRONIZZAZIONE:
    # Aggiungere logging debug in mutex lock/unlock
    # Verificare nessun deadlock o race condition
    # Contare entry scritte vs richieste fatte
    # Misurare latenza tra ricezione e salvataggio

METRICHE PERFORMANCE:
    # Throughput: richieste/minuto
    # Latenza: tempo ricezione→salvataggio
    # Utilizzo memoria: size buffer e thread stack
    # Utilizzo disco: crescita file output

LIMITAZIONI
-----------

• Supporto solo HTTP (non HTTPS nativo)
• Un solo servizio per sessione
• Buffer singolo (non pipeline richieste)
• Nessuna autenticazione avanzata
• Formato output fisso
• Threading model semplificato
• Gestione errori di base

ESTENSIONI POSSIBILI
--------------------

• Supporto HTTPS con OpenSSL
• Multi-servizio con thread pool
• Pipeline richieste HTTP
• Autenticazione OAuth/API keys
• Output formato JSON/XML/CSV
• Database storage invece di file
• Interfaccia web monitoring
• Configurazione runtime (file config)
• Compressione e crittografia output
• Load balancing tra servizi multipli

RIFERIMENTI
-----------

• Stevens, W. Richard. "Advanced Programming in the UNIX Environment"
• Butenhof, David. "Programming with POSIX Threads"
• RFC 7230 - HTTP/1.1 Message Syntax and Routing
• POSIX.1-2008 Thread Programming Standard

AUTORE E VERSIONE
-----------------

Implementazione basata su esempi libreria APUE
Compatibilità: Linux, macOS
Standard: ANSI C (C90)
Threading: POSIX pthread con condition variables
Networking: BSD Socket API

================================================================================