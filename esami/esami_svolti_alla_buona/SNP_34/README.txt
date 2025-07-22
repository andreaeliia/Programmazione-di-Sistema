================================================================================
                    ESERCIZI APUE - GESTIONE I/O E SEGNALI UNIX
================================================================================

DESCRIZIONE
-----------
Questo programma implementa due esercizi pratici basati sulla libreria APUE
(Advanced Programming in the UNIX Environment) per dimostrare concetti
fondamentali della programmazione di sistema Unix.

ESERCIZI IMPLEMENTATI
--------------------

Esercizio 1: Comportamento di 'ls' con pipe
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Analizza e dimostra come il comando 'ls' rileva automaticamente quando il suo
output è diretto verso una pipe anziché un terminale, utilizzando la funzione
isatty(). In questo caso, 'ls' passa automaticamente al formato una-per-riga
(equivalente all'opzione -1).

Concetti dimostrati:
- Uso della funzione isatty() per rilevare il tipo di output
- Differenza tra output verso terminale e verso pipe
- Comportamento automatico dei comandi Unix

Esercizio 2: Meccanismo 'nohup'
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Spiega e dimostra il funzionamento del comando 'nohup', che previene la
terminazione di un processo quando il terminale padre viene chiuso, ignorando
il segnale SIGHUP.

Concetti dimostrati:
- Gestione del segnale SIGHUP
- Creazione e controllo di processi figlio con fork()
- Comunicazione tra processi tramite segnali
- Meccanismo di funzionamento di nohup

REQUISITI DI SISTEMA
-------------------
- Sistema operativo: Linux o macOS
- Compilatore: GCC con supporto ANSI C (C90)
- Libreria APUE installata nel sistema
- Make per la compilazione

STRUTTURA DEI FILE
-----------------
esercizi_apue.c     - Codice sorgente principale
Makefile           - Script di compilazione
README.txt         - Questo file di documentazione

COMPILAZIONE
-----------
1. Assicurarsi di avere la libreria APUE installata:
   - Linux: libapueLinux nella directory ./lib/
   - macOS: libapueMacOS nella directory ./lib/
   - Header apue.h nella directory ./include/

2. Compilare con make:
   $ make

   Il Makefile rileva automaticamente il sistema operativo e usa i flag
   appropriati per Linux o macOS.

3. Per pulire i binari compilati:
   $ make clean

ESECUZIONE
----------
Lanciare il programma compilato:
$ ./esercizi_apue

Il programma presenta un menu interattivo con le seguenti opzioni:
1. Dimostrazione comportamento ls con pipe
2. Dimostrazione meccanismo nohup  
0. Esci

DETTAGLI TECNICI
---------------

Esercizio 1 - Analisi ls con pipe:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Il programma dimostra che 'ls' usa la funzione isatty(STDOUT_FILENO) per
determinare se l'output va verso un terminale o una pipe/file.

Comportamento di ls:
- Terminale: formato multi-colonna per leggibilità
- Pipe/File: formato una-per-riga (-1) per parsing

Test eseguiti:
- Controllo diretto con isatty() su stdout
- Confronto tra 'ls' normale e 'ls | cat'
- Spiegazione del meccanismo interno

Esercizio 2 - Meccanismo nohup:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Il programma simula il comportamento di nohup creando un processo figlio
e dimostrando la gestione del segnale SIGHUP.

Fasi simulate:
1. Creazione processo figlio con fork()
2. Installazione gestore per SIGHUP
3. Invio del segnale dal processo padre
4. Dimostrazione della terminazione/continuazione

Funzionamento reale di nohup:
- Ignora SIGHUP: signal(SIGHUP, SIG_IGN)
- Redirge stdout a nohup.out se è un terminale
- Redirge stderr a stdout
- Mantiene il processo attivo anche dopo chiusura terminale

FUNZIONI PRINCIPALI
------------------
main()                    - Menu principale e controllo flusso
esercizio1_demo_ls_pipe() - Dimostrazione comportamento ls
test_output_type()        - Test con isatty()
esercizio2_demo_nohup()   - Dimostrazione meccanismo nohup
sighup_handler()          - Gestore segnale SIGHUP
print_menu()              - Stampa menu di selezione

COMPATIBILITÀ
------------
Il codice è scritto in C90 (ANSI C) per massima portabilità e compatibilità
con sistemi Unix/Linux legacy. Utilizza le seguenti chiamate di sistema:

- fork() - Creazione processi
- signal() - Gestione segnali  
- kill() - Invio segnali
- waitpid() - Attesa processi figlio
- isatty() - Controllo tipo terminale
- sleep() - Pausa esecuzione

DIPENDENZE APUE
--------------
Il programma utilizza le seguenti funzioni dalla libreria APUE:
- err_sys() - Gestione errori con terminazione
- Header apue.h per definizioni e prototipi standard

NOTE DI SICUREZZA
----------------
- Il gestore del segnale SIGHUP usa write() che è signal-safe
- Viene usata una variabile volatile sig_atomic_t per la comunicazione
  tra gestore segnale e main program
- Gestione appropriata dei processi zombie con waitpid()

LIMITAZIONI
----------
- Le dimostrazioni sono semplificate per scopi didattici
- Il gestore SIGHUP include printf() che non è technically signal-safe
  (in produzione si dovrebbe usare solo write())
- Non gestisce tutti i casi edge di errore possibili

AUTORE E LICENZA
---------------
Implementazione basata sui concetti del libro "Advanced Programming in the
UNIX Environment" di W. Richard Stevens e Stephen A. Rago.

================================================================================