=================================================================
    CONFRONTO PRESTAZIONI SERVER TCP: THREAD vs PROCESSI
=================================================================

DESCRIZIONE
-----------
Questo progetto implementa e confronta le prestazioni di due 
architetture di server TCP concorrenti:
1. Server basato su thread (pthread)
2. Server basato su processi figli (fork)

Il confronto viene effettuato misurando il numero di client 
serviti al secondo sotto diversi carichi di lavoro.

REQUISITI
---------
- Compilatore GCC con supporto C90/ANSI C
- Sistema operativo Linux o macOS
- Libreria APUE (Advanced Programming in Unix Environment)
- Libreria pthread (per il server basato su thread)

COMPILAZIONE
------------
1. Assicurarsi che le librerie APUE siano presenti in ./lib/
   - libapueLinux.a per Linux
   - libapueMacOS.a per macOS

2. Compilare tutti i programmi:
   make all

3. Per compilare singoli programmi:
   make server_thread
   make server_process
   make client_test
   make performance_test

ESECUZIONE DEI TEST
-------------------
1. Test manuale:
   
   Terminale 1 (Server Thread):
   ./server_thread 8080
   
   Terminale 2 (Server Process):
   ./server_process 8081
   
   Terminale 3 (Client):
   ./client_test localhost 8080
   ./client_test localhost 8081

2. Test automatico delle prestazioni:
   ./performance_test

   Il test automatico:
   - Avvia entrambi i server su porte diverse
   - Esegue test con diversi numeri di client (10, 50, 100, 200)
   - Misura i tempi di risposta
   - Calcola i client serviti al secondo
   - Presenta un confronto finale

STRUTTURA DEL CODICE
--------------------
- server_thread.c:  Server TCP con gestione thread
- server_process.c: Server TCP con gestione processi
- client_test.c:    Client per test manuali
- performance_test.c: Test automatico delle prestazioni

RISULTATI ATTESI
----------------
Generalmente si osserva che:
- I thread hanno overhead minore per la creazione
- I processi offrono maggiore isolamento e stabilita
- Le prestazioni dipendono dal carico e dalle risorse del sistema
- Su sistemi moderni, i thread tendono ad essere piu veloci
  per carichi medio-bassi

PULIZIA
-------
Per rimuovere i file compilati:
make clean

NOTE SULLA PORTABILITA
----------------------
Il codice e stato scritto per essere compatibile con:
- Standard C90/ANSI C
- Linux (testato su Ubuntu/CentOS)
- macOS (testato su versioni recenti)

Le differenze di piattaforma sono gestite attraverso
macro preprocessor e flag di compilazione specifici.

AUTORE
------
Progetto per il confronto delle prestazioni di server TCP