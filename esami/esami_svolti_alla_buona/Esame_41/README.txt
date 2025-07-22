===============================================================================
                            SHELL ENHANCED
===============================================================================

DESCRIZIONE
-----------
Questo progetto è una modifica dell'esempio shell2.c di Stevens dal libro
"Advanced Programming in the UNIX Environment" (APUE). La shell è stata
estesa per supportare:

1. Pipe tra due comandi (comando1 | comando2)
2. Redirezione dell'output verso file (comando > file)
3. Esecuzione di comandi singoli

CARATTERISTICHE
---------------
- Compatibilità con Linux e macOS
- Codice conforme allo standard C90
- Gestione dei segnali (SIGINT)
- Parsing semplice delle linee di comando
- Gestione degli errori con funzioni APUE

COMPILAZIONE
------------
Per compilare il programma, utilizzare:

    make

Per pulire i file binari:

    make clean

UTILIZZO
--------
Eseguire il programma:

    ./shell_enhanced

La shell mostrerà un prompt "% " e accetterà i seguenti tipi di comandi:

1. Comando singolo:
   % ls
   % date
   % whoami

2. Pipe tra due comandi:
   % ls | wc
   % cat /etc/passwd | grep root
   % ps | head

3. Redirezione dell'output:
   % ls > output.txt
   % date > timestamp.txt
   % whoami > user.txt

LIMITAZIONI
-----------
- I comandi non possono accettare argomenti (come richiesto dalla traccia)
- Supporta solo pipe semplici tra due comandi
- Supporta solo redirezione dell'output (stdout)
- Non supporta redirezione dell'input o pipe multiple

STRUTTURA DEL CODICE
--------------------
Il programma è organizzato nelle seguenti funzioni:

- main(): Loop principale della shell
- sig_int(): Gestore del segnale SIGINT
- parse_command(): Analizza la linea di comando
- execute_single_command(): Esegue comandi singoli
- execute_pipe_command(): Esegue comandi con pipe
- execute_redirect_command(): Esegue comandi con redirezione
- print_prompt(): Stampa il prompt della shell

ESEMPI DI UTILIZZO
------------------
1. Contare i file nella directory corrente:
   % ls | wc -l

2. Salvare la lista dei processi in un file:
   % ps > processi.txt

3. Vedere i primi 10 file:
   % ls | head

4. Salvare la data corrente:
   % date > oggi.txt

COMPATIBILITÀ
-------------
Il programma è stato testato e funziona su:
- Linux (con flag -D_GNU_SOURCE)
- macOS (con flag -D_DARWIN_SOURCE)

DIPENDENZE
----------
- Libreria APUE (apue.h)
- Librerie standard UNIX/POSIX
- Compilatore GCC compatibile con C90

NOTE TECNICHE
-------------
- Utilizza fork() per creare processi figli
- Utilizza pipe() per la comunicazione tra processi
- Utilizza dup2() per la redirezione dei file descriptor
- Utilizza waitpid() per attendere la terminazione dei processi figli
- Gestisce correttamente la pulizia delle risorse

SEGNALI
-------
- SIGINT (Ctrl+C): Interrompe il comando corrente e mostra il prompt
- EOF (Ctrl+D): Termina la shell

AUTORE
------
Basato sull'esempio shell2.c di W. Richard Stevens
Modificato per supportare pipe e redirezione dell'output
===============================================================================