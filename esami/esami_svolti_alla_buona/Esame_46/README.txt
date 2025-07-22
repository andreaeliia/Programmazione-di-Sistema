=================================================================
                    FILE MONITOR - README
=================================================================

DESCRIZIONE
-----------
Questo programma implementa un sistema di monitoraggio file con due 
thread che si alternano nella lettura e processamento di dati da un 
file in continua crescita (dump.txt). Il programma cerca sequenze 
di tre caratteri alfanumerici identici consecutivi.

FUNZIONALITÀ PRINCIPALI
-----------------------
• Due thread worker che si alternano nell'accesso al file
• Lettura progressiva dei nuovi dati aggiunti al file
• Ricerca di pattern: tre caratteri alfanumerici uguali consecutivi
• Output sincronizzato del progresso e dei risultati
• Supporto multipiattaforma (Linux/macOS)
• Azzeramento automatico dei byte già elaborati

PREREQUISITI
------------
• Compilatore GCC con supporto ANSI C (C90)
• Libreria APUE (Advanced Programming in the UNIX Environment)
• Sistema operativo: Linux o macOS
• OpenSSL (per generare i dati di test)

STRUTTURA FILES
---------------
file_monitor.c  - Codice sorgente principale
Makefile       - Script di compilazione
README.txt     - Questo file di documentazione
dump.txt       - File di input (generato automaticamente)

COMPILAZIONE
------------
Per compilare il programma utilizzare il Makefile fornito:

    make

Per compilare manualmente:
    
    Linux:
    gcc -ansi -Wall -I./include -pthread -DLINUX -D_GNU_SOURCE \
        file_monitor.c -o file_monitor -L./lib -lapueLinux -pthread
    
    macOS:
    gcc -ansi -Wall -I./include -pthread -DMACOS -D_DARWIN_SOURCE \
        file_monitor.c -o file_monitor -L./lib -lapueMacOS -pthread

UTILIZZO
--------
1. Avviare lo script bash per generare i dati:
   
   while true
   do
       openssl rand 1000 >> dump.txt
       sleep 1
   done
   
   Oppure utilizzare il Makefile:
   make test_data

2. In un altro terminale, avviare il programma:
   
   ./file_monitor

3. Il programma mostrerà:
   - Progresso della lettura (byte letti per thread)
   - Totale byte processati
   - Sequenze trovate con posizione nel file

4. Per terminare premere Ctrl+C

ESEMPIO OUTPUT
--------------
Avvio monitoraggio file dump.txt...
Premere Ctrl+C per terminare

[Thread 1] Letti 1024 bytes - Totale processati: 1024 bytes
[Thread 2] Letti 1024 bytes - Totale processati: 2048 bytes
[Thread 1] Letti 1024 bytes - Totale processati: 3072 bytes
*** [Thread 1] TROVATA SEQUENZA: 'aaa' alla posizione 1526 ***
[Thread 2] Letti 1024 bytes - Totale processati: 4096 bytes
*** [Thread 2] TROVATA SEQUENZA: '333' alla posizione 3890 ***

ARCHITETTURA DEL PROGRAMMA
--------------------------
Il programma è strutturato nelle seguenti componenti:

THREAD PRINCIPALE:
- Inizializza mutex e condition variables
- Crea due thread worker
- Gestisce apertura/chiusura del file

THREAD WORKER:
- Si alternano nell'accesso al file (Thread 1, Thread 2, Thread 1...)
- Leggono blocchi da 1024 byte
- Processano i dati cercando pattern
- Sincronizzano output e accesso al file

MECCANISMO DI ALTERNANZA:
- Condition variable per controllo turni
- Mutex per accesso esclusivo alle risorse condivise
- Sistema di segnalazione tra thread

PATTERN MATCHING:
- Ricerca sequenze di 3 caratteri alfanumerici identici
- Mantiene stato tra letture per pattern a cavallo dei buffer
- Gestisce continuità tra diverse letture

SINCRONIZZAZIONE
----------------
Il programma utilizza diversi meccanismi di sincronizzazione:

• file_mutex:  Accesso esclusivo al file
• print_mutex: Output sincronizzato su stdout
• turn_mutex:  Controllo alternanza thread
• turn_cond:   Segnalazione cambio turno

GESTIONE ERRORI
---------------
• Verifica apertura file
• Controllo creazione thread
• Gestione errori di sistema con funzioni APUE
• Pulizia risorse al termine

LIMITAZIONI
-----------
• Il file deve esistere prima dell'avvio del programma
• Non gestisce rotazione o troncamento del file
• Pattern di ricerca fisso (3 caratteri identici)
• Buffer size fisso (1024 byte)

COMANDI MAKEFILE
----------------
make          - Compila il programma
make clean    - Rimuove i file binari
make test_data - Avvia generazione dati di test
make stop_test - Ferma generazione dati di test

TROUBLESHOOTING
---------------
Problema: "Errore apertura file dump.txt"
Soluzione: Verificare che il file esista e sia leggibile

Problema: Errori di compilazione
Soluzione: Verificare che la libreria APUE sia installata

Problema: Il programma non trova sequenze
Soluzione: I dati casuali potrebbero non contenere pattern,
          attendere o generare più dati

=================================================================