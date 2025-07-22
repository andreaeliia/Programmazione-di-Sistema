=============================================================================== CLIENT-SERVER PER RICERCA FILE JPEG E PNG
DESCRIZIONE
Questo progetto implementa un sistema client-server che permette la ricerca
ricorsiva di file JPEG e PNG nella home directory dell'utente che esegue il
server.

Il client invia richieste al server specificando il tipo di file da cercare
("JPEG" o "PNG"), e il server risponde con l'elenco completo dei percorsi
dei file trovati che corrispondono al tipo richiesto.

FUNZIONALITÀ
Ricerca ricorsiva di file JPEG (magic number: FFD8)
Ricerca ricorsiva di file PNG (magic number: 89504E470D0A1A0A)
Comunicazione client-server tramite socket TCP
Gestione multipla di client tramite fork()
Identificazione dei file basata sui magic numbers (non sull'estensione)
Compatibilità con Linux e macOS
COMPILAZIONE
Per compilare il progetto utilizzare il comando:

make
oppure per compilare singolarmente:

make client
make server
Per pulire i file binari:

make clean
UTILIZZO
AVVIO DEL SERVER Aprire un terminale ed eseguire:
./server
Il server si avvierà sulla porta 8080 e rimarrà in ascolto per le connessioni dei client.
AVVIO DEL CLIENT Aprire un secondo terminale ed eseguire:
./client
Il client mostrerà un menu interattivo dove è possibile:
Digitare "JPEG" per cercare file JPEG
Digitare "PNG" per cercare file PNG
Digitare "quit" per uscire
ESEMPIO DI UTILIZZO
Esempio di sessione client:

Client per ricerca file JPEG/PNG
Digita 'JPEG' per cercare file JPEG
Digita 'PNG' per cercare file PNG
Digita 'quit' per uscire

Inserisci richiesta: JPEG
Risultati ricevuti dal server:
===============================
/home/user/Pictures/photo1.jpg
/home/user/Documents/image.jpeg
===============================

Inserisci richiesta: PNG
Risultati ricevuti dal server:
===============================
/home/user/Pictures/screenshot.png
/home/user/Downloads/icon.png
===============================

Inserisci richiesta: quit
Client terminato.
ARCHITETTURA
Il progetto è composto da due file principali:

CLIENT.C:

Gestisce l'interfaccia utente
Si connette al server per ogni richiesta
Invia la richiesta ("JPEG" o "PNG")
Riceve e visualizza i risultati
SERVER.C:

Resta in ascolto sulla porta 8080
Gestisce connessioni multiple tramite fork()
Esegue ricerca ricorsiva nella home directory
Identifica i file tramite magic numbers
Invia i percorsi trovati al client
DETTAGLI TECNICI
Standard: C90 (ANSI C)
Librerie utilizzate: apue.h
Socket TCP per la comunicazione
Identificazione file basata su magic numbers:
JPEG: 0xFF 0xD8
PNG: 0x89 0x50 0x4E 0x47 0x0D 0x0A 0x1A 0x0A
Ricerca ricorsiva tramite opendir/readdir
Gestione errori con funzioni err_sys() da apue.h
REQUISITI DI SISTEMA
Sistema operativo: Linux o macOS
Compilatore: GCC con supporto C90
Libreria APUE installata e configurata
Directory include/ con apue.h
Directory lib/ con libreria apue appropriata (apueLinux o apueMacOS)
FILE INCLUSI
client.c : Codice sorgente del client
server.c : Codice sorgente del server
Makefile : File per la compilazione
README.txt : Questo file di documentazione
NOTE
Il server cerca esclusivamente nella home directory dell'utente
La ricerca è basata sui magic numbers, non sulle estensioni dei file
Ogni connessione client viene gestita in un processo separato
Il server rimane attivo fino alla terminazione manuale (Ctrl+C)
Il client termina automaticamente dopo ogni ricerca (una connessione per richiesta)
LIMITAZIONI
La porta del server (8080) è fissa nel codice
La ricerca avviene solo nella home directory
Non c'è autenticazione o controllo degli accessi
I magic numbers sono controllati solo all'inizio del file
RISOLUZIONE PROBLEMI
Se il server non si avvia:

Verificare che la porta 8080 sia libera
Controllare i permessi di lettura sulla home directory
Se il client non si connette:

Verificare che il server sia in esecuzione
Controllare che l'IP del server sia corretto (127.0.0.1)
Se la compilazione fallisce:

Verificare che la libreria APUE sia installata
Controllare che le directory include/ e lib/ siano presenti
Verificare che il compilatore GCC sia installato
===============================================================================

