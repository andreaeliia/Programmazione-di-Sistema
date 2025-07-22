SISTEMA TCP CON MEMORIA CONDIVISA
================================

DESCRIZIONE:
-----------
Il sistema implementa due server TCP indipendenti che operano sulla stessa 
macchina. Entrambi i server ricevono stringhe di testo dai client (terminate 
da newline) e le memorizzano in un'area di memoria condivisa, garantendo che 
le stringhe non si sovrappongano tramite meccanismi di sincronizzazione.

COMPONENTI:
----------
- server1.c     : Primo server TCP (porta 8001)
- server2.c     : Secondo server TCP (porta 8002)  
- client.c      : Client di test per connettersi ai server
- shared_mem.h  : Header con definizioni per memoria condivisa
- Makefile      : File di compilazione multipiattaforma

COMPILAZIONE:
------------
make all

Questo comando compila tutti i componenti utilizzando le flag appropriate
per il sistema operativo (Linux o macOS).

ESECUZIONE:
----------
1. Avviare il primo server:
   ./server1

2. Avviare il secondo server (in un altro terminale):
   ./server2

3. Connettersi con un client (in un altro terminale):
   ./client <porta>
   
   dove <porta> può essere 8001 o 8002

FUNZIONAMENTO:
-------------
- I server ascoltano rispettivamente sulle porte 8001 e 8002
- Ogni server accetta connessioni multiple tramite fork()
- Le stringhe ricevute vengono scritte in memoria condivisa
- Un semaforo garantisce l'accesso esclusivo alla memoria condivisa
- Il sistema visualizza tutte le stringhe ricevute da entrambi i server

REQUISITI SISTEMA:
-----------------
- Compilatore GCC con supporto C90
- Libreria APUE (Advanced Programming in the UNIX Environment)
- Sistema operativo Linux o macOS

PULIZIA:
-------
make clean

Rimuove tutti i file binari generati.