================================================================================
                    SISTEMA MULTIPLAYER SU MATRICE 200x200
                        Client-Server Game con Eliminazione
================================================================================

DESCRIZIONE
-----------
Sistema di gioco multiplayer client-server dove i giocatori si muovono su una
matrice virtuale 200x200 e possono eliminarsi a vicenda tramite collisioni.
Il server gestisce lo stato globale del gioco e coordina tutti i client tramite
comunicazione mista unicast/multicast.

Caratteristiche principali:
• Matrice di gioco condivisa 200x200 posizioni
• Movimento con tasti u/n/h/j (su/giù/sinistra/destra)
• Eliminazione per collisione tra giocatori
• Comunicazione unicast client→server per movimenti
• Comunicazione multicast server→client per aggiornamenti stato
• Indirizzamento dinamico basato su IP locale
• Ultimo giocatore attivo vince la partita

ARCHITETTURA SISTEMA
--------------------

SERVER (game_server):
├── Gestione Stato Gioco
│   ├── Matrice 200x200 con posizioni giocatori
│   ├── Lista client attivi con nickname e posizioni
│   ├── Logica eliminazione per collisioni
│   └── Rilevamento fine gioco (ultimo sopravvissuto)
│
├── Comunicazione di Rete
│   ├── Socket UDP unicast per ricezione comandi client
│   ├── Socket UDP multicast per broadcast aggiornamenti
│   ├── Porta unicast: 7xxx (dove xxx = ultimo ottetto IP)
│   └── Indirizzo multicast: 230.0.0.xxx
│
└── Game Loop
    ├── Ricezione messaggi client (join/move/quit)
    ├── Aggiornamento stato matrice
    ├── Broadcast periodico stato completo
    └── Gestione eventi speciali (eliminazioni/vittoria)

CLIENT (game_client):
├── Input Handler
│   ├── Controlli tastiera non-bloccanti
│   ├── Mapping tasti: u=su, n=giù, h=sx, j=dx, q=quit
│   └── Terminale raw mode per responsività
│
├── Comunicazione di Rete  
│   ├── Socket UDP unicast per invio comandi al server
│   ├── Socket UDP multicast per ricezione aggiornamenti
│   ├── Auto-discovery configurazione rete da IP locale
│   └── Gestione riconnessione e timeout
│
└── Game Interface
    ├── Rendering semplificato matrice di gioco
    ├── Display posizioni altri giocatori
    ├── Notifiche eliminazioni e fine gioco
    └── Statistiche partita real-time

PROTOCOLLO COMUNICAZIONE
------------------------

MESSAGGI CLIENT → SERVER (Unicast):
• MSG_CLIENT_JOIN: Richiesta partecipazione con nickname e posizione iniziale
• MSG_CLIENT_MOVE: Comando movimento (direzione u/n/h/j)
• MSG_CLIENT_QUIT: Notifica abbandono volontario

MESSAGGI SERVER → CLIENT (Multicast):
• MSG_SERVER_UPDATE: Stato completo gioco (posizioni tutti i client)
• MSG_SERVER_ELIMINATION: Notifica eliminazione specifica
• MSG_SERVER_GAME_END: Dichiarazione vincitore e fine partita

CONFIGURAZIONE AUTOMATICA RETE:
• Rileva IP locale automaticamente
• Calcola ultimo ottetto (xxx)
• Porta unicast = 7xxx (7050 se xxx < 100)
• Indirizzo multicast = 230.0.0.xxx

REQUISITI SISTEMA
-----------------
• Sistema operativo: Linux o macOS
• Compilatore: GCC con supporto ANSI C (C90)
• Libreria APUE (Advanced Programming in UNIX Environment)
• Rete: Supporto UDP unicast e multicast
• Terminale: Supporto ANSI escape codes per rendering

STRUTTURA FILE
--------------
game_protocol.h    - Header condiviso con definizioni protocolli
game_server.c      - Codice sorgente server multiplayer
game_client.c      - Codice sorgente client di gioco
Makefile          - Script compilazione cross-platform
README.txt        - Documentazione completa

COMPILAZIONE
------------
Il sistema detecta automaticamente la piattaforma:

    make all           # Compila server e client
    make game_server   # Compila solo server
    make game_client   # Compila solo client
    make test          # Mostra istruzioni test
    make clean         # Rimuove binari
    make help          # Mostra tutti i comandi

Compilazione manuale Linux:
    gcc -ansi -Wall -I./include -DLINUX -D_GNU_SOURCE \
        game_server.c -o game_server -L./lib -lapueLinux

Compilazione manuale macOS:
    gcc -ansi -Wall -I./include -DMACOS -D_DARWIN_SOURCE \
        game_server.c -o game_server -L./lib -lapueMacOS

UTILIZZO
--------

AVVIO SERVER:
    ./game_server

Il server si configura automaticamente:
- Rileva IP locale e calcola porte/multicast
- Avvia listener su porta unicast calcolata
- Prepara broadcast multicast per aggiornamenti
- Inizializza matrice 200x200 vuota

Output server tipico:
    === Game Server Initialized ===
    Unicast port: 7192
    Multicast address: 230.0.0.192:8888
    Server running...
    Waiting for clients on port 7192

AVVIO CLIENT:
    ./game_client <nickname> [server_ip]

Parametri:
    nickname   - Nome giocatore (max 15 caratteri)
    server_ip  - IP del server (opzionale, default localhost)

Esempi:
    ./game_client Player1              # Server locale
    ./game_client Alice 192.168.1.100 # Server remoto
    ./game_client Bob                  # Server localhost

CONTROLLI GIOCO:
    u = Movimento SU
    n = Movimento GIÙ  
    h = Movimento SINISTRA
    j = Movimento DESTRA
    q = Abbandona gioco

PROCEDURA TEST COMPLETA
-----------------------

1. PREPARAZIONE AMBIENTE:
   # Terminal 1 - Avvia server
   $ ./game_server
   
   Attendi output:
   === Game Server Initialized ===
   Unicast port: 7xxx
   Multicast address: 230.0.0.xxx:8888
   Server running...

2. CONNESSIONE CLIENT MULTIPLI:
   # Terminal 2 - Primo giocatore
   $ ./game_client Alice
   
   # Terminal 3 - Secondo giocatore  
   $ ./game_client Bob
   
   # Terminal 4 - Terzo giocatore
   $ ./game_client Charlie 192.168.1.100

3. GAMEPLAY:
   - Ogni client mostra la propria posizione e quella degli altri
   - Muovi il tuo giocatore con u/n/h/j
   - Cerca di collidere con altri giocatori per eliminarli
   - Evita di essere eliminato da altri
   - L'ultimo giocatore rimasto vince

4. MONITORAGGIO SERVER:
   Il server logga in tempo reale:
   Client 12345 (Alice) joined at (100,50)
   Client 12346 (Bob) joined at (75,120)
   Client 12345 moved to (101,50)
   Client 12346 eliminated
   Game ended! Winner: Client 12345

5. TERMINAZIONE:
   - Client: premi 'q' per quit pulito
   - Server: Ctrl+C per shutdown

LOGICA ELIMINAZIONE
-------------------

MECCANISMO COLLISIONE:
1. Client invia comando movimento al server
2. Server calcola nuova posizione target
3. Server verifica se posizione è occupata da altro client
4. Se occupata: client target viene eliminato
5. Client che si muove occupa la nuova posizione
6. Server notifica eliminazione via multicast

ESEMPIO SCENARIO:
• Alice in posizione (50,50)
• Bob in posizione (51,50)  
• Alice preme 'j' (destra) → target (51,50)
• Server rileva collisione Alice→Bob
• Bob viene eliminato e rimosso dalla matrice
• Alice si sposta in (51,50)
• Tutti i client ricevono notifica eliminazione Bob

CONDIZIONI VITTORIA:
• Partita termina quando rimane 1 solo giocatore attivo
• Server invia MSG_SERVER_GAME_END a tutti i client
• Vincitore riceve congratulazioni speciali
• Altri client vedono nome del vincitore

CONFIGURAZIONE RETE DETTAGLIATA
-------------------------------

CALCOLO AUTOMATICO INDIRIZZI:
Il sistema rileva automaticamente l'IP locale e ne estrae l'ultimo ottetto:

Esempio IP locale: 192.168.1.192
• Ultimo ottetto: 192
• Porta unicast server: 7192
• Indirizzo multicast: 230.0.0.192
• Porta multicast: 8888 (fissa)

Esempio IP locale: 10.0.0.50  
• Ultimo ottetto: 50
• Porta unicast server: 7050
• Indirizzo multicast: 230.0.0.50
• Porta multicast: 8888 (fissa)

GESTIONE MULTICAST:
• Server invia broadcast ogni 1 secondo
• Client si uniscono automaticamente al gruppo multicast
• Messaggi includono stato completo di tutti i giocatori
• Efficiente per sincronizzazione simultanea N client

FIREWALL E SICUREZZA:
• Aprire porta unicast 7xxx in ingresso per server
• Abilitare traffico multicast su gruppo 230.0.0.x
• Nessuna autenticazione implementata (ambiente fidato)
• Traffico non crittografato (gaming locale)

OUTPUT E RENDERING
------------------

Server output esempio:
    === Game Server Initialized ===
    Unicast port: 7192
    Multicast address: 230.0.0.192:8888
    Network setup complete
    Server running...
    
    Client 12345 (Alice) joined at (100,75)
    Client 12346 (Bob) joined at (150,125)
    Client 12345 moved to (101,75)
    Client 12346 moved to (149,125)
    Client 12345 moved to (102,75)
    
    Active clients: 2
    Client 12345 (Alice) at (102,75)
    Client 12346 (Bob) at (149,125)

Client output esempio:
    === Game Matrix ===
    Your position: (102,75) [ID: 12345]
    
    Other players:
      Bob [ID: 12346] at (149,125)
    
    Total players: 2
    
    Controls: u=up, n=down, h=left, j=right, q=quit

Notifiche speciali:
    === YOU HAVE BEEN ELIMINATED ===
    Eliminated by client 12346
    
    === GAME ENDED ===
    CONGRATULATIONS! YOU WON!

TROUBLESHOOTING
---------------

Errore "bind failed" server:
- Porta già in uso da altro processo
- Verificare: netstat -tulpn | grep 7xxx
- Attendere timeout kernel (2-4 minuti)
- Modificare IP locale per cambiare porta

Errore "multicast join failed" client:
- Sistema non supporta multicast
- Firewall blocca traffico gruppo 230.x.x.x
- Verificare configurazione interfaccia di rete
- Testare con: ping 230.0.0.xxx

Client non riceve aggiornamenti:
- Server e client usano indirizzi diversi (IP differenti)
- Verificare calcolo automatico ultimo ottetto
- Debug con: tcpdump -i any host 230.0.0.xxx
- Controllare routing multicast

Movimenti non registrati:
- Connessione unicast client→server fallita
- Server non raggiungibile su porta 7xxx
- Verificare connettività: telnet server_ip 7xxx
- Controllare timestamp messaggi

Performance degradate:
- Troppi client simultanei (>50)
- Latenza rete alta per multicast
- Sistema sotto carico (CPU/memoria)
- Ottimizzare frequenza aggiornamenti server

Terminale corrotto:
- Client terminato bruscamente senza cleanup
- Eseguire: reset oppure stty sane
- Restart terminale se necessario

PERSONALIZZAZIONE
-----------------

Costanti modificabili in game_protocol.h:

Dimensione matrice:
    #define MATRIX_SIZE 200        /* Default: 200x200 */

Numero massimo client:
    #define MAX_CLIENTS 100       /* Default: 100 */

Frequenza aggiornamenti server:
    #define SERVER_UPDATE_INTERVAL 1000000  /* 1 sec in μs */

Porte di rete:
    #define MULTICAST_PORT 8888    /* Porta multicast fissa */
    #define UNICAST_PORT_BASE 7000 /* Base per calcolo porta unicast */

Timeout e buffer:
    #define MAX_NICKNAME_LEN 16    /* Lunghezza nickname */

LIMITAZIONI
-----------

• Matrice virtuale (non renderizzata graficamente)
• Nessuna persistenza stato tra riavvii server
• Configurazione rete automatica (non manuale)
• Un solo server per volta per IP
• Nessuna autenticazione o sicurezza
• Rendering testuale semplificato
• Gestione errori di rete base

ESTENSIONI POSSIBILI
--------------------

• Rendering grafico 2D della matrice completa
• Persistenza punteggi e statistiche
• Autenticazione giocatori
• Chat tra giocatori
• Power-up e bonus speciali
• Modalità gioco alternative (team, time-limited)
• Replay delle partite
• Interfaccia web di amministrazione
• Support per NAT traversal

RIFERIMENTI
-----------

• Stevens, W. Richard. "Advanced Programming in the UNIX Environment"
• Stevens, W. Richard. "UNIX Network Programming, Volume 1"
• RFC 1112 - Internet Group Management Protocol (IGMP)
• RFC 3171 - IANA Guidelines for IPv4 Multicast Address Assignments

AUTORE E VERSIONE
-----------------

Implementazione basata su esempi libreria APUE
Compatibilità: Linux, macOS
Standard: ANSI C (C90)
Networking: UDP Unicast/Multicast
Gaming: Real-time multiplayer

================================================================================