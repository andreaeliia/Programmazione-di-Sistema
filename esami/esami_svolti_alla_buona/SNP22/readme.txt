================================================================================
                        SMTP LATENCY MONITOR
                    Client TCP per monitoraggio prestazioni SMTP
================================================================================

DESCRIZIONE
-----------
Questo programma implementa un client TCP che monitora periodicamente le
prestazioni di server SMTP pubblici, misurando la latenza di connessione e
risposta. Include gestione centralizzata dei segnali UNIX tramite thread
dedicato per logging completo degli eventi di sistema.

Funzionalità principali:
• Connessione periodica a server SMTP con misurazione latenza
• Chiusura pulita delle connessioni tramite comando SMTP QUIT
• Logging strutturato delle misurazioni su file configurabile
• Thread separato per cattura e logging di tutti i segnali di sistema
• Gestione robusta degli errori di rete e timeout
• Supporto cross-platform Linux/macOS con libreria APUE

ARCHITETTURA
------------
Il programma utilizza un'architettura multi-thread:

THREAD PRINCIPALE (Main):
├── Parsing parametri da linea di comando
├── Loop di monitoraggio periodico
├── Connessione TCP al server SMTP target
├── Misurazione tempi di connessione e risposta
├── Invio comando QUIT per chiusura pulita
└── Logging risultati su file

THREAD SEGNALI (Signal Handler):
├── Blocco di tutti i segnali per thread principale
├── Gestione centralizzata con sigwait()
├── Logging dettagliato segnali ricevuti
├── Gestione graceful shutdown per SIGINT/SIGTERM
└── Prevenzione interruzioni indesiderate del monitoring

REQUISITI SISTEMA
-----------------
• Sistema operativo: Linux o macOS
• Compilatore: GCC con supporto ANSI C (C90)
• Libreria APUE (Advanced Programming in UNIX Environment)
• Threading: Supporto POSIX pthread
• Rete: Accesso TCP a server SMTP (porte 25, 587, 465)

STRUTTURA FILE
--------------
smtp_monitor.c     - Codice sorgente principale
Makefile          - Script compilazione cross-platform
README.txt        - Documentazione completa

COMPILAZIONE
------------
Il Makefile detecta automaticamente il sistema operativo:

    make all             # Compila il programma
    make test-gmail      # Test rapido con Gmail SMTP
    make test-local      # Test con server SMTP locale
    make clean           # Rimuove binari e log
    make help            # Mostra comandi disponibili

Compilazione manuale Linux:
    gcc -ansi -Wall -I./include -DLINUX -D_GNU_SOURCE \
        smtp_monitor.c -o smtp_monitor -L./lib -lapueLinux -lpthread

Compilazione manuale macOS:
    gcc -ansi -Wall -I./include -DMACOS -D_DARWIN_SOURCE \
        smtp_monitor.c -o smtp_monitor -L./lib -lapueMacOS -lpthread

UTILIZZO
--------
Sintassi:
    ./smtp_monitor <hostname> <porta> <intervallo_sec> <logfile>

Parametri:
    hostname      - Server SMTP da monitorare (es: smtp.gmail.com)
    porta         - Porta SMTP (25=standard, 587=submission, 465=SSL)
    intervallo_sec- Secondi tra test consecutivi (minimo 1)
    logfile       - File per salvare log delle misurazioni

Esempi d'uso:
    # Test Gmail ogni 30 secondi
    ./smtp_monitor smtp.gmail.com 587 30 gmail_monitor.log
    
    # Test server aziendale ogni 5 minuti
    ./smtp_monitor mail.company.com 25 300 company_smtp.log
    
    # Test server locale per debug
    ./smtp_monitor localhost 25 10 debug.log

FUNZIONAMENTO DETTAGLIATO
--------------------------

1. AVVIO E CONFIGURAZIONE:
   - Parsing e validazione parametri linea di comando
   - Creazione thread dedicato per gestione segnali
   - Blocco segnali nel thread principale per evitare interruzioni
   - Inizializzazione logging con header informativo

2. CICLO DI MONITORAGGIO:
   Per ogni test eseguito:
   a) Risoluzione DNS del hostname target
   b) Creazione socket TCP
   c) Misurazione tempo di connessione con gettimeofday()
   d) Lettura risposta iniziale server (attesa codice 220)
   e) Misurazione tempo di risposta
   f) Invio comando "QUIT\r\n" per chiusura protocollo
   g) Chiusura socket
   h) Logging risultati (successo/fallimento)
   i) Attesa intervallo configurato

3. GESTIONE SEGNALI:
   Thread separato che:
   - Cattura tutti i segnali possibili con sigwait()
   - Logga nome segnale e timestamp in signals.log
   - Gestisce terminazione graceful per SIGINT/SIGTERM/SIGQUIT
   - Permette continuazione per altri segnali (SIGUSR1, SIGWINCH, etc.)

4. TERMINAZIONE:
   - Segnale di terminazione catturato
   - Completamento test corrente
   - Join del thread segnali
   - Log finale con statistiche

OUTPUT E LOGGING
----------------

Log delle misurazioni (file configurabile):
    [2024-01-15 14:30:15] SUCCESS: smtp.gmail.com:587 - Connect: 45.23ms, Response: 12.84ms
    [2024-01-15 14:30:45] FAILED: smtp.gmail.com:587 - Error: Connection timeout
    [2024-01-15 14:31:15] SUCCESS: smtp.gmail.com:587 - Connect: 38.91ms, Response: 15.67ms

Log dei segnali (signals.log):
    [2024-01-15 14:32:00] Received signal 2 (SIGINT)
    [2024-01-15 14:32:15] Received signal 10 (SIGUSR1)
    [2024-01-15 14:32:30] Received signal 15 (SIGTERM)

Output console real-time:
    === SMTP Latency Monitor ===
    Target: smtp.gmail.com:587
    Interval: 30 seconds
    Starting monitoring... (Ctrl+C to stop)
    
    Test #1: [2024-01-15 14:30:15] Latency: 45.23ms + 12.84ms = 58.07ms total
    Test #2: [2024-01-15 14:30:45] Test failed: Connection timeout
    Test #3: [2024-01-15 14:31:15] Latency: 38.91ms + 15.67ms = 54.58ms total

INTERPRETAZIONE RISULTATI
--------------------------

Tempi di Latenza:
• Connect Time: Tempo per stabilire connessione TCP (risoluzione DNS + handshake)
• Response Time: Tempo per ricevere risposta SMTP iniziale dal server
• Total Time: Somma dei due precedenti = latenza percepita dall'utente

Valori Tipici:
• Connect Time: 10-100ms (dipende da geografia e qualità rete)
• Response Time: 5-50ms (dipende da carico server)
• Timeout: 10 secondi (configurabile modificando SMTP_TIMEOUT_SEC)

Codici di Stato SMTP:
• 220: Server pronto (successo)
• Altri codici: Server non standard o sovraccarico

TROUBLESHOOTING
---------------

Errore "Connection failed":
- Verificare connettività di rete
- Controllare che porta sia aperta (telnet hostname porta)
- Alcuni server richiedono connessioni SSL (porta 465)

Errore "Invalid SMTP response":
- Server potrebbe non essere SMTP
- Server potrebbe richiedere STARTTLS
- Verificare manualmente: telnet hostname porta

Errore "Permission denied" sui log:
- Controllare permessi directory corrente
- Specificare path assoluto per logfile

Prestazioni inaspettate:
- Latenza alta: problemi di rete o server sovraccarico
- Timeout frequenti: server non disponibile o filtri firewall
- Variabilità alta: instabilità di rete

Problemi threading:
- Errore pthread_create: sistema senza supporto threading
- Segnali non catturati: verificare privileges del processo

PERSONALIZZAZIONE
-----------------

Modifiche comuni nel codice sorgente:

Timeout personalizzato:
    #define SMTP_TIMEOUT_SEC 30    /* Default: 10 secondi */

Buffer risposta maggiore:
    #define MAX_RESPONSE_LEN 2048  /* Default: 1024 byte */

Segnali aggiuntivi da gestire:
    /* Aggiungere in signal_handler_thread() */

Formato log personalizzato:
    /* Modificare log_measurement() */

PROTOCOLLO SMTP
---------------

Il programma implementa una versione minimale del protocollo SMTP:

1. Client si connette alla porta SMTP
2. Server risponde con "220 hostname ESMTP ready"
3. Client invia "QUIT\r\n"
4. Server risponde con "221 hostname closing connection"
5. Connessione chiusa

Questo approccio minimale permette test di connettività senza autenticazione
o invio di email reali.

SICUREZZA
---------

Considerazioni di sicurezza:
• Nessuna autenticazione richiesta (solo test connettività)
• Nessun dato sensibile trasmesso
• Log files potrebbero contenere informazioni di rete
• Thread signal handling isolato per stabilità

LIMITAZIONI
-----------

• Non supporta connessioni SSL/TLS native
• Non implementa autenticazione SMTP
• Gestione DNS sincrona (bloccante)
• Un solo target per istanza di programma
• Precisione timing limitata da gettimeofday()

ESEMPI SERVER SMTP PUBBLICI
----------------------------

Server di test comuni:
• Gmail: smtp.gmail.com:587 (submission port)
• Outlook: smtp-mail.outlook.com:587
• Yahoo: smtp.mail.yahoo.com:587
• Server locali: localhost:25

Nota: Molti server moderni richiedono STARTTLS o connessioni SSL dirette
(porta 465) che questo client base non supporta.

RIFERIMENTI
-----------

• RFC 5321 - Simple Mail Transfer Protocol
• Stevens, W. Richard. "Advanced Programming in the UNIX Environment"
• POSIX.1-2008 Thread Programming
• RFC 1123 - Requirements for Internet Hosts

AUTORE E VERSIONE
-----------------

Implementazione basata su esempi libreria APUE
Compatibilità: Linux, macOS
Standard: ANSI C (C90)
Threading: POSIX pthread

================================================================================