SISTEMA MULTI-PROCESSO MULTI-THREAD SINCRONIZZATO
================================================

DESCRIZIONE:
-----------
Il sistema implementa una catena di comunicazione sincronizzata tra:
- P1: Processo con due thread (T1 e T2) 
- P2: Processo che riceve da T1 e condivide con P3
- P3: Processo che riceve da P2 via memoria condivisa e invia a T2

SEQUENZA OPERAZIONI:
-------------------
1. T1 genera stringa casuale e la invia a P2
2. P2 condivide la stringa con P3 tramite memoria condivisa  
3. P3 invia la stringa a T2
4. Il ciclo ricomincia quando T2 riceve la stringa
5. P1 stampa timestamp nanosecondi ad ogni completamento ciclo

COMPONENTI:
----------
- process_p1.c    : Processo P1 con thread T1 e T2
- process_p2.c    : Processo P2 (riceve da T1, condivide con P3)
- process_p3.c    : Processo P3 (legge memoria condivisa, invia a T2)
- sync_comm.h     : Header con definizioni comuni
- Makefile        : File di compilazione multipiattaforma

COMPILAZIONE:
------------
make all

Compila tutti i componenti del sistema.

CONFIGURAZIONE RETE:
-------------------
Il sistema è configurato per funzionare su loopback (127.0.0.1):
- P1 <-> P2: porta 8001
- P3 <-> P1: porta 8002
- P2 <-> P3: memoria condivisa con chiavi IPC

ESECUZIONE:
----------
1. Avviare P2 in un terminale:
   ./process_p2

2. Avviare P3 in un secondo terminale:
   ./process_p3

3. Avviare P1 in un terzo terminale:
   ./process_p1

L'ordine di avvio è importante per permettere le connessioni.

OUTPUT:
------
P1 stamperà timestamp con precisione nanosecondi per ogni ciclo:
Loop completed at: 1234567890.123456789 seconds
Loop completed at: 1234567890.234567890 seconds
...

SINCRONIZZAZIONE:
----------------
- Ogni step attende il completamento del precedente
- La memoria condivisa usa semafori per coordinare P2 e P3
- I socket TCP garantiscono consegna ordinata dei messaggi
- I thread T1 e T2 sono sincronizzati tramite mutex e condition variables

ARCHITETTURA:
------------
Machine A (simulata come localhost):
  P1 (T1 + T2) <--TCP--> P2 <--SHMEM--> P3
                    ^                    |
                    |<--------TCP--------|

PULIZIA:
-------
make clean

Rimuove tutti i file binari generati.

COMPATIBILITÀ:
-------------
- Standard C90 compliant
- Funziona su Linux e macOS
- Richiede libreria APUE
- Supporta pthread per multi-threading