# Sistema Stream Client-Server

## Descrizione
Questo progetto implementa un sistema client-server per la trasmissione di uno stream continuo di caratteri alfanumerici. Il client può regolare la velocità di trasmissione in tempo reale tramite comandi da tastiera.

## Architettura del Sistema
Il sistema è composto da due programmi:
- **server.c**: Server TCP che invia caratteri alfanumerici a velocità variabile
- **client.c**: Client TCP che riceve i caratteri e permette il controllo della velocità

## Funzionalità

### Server
- Accetta connessioni TCP su localhost porta 8080
- Invia stream continuo di caratteri alfanumerici casuali
- Gestisce richieste di cambio velocità dal client
- Mostra la velocità di trasmissione corrente

### Client
- Si connette al server su localhost:8080
- Controlla velocità tramite tastiera:
  - 'u': aumenta velocità
  - 'd': diminuisce velocità
  - 'q': termina il programma
- Misura e mostra la velocità di ricezione
- Salva tutti i dati ricevuti nel file "received_data.txt"

## Compilazione

Assicurarsi di avere la libreria APUE installata nel sistema.

```bash
make server
make client
```

O per compilare entrambi:
```bash
make all
```

## Esecuzione

1. Avviare il server in un terminale:
```bash
./server
```

2. Avviare il client in un altro terminale:
```bash
./client
```

3. Nel terminale del client:
   - Premere 'u' per aumentare la velocità
   - Premere 'd' per diminuire la velocità  
   - Premere 'q' per uscire

## File di Output
I dati ricevuti vengono salvati automaticamente in "received_data.txt"

## Note Tecniche
- Utilizza socket TCP per la comunicazione
- Input non bloccante per il controllo della velocità
- Gestione dei segnali per terminazione pulita
- Compatibile con Linux e macOS

## Velocità
- Velocità iniziale: 10 caratteri/secondo
- Minima: 1 carattere/secondo
- Massima: 100 caratteri/secondo
- Incremento/decremento: ±5 caratteri/secondo