=================================================================
    TRASMISSIONE MULTICAST POSIZIONE SEGNAPOSTO MATRICE 20x20
=================================================================

DESCRIZIONE
-----------
Questo progetto implementa un sistema di trasmissione multicast
in tempo reale per condividere la posizione di un segnaposto
all'interno di una matrice 20x20 sulla rete locale.

Il sistema è composto da:
1. SENDER: Programma che permette all'utente di muovere un 
   segnaposto nella matrice usando i tasti freccia e trasmette
   la posizione in multicast
2. RECEIVER: Programma che riceve i messaggi multicast e mostra
   graficamente la posizione del segnaposto al terminale

REQUISITI
---------
- Compilatore GCC con supporto C90/ANSI C
- Sistema operativo Linux o macOS
- Libreria APUE (Advanced Programming in Unix Environment)
- Terminale con supporto per sequenze di escape ANSI
- Rete locale con supporto multicast

COMPILAZIONE
------------
1. Assicurarsi che le librerie APUE siano presenti in ./lib/
   - libapueLinux.a per Linux
   - libapueMacOS.a per macOS

2. Compilare entrambi i programmi:
   make all

3. Per compilare singoli programmi:
   make sender
   make receiver

ESECUZIONE
----------
1. Avviare il receiver su una o piu macchine:
   ./receiver

   Il receiver mostrera:
   - Una matrice 20x20 vuota inizialmente
   - Il segnaposto '*' nella posizione ricevuta via multicast
   - Informazioni sulla connessione multicast

2. Avviare il sender:
   ./sender

   Il sender mostrera:
   - Una matrice 20x20 con il segnaposto '*'
   - Istruzioni per il controllo
   - Posizione corrente del segnaposto

3. Controlli del sender:
   - Freccia SU    (o 'w'): Muovi il segnaposto in alto
   - Freccia GIU   (o 's'): Muovi il segnaposto in basso  
   - Freccia SINISTRA (o 'a'): Muovi il segnaposto a sinistra
   - Freccia DESTRA   (o 'd'): Muovi il segnaposto a destra
   - 'q' o ESC: Esci dal programma

4. I movimenti del segnaposto vengono trasmessi in tempo reale
   via multicast e visualizzati su tutti i receiver connessi.

CONFIGURAZIONE RETE
--------------------
Il sistema utilizza:
- Indirizzo multicast: 239.255.255.250
- Porta: 12345
- Protocollo: UDP

Questi parametri possono essere modificati nei file sorgente
se necessario per la propria configurazione di rete.

FORMATO MESSAGGIO
-----------------
I messaggi multicast contengono:
- Coordinata X (0-19)
- Coordinata Y (0-19)
- Timestamp del movimento
- Checksum per validazione

RISOLUZIONE PROBLEMI
--------------------
1. Se il receiver non riceve messaggi:
   - Verificare che il multicast sia supportato dalla rete
   - Controllare le impostazioni del firewall
   - Assicurarsi che sender e receiver siano sulla stessa rete

2. Se la visualizzazione e distorta:
   - Il terminale deve supportare sequenze ANSI
   - Ridimensionare il terminale se necessario
   - Su alcuni sistemi potrebbe essere necessario abilitare
     il supporto colori del terminale

3. Se i tasti freccia non funzionano:
   - Utilizzare i controlli alternativi (w,a,s,d)
   - Verificare le impostazioni del terminale

STRUTTURA DEL CODICE
--------------------
sender.c:
- setup_multicast_sender(): Configura socket multicast
- setup_terminal(): Configura terminale per input non-echo
- display_matrix(): Mostra la matrice al terminale
- handle_input(): Gestisce input da tastiera
- send_position(): Invia posizione via multicast

receiver.c:
- setup_multicast_receiver(): Configura ricezione multicast
- display_matrix(): Mostra la matrice ricevuta
- parse_position(): Decodifica messaggi ricevuti
- update_display(): Aggiorna visualizzazione in tempo reale

LIMITAZIONI
-----------
- La matrice e fissa a 20x20 caratteri
- Supporta un solo segnaposto per sessione
- Richiede terminale con supporto ANSI
- Funziona solo su reti con supporto multicast

PULIZIA
-------
Per rimuovere i file compilati:
make clean

COMPATIBILITA
-------------
Testato su:
- Linux (Ubuntu, CentOS, Debian)
- macOS (versioni recenti)
- Terminali: bash, zsh, Terminal.app, xterm

AUTORE
------
Programma per dimostrazione trasmissione multicast
in tempo reale con interfaccia grafica testuale.