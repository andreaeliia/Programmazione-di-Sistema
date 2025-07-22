BENCHMARK TOKEN RING - CODE MESSAGGI VS TCP
==========================================

DESCRIZIONE:
-----------
Il sistema implementa due versioni di un token ring tra tre processi:
1. Versione con code di messaggi System V IPC
2. Versione con connessioni TCP

Entrambe le implementazioni garantiscono che ci sia sempre un solo token
circolante tra i tre processi e misurano la velocità di rotazione per
confrontare le performance.

COMPONENTI:
----------
- msg_process1.c   : Processo 1 per version code messaggi
- msg_process2.c   : Processo 2 per version code messaggi  
- msg_process3.c   : Processo 3 per version code messaggi
- tcp_process1.c   : Processo 1 per versione TCP
- tcp_process2.c   : Processo 2 per versione TCP
- tcp_process3.c   : Processo 3 per versione TCP
- token_ring.h     : Header comune con definizioni
- Makefile         : File di compilazione multipiattaforma

COMPILAZIONE:
------------
make all

Compila tutti i componenti per entrambe le versioni del benchmark.

ESECUZIONE VERSIONE CODE MESSAGGI:
----------------------------------
1. In tre terminali separati, eseguire:
   Terminal 1: ./msg_process1
   Terminal 2: ./msg_process2  
   Terminal 3: ./msg_process3

2. Il processo 1 inizia automaticamente il token ring
3. I risultati mostrano il numero di rotazioni completate al secondo

ESECUZIONE VERSIONE TCP:
------------------------
1. In tre terminali separati, eseguire:
   Terminal 1: ./tcp_process1
   Terminal 2: ./tcp_process2
   Terminal 3: ./tcp_process3

2. Attendere che tutti i processi si connettano
3. Il token ring inizia automaticamente
4. I risultati mostrano il numero di rotazioni completate al secondo

INTERPRETAZIONE RISULTATI:
-------------------------
- Ogni processo mostra il numero di token ricevuti/inviati
- La velocità è misurata in rotazioni complete per secondo
- Generalmente le code messaggi sono più veloci del TCP per IPC locale
- I risultati dipendono dal carico del sistema

FUNZIONAMENTO TOKEN RING:
------------------------
- Il token circola: Processo1 -> Processo2 -> Processo3 -> Processo1
- Ogni processo incrementa un contatore nel token
- La sincronizzazione garantisce un solo token circolante
- Il benchmark dura 10 secondi per ogni test

PULIZIA:
-------
make clean

Rimuove tutti i file binari generati.

COMPATIBILITÀ:
-------------
- Testato su Linux e macOS
- Richiede libreria APUE
- Standard C90 compliant