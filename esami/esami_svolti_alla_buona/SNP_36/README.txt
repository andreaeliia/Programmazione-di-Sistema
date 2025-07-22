PROGRAMMA CONTATORE FILE/DIRECTORY CLIENT-SERVER
================================================

DESCRIZIONE
-----------
Questo programma implementa un sistema client-server per contare file o directory
utilizzando comunicazione UDP. Il client (A) permette all'utente di scegliere se
contare file o directory, mentre il server (B) esegue il conteggio e restituisce
il risultato.

COMPONENTI
----------
- A.c      : Programma client che richiede il tipo di conteggio all'utente
- B.c      : Programma server che conta file o directory del sistema
- Makefile : File per la compilazione automatica
- README.txt : Questo file di documentazione

REQUISITI
---------
- Sistema operativo: Linux o macOS
- Compilatore: gcc con supporto C90 (-ansi)
- Libreria: apue.h (Advanced Programming in the UNIX Environment)
- Porte di rete: Il server utilizza la porta 12345 (modificabile nel codice)

COMPILAZIONE
------------
Il progetto include un Makefile che gestisce automaticamente la compilazione
per entrambe le piattaforme (Linux e macOS):

    make all

Oppure compilare singolarmente:
    make A
    make B

Per pulire i file binari:
    make clean

UTILIZ