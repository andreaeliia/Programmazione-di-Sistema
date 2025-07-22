================================================================================
                    DETERMINAZIONE PATH STANDARD INPUT
================================================================================

DESCRIZIONE
-----------
Questo programma e' in grado di:
1. Determinare automaticamente il path del file associato allo standard input
2. Leggere e processare i dati provenienti dallo standard input
3. Fornire statistiche sui dati letti (righe, parole, caratteri)

Il programma funziona sia su sistemi Linux che macOS utilizzando approcci
specifici per ciascuna piattaforma.

COMPILAZIONE
------------
Per compilare il programma:

1. Assicurarsi di avere la libreria APUE installata
2. Utilizzare il Makefile fornito:
   
   make find_stdin_path

oppure compilare manualmente:

Su Linux:
   gcc -ansi -Wall -DLINUX -D_GNU_SOURCE find_stdin_path.c -o find_stdin_path -lapueLinux

Su macOS:
   gcc -ansi -Wall -DMACOS -D_DARWIN_SOURCE find_stdin_path.c -o find_stdin_path -lapueMacOS

UTILIZZO
--------
Il programma puo' essere utilizzato in diversi modi:

1. LETTURA DA FILE:
   ./find_stdin_path < nomefile.txt
   
   Esempio:
   echo "Questo e' un test" > test.txt
   ./find_stdin_path < test.txt

2. LETTURA DA PIPE:
   echo "Dati di test" | ./find_stdin_path
   
   cat file.txt | ./find_stdin_path

3. LETTURA DA TERMINALE:
   ./find_stdin_path
   
   (Digitare il testo e premere Ctrl+D per terminare)

FUNZIONALITA'
-------------
- DETERMINAZIONE PATH: Il programma tenta di determinare il path completo
  del file associato allo standard input utilizzando:
  * Su Linux: /proc/self/fd/0 (link simbolico)
  * Su macOS: proc_pidpath() o fcntl() con F_GETPATH

- LETTURA DATI: Legge tutti i dati dallo standard input e li visualizza

- STATISTICHE: Conta e visualizza:
  * Numero di righe
  * Numero di parole
  * Numero di caratteri totali

LIMITAZIONI
-----------
- Il path puo' essere determinato solo quando lo stdin e' associato a un file
  regolare
- Non funziona con terminali, pipe, socket o dispositivi speciali per la
  determinazione del path
- La determinazione del path dipende dalle caratteristiche del sistema operativo

STRUTTURA DEL CODICE
-------------------
- get_stdin_path(): Determina il path del file associato allo stdin
- process_stdin_data(): Legge e processa i dati dallo stdin
- main(): Funzione principale che coordina le operazioni

ESEMPI D'USO
------------
1. Test con file di testo:
   echo -e "Prima riga\nSeconda riga\nTerza riga" > esempio.txt
   ./find_stdin_path < esempio.txt

2. Test con pipe:
   ls -la | ./find_stdin_path

3. Test interattivo:
   ./find_stdin_path
   (Digitare alcune righe di testo e premere Ctrl+D)

OUTPUT ATTESO
-------------
Il programma produrra' un output simile a:

=== Determinazione automatica del path dello stdin ===

Path del file associato allo standard input:
/path/completo/al/file.txt

Lettura dati dallo standard input...
(Premere Ctrl+D per terminare l'input)

[contenuto del file]

--- Statistiche input ---
Righe: X
Parole: Y
Caratteri: Z

Programma terminato.

NOTE TECNICHE
-------------
- Il programma utilizza la libreria APUE per la gestione degli errori
- Compatibile con standard C90 (ANSI C)
- Utilizza system call specifiche del sistema operativo per la determinazione
  del path
- Gestisce correttamente i diversi tipi di input (file, pipe, terminale)

COMPILATORE E STANDARD
---------------------
- Standard: C90 (ANSI C)
- Compilatore: GCC
- Flag richiesti: -ansi -Wall
- Libreria: APUE (Advanced Programming in the UNIX Environment)

================================================================================