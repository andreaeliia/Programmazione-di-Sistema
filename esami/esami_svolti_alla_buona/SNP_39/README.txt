================================================================================
                              MIRROR DAEMON
                     Sistema di Mirroring Automatico con Threading
================================================================================

DESCRIZIONE
-----------
Il Mirror Daemon è un programma che esegue il mirroring periodico e automatico
di una directory sorgente verso una directory destinazione. Il sistema copia
solo i file che sono stati modificati dall'ultimo backup, ottimizzando le 
performance.

Il programma supporta due modalità operative:
- Single-thread: processamento sequenziale dei file
- Multi-thread: processamento parallelo con pool di thread worker

CARATTERISTICHE PRINCIPALI
--------------------------
* Daemon automatico con esecuzione in background
* Scansione ricorsiva delle directory
* Copia incrementale (solo file modificati)
* Supporto multi-threading per performance migliorate  
* Logging completo delle operazioni
* Gestione segnali per terminazione pulita
* Preservazione timestamp e permessi file
* Creazione automatica directory destinazione
* Misurazione performance per confronto modalità

COMPILAZIONE
------------
Il programma utilizza il Makefile fornito che supporta sia Linux che macOS:

1. Aggiungere 'mirror_daemon' alla variabile PROGS nel Makefile:
   PROGS = mirror_daemon

2. Compilare:
   make mirror_daemon

3. Per pulire i binari:
   make clean

REQUISITI SISTEMA  
-----------------
* Compilatore GCC con supporto C90/ANSI C
* Libreria APUE (Advanced Programming in the UNIX Environment)
* Sistema Unix-like (Linux o macOS)
* Libreria pthread per supporto multi-threading
* Header sys/stat.h, dirent.h, time.h

USO DEL PROGRAMMA
-----------------
Sintassi:
./mirror_daemon <directory_sorgente> <directory_destinazione> [modalità_thread]

Parametri:
- directory_sorgente: percorso della directory da monitorare
- directory_destinazione: percorso dove creare il mirror
- modalità_thread: 0 = single-thread, 1 = multi-thread (opzionale, default: 0)

Esempi:
./mirror_daemon /home/user/documenti /backup/documenti
./mirror_daemon /var/www/html /backup/www 1

FUNZIONAMENTO
-------------
1. Il programma si daemonizza automaticamente all'avvio
2. Ogni 30 secondi esegue una scansione della directory sorgente
3. Identifica file nuovi o modificati confrontando timestamp e dimensioni
4. Copia solo i file che necessitano aggiornamento
5. Mantiene struttura directory e permessi originali
6. Registra tutte le operazioni nei log di sistema

MODALITÀ OPERATIVE
------------------
SINGLE-THREAD (modalità 0):
- Processamento sequenziale file per file
- Minore utilizzo memoria e CPU
- Adatta per sistemi con risorse limitate
- Tempo di esecuzione più lungo per molti file

MULTI-THREAD (modalità 1):
- Processamento parallelo con pool di thread worker
- Maggiore utilizzo risorse ma performance superiori
- Adatta per sistemi con più CPU/core
- Ottimale per grandi volumi di dati

LOGGING
-------
Il daemon registra le attività in due modi:

1. File di log: /tmp/mirror_daemon.log
   - Timestamp dettagliati
   - Statistiche operazioni (file processati, tempo impiegato)
   - Messaggi di stato ed errori

2. Syslog di sistema:
   - Integrazione con logging sistema operativo  
   - Consultabile con 'journalctl' (Linux) o 'log show' (macOS)

GESTIONE SEGNALI
----------------
Il daemon gestisce i seguenti segnali:

SIGTERM / SIGINT: Terminazione pulita del daemon
SIGHUP: Ricaricamento configurazione (placeholder per estensioni future)
SIGPIPE: Ignorato per evitare terminazione accidentale

Per terminare il daemon:
kill -TERM `pgrep mirror_daemon`

STRUTTURA CODICE
----------------
Il programma è organizzato nelle seguenti funzioni principali:

main(): Gestione argomenti e loop principale del daemon
daemonize(): Conversione processo in daemon
scan_directory(): Scansione ricorsiva directory e costruzione lista file
need_update(): Verifica se file necessita aggiornamento
copy_file(): Copia fisica del file con preservazione metadati
perform_mirror_single(): Mirroring sequenziale single-thread  
perform_mirror_threaded(): Mirroring parallelo multi-thread
worker_thread(): Funzione eseguita dai thread worker
log_message(): Sistema di logging unificato

PERFORMANCE E OTTIMIZZAZIONI
----------------------------
Il sistema implementa diverse ottimizzazioni:

* Copia incrementale: solo file modificati vengono processati
* Buffer I/O ottimizzato (8KB) per efficienza trasferimento dati
* Pooling thread per evitare overhead creazione/distruzione
* Mutex per sincronizzazione thread-safe delle statistiche
* Verifica preliminare esistenza/timestamp per evitare copie inutili

LIMITAZIONI
-----------
* I file possono essere solo modificati o aggiunti, mai rimossi
* Dimensione massima path: 1024 caratteri  
* Numero massimo thread worker: 10
* Intervallo fisso controllo: 30 secondi
* Buffer copia file: 8KB

RISOLUZIONE PROBLEMI
--------------------
Problemi comuni e soluzioni:

1. "Impossibile accedere alla directory": Verificare permessi lettura
2. "Errore creazione directory": Controllare permessi scrittura destinazione  
3. Daemon non si avvia: Verificare disponibilità porta/risorse sistema
4. File non copiati: Controllare spazio disco e permessi file
5. Performance scadenti: Provare modalità multi-thread o ottimizzare I/O disco

Per debug dettagliato consultare:
- /tmp/mirror_daemon.log
- Log di sistema (syslog/journald)

SICUREZZA
---------
* Il daemon cambia directory di lavoro su "/" per sicurezza
* Chiude tutti i file descriptor non necessari
* Gestione sicura dei segnali
* Validazione input parametri
* Controllo errori su tutte le operazioni I/O

================================================================================
Autore: Sistema Mirror Daemon
Versione: 1.0
Licenza: Per uso educativo - Corso Programmazione di Sistema
================================================================================