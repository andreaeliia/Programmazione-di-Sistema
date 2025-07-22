=================================================================
            COMMAND LINE ACCESS - README
=================================================================

DESCRIZIONE
-----------
Questo programma dimostra come una funzione può accedere agli argomenti
della linea di comando SENZA che argc e argv vengano passati come
parametri della funzione e SENZA utilizzare variabili globali.

La soluzione sfrutta le interfacce fornite dal sistema operativo per
interrogare le informazioni del processo corrente e recuperare gli
argomenti originali della command line.

PROBLEMA RISOLTO
----------------
Domanda: "C'è un modo, per una funzione chiamata dal main, di esaminare 
gli argomenti passati sulla linea di comando del programma senza:
a) passare argc e argv come argomenti della chiamata della funzione
b) copiare argc e argv in una variabile globale?"

Risposta: SÌ, è possibile utilizzando le interfacce del sistema operativo.

APPROCCIO TECNICO
-----------------
Il programma implementa due strategie diverse a seconda del sistema:

LINUX:
• Utilizza il file /proc/self/cmdline
• Questo file contiene gli argomenti separati da byte null
• Accessibile in sola lettura da qualsiasi processo

MACOS:
• Utilizza la syscall sysctl() con KERN_PROCARGS2
• Richiede libproc per accedere alle informazioni del processo
• Restituisce una struttura con argc e argv del processo

PREREQUISITI
------------
• Compilatore GCC con supporto ANSI C (C90)
• Libreria APUE (Advanced Programming in the UNIX Environment)
• Sistema operativo: Linux o macOS
• Su macOS: accesso a libproc (generalmente disponibile)

STRUTTURA FILES
---------------
cmdline_access.c - Codice sorgente principale
Makefile        - Script di compilazione con test
README.txt      - Questo file di documentazione

COMPILAZIONE
------------
Utilizzare il Makefile fornito:

    make

Per compilazione manuale:

    Linux:
    gcc -ansi -Wall -I./include -DLINUX -D_GNU_SOURCE \
        cmdline_access.c -o cmdline_access -L./lib -lapueLinux
    
    macOS:
    gcc -ansi -Wall -I./include -DMACOS -D_DARWIN_SOURCE \
        cmdline_access.c -o cmdline_access -L./lib -lapueMacOS

UTILIZZO
--------
Eseguire il programma con diversi argomenti per vedere la dimostrazione:

    ./cmdline_access
    ./cmdline_access hello world
    ./cmdline_access "argomento con spazi" -flag 123 3.14
    
Il programma mostrerà:
1. Gli argomenti ricevuti normalmente nel main()
2. Gli stessi argomenti recuperati dalla funzione senza parametri
3. Un'analisi statistica degli argomenti

ESEMPI DI ESECUZIONE
--------------------

Esempio 1 - Nessun argomento aggiuntivo:
$ ./cmdline_access

=== Dimostrazione accesso argomenti command line ===
Programma avviato con 1 argomenti:
  argv[0] = "./cmdline_access"

--- Ora la funzione accederà agli argomenti SENZA riceverli ---

Funzione chiamata SENZA argc e argv...
Sistema: Linux - Usando /proc/self/cmdline
Apertura /proc/self/cmdline...
Letti 18 bytes da /proc/self/cmdline
Estratti 1 argomenti
✓ Successo! Argomenti recuperati:

Argomenti recuperati dalla funzione (argc = 1):
  [0] = "./cmdline_access"

Esempio 2 - Con argomenti:
$ ./cmdline_access hello world 123

=== Dimostrazione accesso argomenti command line ===
Programma avviato con 4 argomenti:
  argv[0] = "./cmdline_access"
  argv[1] = "hello"
  argv[2] = "world"
  argv[3] = "123"

--- Ora la funzione accederà agli argomenti SENZA riceverli ---

Funzione chiamata SENZA argc e argv...
Sistema: Linux - Usando /proc/self/cmdline
✓ Successo! Argomenti recuperati:

Argomenti recuperati dalla funzione (argc = 4):
  [0] = "./cmdline_access"
  [1] = "hello"
  [2] = "world"
  [3] = "123"

=== ANALISI ARGOMENTI ===
Numero totale di argomenti: 4
Nome programma: ./cmdline_access
Argomenti utente: 3
Caratteri totali: 35
Argomenti numerici: 1

TEST AUTOMATICI
---------------
Il Makefile include diversi test automatici:

    make test1  - Test senza argomenti aggiuntivi
    make test2  - Test con argomenti semplici
    make test3  - Test con argomenti complessi (spazi, flag, numeri)
    make test4  - Test con molti argomenti
    make test_all - Esegue tutti i test in sequenza

ARCHITETTURA DEL PROGRAMMA
--------------------------

FUNZIONI PRINCIPALI:

main():
• Mostra gli argomenti ricevuti normalmente
• Chiama demonstrate_access_without_params() SENZA passare argc/argv

demonstrate_access_without_params():
• Funzione che deve recuperare gli argomenti da sola
• Chiama le funzioni specifiche per il sistema operativo

get_cmdline_args_linux():
• Legge /proc/self/cmdline
• Parsing degli argomenti separati da null-byte

get_cmdline_args_macos():
• Utilizza sysctl() con KERN_PROCARGS2
• Estrae argc e argv dalla struttura kernel

parse_cmdline_buffer():
• Analizza il buffer con gli argomenti
• Separa gli argomenti basandosi sui null-byte

analyze_arguments():
• Fornisce statistiche sugli argomenti recuperati
• Conta argomenti numerici, lunghezze, ecc.

DETTAGLI IMPLEMENTATIVI
-----------------------

LINUX (/proc/self/cmdline):
• File virtuale contenente gli argomenti del processo
• Formato: arg0\0arg1\0arg2\0...
• Sempre accessibile per il processo corrente
• Richiede parsing manuale dei null-byte

MACOS (sysctl KERN_PROCARGS2):
• Syscall che restituisce informazioni complete del processo
• Include argc, nome eseguibile e argomenti
• Formato più complesso che richiede navigazione dei dati
• Necessita di gestione del padding e dei separatori

LIMITAZIONI
-----------
• Su Linux: dipende dalla disponibilità di /proc filesystem
• Su macOS: richiede permessi per accedere a sysctl
• Gli argomenti molto lunghi potrebbero essere troncati
• La dimensione massima degli argomenti è limitata dai buffer
• Non gestisce modifiche dinamiche agli argomenti durante l'esecuzione

CONSIDERAZIONI DI SICUREZZA
---------------------------
• Il metodo espone gli argomenti della command line
• Altri processi potrebbero vedere gli stessi dati
• Non adatto per password o informazioni sensibili
• Su sistemi multi-user potrebbero esserci restrizioni

VANTAGGI DELLA SOLUZIONE
------------------------
• Non richiede modifiche alla signature delle funzioni
• Non utilizza variabili globali
• Mantiene l'incapsulamento del codice
• Funziona su più piattaforme

SVANTAGGI DELLA SOLUZIONE
-------------------------
• Dipendente dal sistema operativo
• Meno efficiente dell'accesso diretto
• Maggiore complessità del codice
• Possibili problemi di portabilità

CONCLUSIONE
-----------
È tecnicamente possibile per una funzione accedere agli argomenti
della command line senza riceverli come parametri, utilizzando le
interfacce del sistema operativo. Tuttavia, questo approccio è più
complesso e meno efficiente rispetto ai metodi tradizionali.

La soluzione dimostra la flessibilità dei sistemi Unix/Linux nel
fornire accesso alle informazioni dei processi attraverso interfacce
standard come /proc e sysctl.

=================================================================