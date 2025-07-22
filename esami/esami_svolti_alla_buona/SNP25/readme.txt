================================================================================
        CONTROLLO LAMPADINE CON SINCRONIZZAZIONE DI PROCESSI
================================================================================

DESCRIZIONE GENERALE
====================

Questo progetto implementa un sistema di controllo per due lampadine (rossa e 
verde) utilizzando processi concorrenti e meccanismi di sincronizzazione. 
Il sistema confronta le prestazioni tra due diverse tecniche:
1. Semafori System V
2. Record Locking

ARCHITETTURA DEL SISTEMA
========================

Il sistema è composto da tre processi principali:

- PROCESSO A (Server): Gestisce le richieste di accensione/spegnimento delle
  lampadine. Garantisce che solo una lampadina sia accesa per volta.

- PROCESSO B (Client Rosso): Invia richieste per accendere la lampadina rossa
  per la durata di un secondo.

- PROCESSO C (Client Verde): Invia richieste per accendere la lampadina verde
  per la durata di un secondo.

API DISPONIBILI
===============

Le seguenti funzioni sono implementate per il controllo delle lampadine:

- on_red()    : Accende la lampadina rossa
- off_red()   : Spegne la lampadina rossa  
- on_green()  : Accende la lampadina verde
- off_green() : Spegne la lampadina verde

VINCOLI DEL SISTEMA
===================

1. MUTUA ESCLUSIONE: Solo una lampadina può essere accesa contemporaneamente
2. DURATA: Ogni lampadina rimane accesa per esattamente 1 secondo
3. VELOCITA': I cicli di accensione/spegnimento devono essere il più rapidi 
   possibile
4. FAIRNESS: Entrambe le lampadine devono avere accesso equo alle risorse

FILES DEL PROGETTO
==================

semaphore_version.c     - Implementazione con Semafori System V
record_locking_version.c - Implementazione con Record Locking
Makefile               - Script di compilazione multi-piattaforma
README.txt             - Questo file di documentazione

COMPILAZIONE
============

Il progetto usa un Makefile compatibile con Linux e Mac OS.

Compilazione di entrambe le versioni:
    make all

Compilazione versione specifica:
    make semaphore_version
    make record_locking_version

Test di entrambe le versioni:
    make test

Pulizia dei file compilati:
    make clean

Informazioni di sistema:
    make info

REQUISITI DI SISTEMA
====================

- Compilatore: GCC con supporto ANSI C (C90)
- Sistema: Linux o Mac OS
- Librerie: apue.h (Advanced Programming in UNIX Environment)
- Permessi: Capacità di creare semafori System V e file temporanei

DETTAGLI IMPLEMENTAZIONE
========================

VERSIONE SEMAFORI SYSTEM V
---------------------------

Utilizza un set di 3 semafori:
- Semaforo 0: Richieste lampadina rossa
- Semaforo 1: Richieste lampadina verde  
- Semaforo 2: Mutex per accesso esclusivo

Vantaggi:
+ Comunicazione veloce tra processi
+ Meccanismo nativo del kernel
+ Basso overhead per operazioni semplici

Svantaggi:
- Richiede pulizia esplicita dei semafori
- Limitato numero di semafori di sistema
- Possibili problemi di persistenza

VERSIONE RECORD LOCKING
-----------------------

Utilizza file locking con regioni specifiche:
- Regione 0: Lock lampadina rossa
- Regione 1: Lock lampadina verde
- Regione 2: Mutex generale
- Regione 3: Lock per file richieste

Vantaggi:
+ Più portabile tra sistemi UNIX
+ Pulizia automatica alla terminazione del processo
+ Nessun limite di sistema sui lock

Svantaggi:
- Overhead maggiore per operazioni su file
- Dipendenza dal filesystem
- Possibili problemi con NFS

ESECUZIONE
==========

Per eseguire la versione con semafori:
    ./semaphore_version

Per eseguire la versione con record locking:
    ./record_locking_version

Output tipico:
- Messaggi di avvio dei processi
- Log delle richieste inviate
- Stato delle lampadine (ACCESA/SPENTA)
- Statistiche finali con tempo di esecuzione

ANALISI DELLE PRESTAZIONI
==========================

Il programma misura automaticamente il tempo di esecuzione per confrontare
le prestazioni dei due meccanismi. Tipicamente:

- Semafori System V: Più veloci per operazioni semplici
- Record Locking: Overhead maggiore ma più robusto

I risultati dipendono da:
- Carico del sistema
- Tipo di filesystem (per record locking)
- Numero di processi concorrenti
- Frequenza delle richieste

GESTIONE ERRORI
===============

Il programma include gestione completa degli errori:

- Controllo fallimenti fork()
- Gestione errori semafori/file locking
- Pulizia risorse tramite signal handler
- Controllo integrità operazioni I/O

In caso di errore, il programma:
1. Stampa messaggio di errore descrittivo
2. Esegue pulizia delle risorse allocate
3. Termina con codice di uscita appropriato

PULIZIA RISORSE
===============

AUTOMATICA:
- Signal handler per SIGINT e SIGTERM
- Pulizia alla terminazione normale
- Rimozione automatica file temporanei

MANUALE:
- Comando "make clean" rimuove binari
- Rimozione manuale semafori: ipcrm -s <semid>
- Rimozione file lock: rm *.lock *.dat

TROUBLESHOOTING
===============

Problema: "semget error: No space left on device"
Soluzione: Rimuovere semafori orfani con: ipcs -s; ipcrm -s <semid>

Problema: "Permission denied" sui file lock
Soluzione: Verificare permessi directory corrente

Problema: Processi zombie
Soluzione: Verificare che wait() sia chiamato correttamente

Problema: Deadlock apparente
Soluzione: Verificare ordine acquisizione lock, usare timeout

LIMITAZIONI
===========

1. Numero fisso di richieste (5 per lampadina) per scopi dimostrativi
2. Temporizzazione fissa di 1 secondo per accensione
3. Non implementa priorità tra richieste
4. Nessuna persistenza dello stato tra esecuzioni

ESTENSIONI POSSIBILI
====================

- Implementazione con memoria condivisa POSIX
- Aggiunta di più lampadine/colori
- Sistema di priorità per le richieste  
- Interfaccia di configurazione runtime
- Logging su file delle operazioni
- Interfaccia grafica per visualizzazione stato

CONFORMITA' STANDARD
=====================

Il codice è conforme a:
- Standard ANSI C (C90)
- POSIX.1 per system call
- Single UNIX Specification v3
- Compatibile GNU/Linux e BSD

AUTORE E LICENZA
================

Implementazione di esempio per corso di Sistemi Operativi
Basato su esempi da "Advanced Programming in the UNIX Environment"

Questo codice è fornito per scopi educativi.

================================================================================
                              FINE DOCUMENTAZIONE
================================================================================