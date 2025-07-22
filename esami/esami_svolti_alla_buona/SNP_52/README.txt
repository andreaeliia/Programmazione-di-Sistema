================================================================================
                    SIMULAZIONE INCROCIO A 4 VIE - README
================================================================================

DESCRIZIONE
-----------
Questo programma simula il funzionamento di un incrocio stradale a 4 vie
utilizzando processi multipli e memoria condivisa. La simulazione include:

- 4 processi che rappresentano le 4 strade dell'incrocio
- 1 processo controller che gestisce il semaforo dell'incrocio  
- Generazione casuale di veicoli su ciascuna strada
- Controllo dell'accesso esclusivo all'incrocio
- Tempo fisso di attraversamento (3 secondi per veicolo)

SPECIFICHE TECNICHE
-------------------
- Standard: C90 (ANSI C)
- Libreria: apue.h (Advanced Programming in the UNIX Environment)
- IPC: Memoria condivisa e semafori System V
- Compatibilità: Linux e macOS
- Processi: 5 processi totali (4 strade + 1 controller)

STRUTTURA DEL PROGETTO
----------------------
crossroads.c    - Codice sorgente principale
Makefile        - File di compilazione per Linux/macOS
README.txt      - Questo file di documentazione

COMPILAZIONE
------------
Assicurarsi di avere:
1. La libreria apue.h installata nella directory include/
2. I file di libreria appropriati in lib/ (apueLinux.a o apueMacOS.a)

Compilare con:
    make

Per pulire i file binari:
    make clean

ESECUZIONE
----------
Dopo la compilazione, eseguire:
    ./crossroads

Il programma mostrerà:
- Stato iniziale della simulazione
- Arrivo dei veicoli sulle varie strade
- Movimento dei veicoli attraverso l'incrocio
- Statistiche finali

Per terminare premere Ctrl+C in qualsiasi momento.

FUNZIONAMENTO
-------------
1. GENERAZIONE VEICOLI
   - Ogni strada genera veicoli casualmente
   - Probabilità di arrivo: 30% ogni 1-3 secondi
   - Massimo 10 veicoli in coda per strada
   - Limite totale: 50 veicoli

2. CONTROLLO INCROCIO
   - Solo un veicolo alla volta può attraversare
   - Attraversamento richiede esattamente 3 secondi
   - Selezione sequenziale circolare delle strade (0→1→2→3→0...)
   - Accesso garantito in base all'ordine di arrivo per strada

3. SINCRONIZZAZIONE
   - Semaforo mutex per accesso ai dati condivisi
   - Semaforo per controllo dell'incrocio
   - Memoria condivisa per stato globale della simulazione

4. IDENTIFICAZIONE STRADE
   - Strada 0: Nord
   - Strada 1: Est  
   - Strada 2: Sud
   - Strada 3: Ovest

OUTPUT SIMULAZIONE
------------------
Il programma mostra periodicamente:
- Stato dell'incrocio (libero/occupato)
- Numero di veicoli in coda per ogni strada
- Veicoli creati e attraversati
- Notifiche di arrivo e attraversamento veicoli

STRUTTURE DATI PRINCIPALI
--------------------------
- Vehicle: rappresenta un singolo veicolo (ID + strada origine)
- RoadQueue: coda FIFO per veicoli di una strada
- SharedData: struttura in memoria condivisa con stato globale

GESTIONE ERRORI
---------------
- Controllo di tutti i valori di ritorno delle system call
- Cleanup automatico delle risorse IPC
- Gestione segnali per terminazione controllata
- Messaggi di errore descrittivi tramite apue.h

LIMITAZIONI
-----------
- Numero fisso di strade (4)
- Dimensione fissa delle code (10 veicoli per strada)
- Tempo di attraversamento fisso (3 secondi)
- Limite massimo veicoli totali (50)
- Selezione strada con algoritmo round-robin semplice

CLEANUP RISORSE
---------------
Il programma pulisce automaticamente:
- Memoria condivisa
- Set di semafori
- Processi figli

Anche in caso di terminazione forzata (Ctrl+C), il cleanup viene eseguito
tramite il gestore di segnali.

CODICI DI USCITA
----------------
0: Simulazione completata con successo
1: Errore durante l'inizializzazione o esecuzione

REQUISITI SISTEMA
-----------------
- Sistema operativo: Linux o macOS
- Compilatore: GCC con supporto C90
- Libreria APUE installata e configurata
- Permessi per creazione risorse IPC

================================================================================
Per domande o problemi, consultare la documentazione della libreria APUE
================================================================================