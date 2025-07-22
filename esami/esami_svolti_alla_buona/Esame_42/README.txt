===============================================================================
                     SOCKET STAT ANALYSIS E SHARED MEMORY LIST
===============================================================================

DESCRIZIONE
-----------
Questo progetto contiene due esercizi distinti che utilizzano la libreria APUE:

ESERCIZIO 1 - Socket Stat Analysis (socket_stat_test.c)
-------------------------------------------------------
Programma che verifica quali elementi della struttura stat sono supportati
per file di tipo socket, confrontando le differenze tra macOS e GNU/Linux.

ESERCIZIO 2 - Shared Memory List (shm_writer.c + shm_reader.c)
--------------------------------------------------------------
Due programmi separati che implementano una lista condivisa in memoria:
- shm_writer: Crea la memoria condivisa e inserisce elementi nella lista
- shm_reader: Legge la memoria condivisa e stampa tutti gli elementi

CARATTERISTICHE
---------------
- Codice conforme allo standard C90
- Compatibilità con Linux e macOS
- Utilizzo della libreria APUE
- Gestione degli errori con funzioni APUE
- Struttura modulare con funzioni separate
- Documentazione completa nei commenti

COMPILAZIONE
------------
Per compilare tutti i programmi:

    make

Per compilare singolarmente:
    make socket_stat_test
    make shm_writer
    make shm_reader

Per pulire i file binari:

    make clean

UTILIZZO - ESERCIZIO 1
----------------------
Eseguire il programma di analisi socket:

    ./socket_stat_test

Il programma:
1. Crea un socket UNIX temporaneo in /tmp/test_socket
2. Analizza tutti i campi della struttura stat
3. Mostra una tabella con i campi supportati e i loro valori
4. Rimuove automaticamente il socket di test
5. Fornisce note specifiche per la piattaforma

Output esempio:
    Field           Supported  Value
    -----           ---------  -----
    st_mode         YES        0140000 (S_ISSOCK=YES)
    st_ino          YES        12345678
    st_dev          YES        2049
    ...

UTILIZZO - ESERCIZIO 2
----------------------
Passo 1 - Creare la lista (Writer):

    ./shm_writer

Il programma:
- Crea una memoria condivisa con chiave 0x1234
- Inserisce 10 numeri interi nella lista (10, 20, 30, ..., 100)
- Mostra informazioni sulla lista creata

Passo 2 - Leggere la lista (Reader):

    ./shm_reader

Il programma:
- Si connette alla memoria condivisa esistente
- Mostra informazioni sulla lista
- Stampa tutti gli elementi in ordine
- Offre l'opzione di rimuovere la memoria condivisa

STRUTTURA DATI - ESERCIZIO 2
----------------------------
La memoria condivisa contiene:

struct shm_header {
    int num_elements;        // Numero di elementi nella lista
    int first_element_offset; // Offset del primo elemento
};

struct list_element {
    int value;              // Valore intero dell'elemento
    int next_offset;        // Offset del prossimo elemento (-1 se ultimo)
};

La lista è implementata come lista collegata usando offset relativi
invece di puntatori per garantire la portabilità tra processi.

DIFFERENZE PIATTAFORMA - ESERCIZIO 1
------------------------------------
GNU/Linux:
- Supporta tutti i campi standard della struttura stat
- Include campi aggiuntivi: st_blksize, st_blocks
- st_rdev solitamente è 0 per i socket
- Timestamp più precisi

macOS:
- Supporta i campi base della struttura stat
- Non sempre include st_blksize e st_blocks
- Comportamento diverso per st_rdev
- Possibili differenze nei timestamp

Campi generalmente supportati su entrambe le piattaforme:
- st_mode (sempre supportato, identifica il tipo socket)
- st_ino (inode number)
- st_dev (device ID)
- st_uid, st_gid (proprietario e gruppo)
- st_nlink (numero di link)
- st_size (solitamente 0 per socket)
- st_atime, st_mtime, st_ctime (timestamp)

GESTIONE ERRORI
---------------
Entrambi i programmi utilizzano le funzioni APUE per la gestione degli errori:
- err_sys(): Per errori di sistema che terminano il programma
- err_ret(): Per errori non fatali che permettono la continuazione

PULIZIA RISORSE
---------------
Esercizio 1: Il socket temporaneo viene automaticamente rimosso
Esercizio 2: Il reader offre l'opzione di rimuovere la memoria condivisa

LIMITAZIONI
-----------
- Il socket di test è di tipo UNIX domain socket
- La lista può contenere massimo 10 elementi (configurabile)
- La chiave della memoria condivisa è fissa (0x1234)
- I valori nella lista sono numeri interi semplici

ESEMPI DI ESECUZIONE
--------------------
Terminal 1:
    $ ./shm_writer
    Creazione della lista in memoria condivisa...
    Aggiunta elementi alla lista:
      Aggiunto: 10
      Aggiunto: 20
      ...
    Lista creata con successo!

Terminal 2:
    $ ./shm_reader
    === SHARED MEMORY READER ===
    Connessione alla memoria condivisa...
    
    === INFORMAZIONI LISTA ===
    Numero totale di elementi: 10
    
    === CONTENUTO DELLA LISTA ===
    Elementi nella lista:
      Elemento 1: 100
      Elemento 2: 90
      ...

NOTE TECNICHE
-------------
- Utilizzo di System V IPC per la memoria condivisa
- Implementazione lista collegata con offset
- Gestione della concorrenza tra writer e reader
- Pulizia automatica delle risorse temporanee

COMPATIBILITÀ
-------------
Testato su:
- GNU/Linux con glibc
- macOS con standard C library
- Compilatori GCC compatibili con C90

DIPENDENZE
----------
- Libreria APUE (apue.h)
- System V IPC (sys/shm.h, sys/ipc.h)
- Socket UNIX (sys/socket.h, sys/un.h)
- Funzioni stat standard (sys/stat.h)

===============================================================================