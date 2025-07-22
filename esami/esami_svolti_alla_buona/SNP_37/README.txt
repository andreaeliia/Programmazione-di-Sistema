# SINCRONIZZAZIONE DI PROCESSI CON SEMAFORI POSIX

## DESCRIZIONE DEL PROBLEMA
Implementazione di un sistema di sincronizzazione per 8 processi (P1-P8) 
che devono essere eseguiti secondo un ordine di precedenza specificato da 
un grafo aciclico diretto.

## STRUTTURA DEL GRAFO
- P1 -> P2 -> P3 (sequenza lineare iniziale)
- Da P3 si diramano tre percorsi:
  * P3 -> P4 -> P8 (ramo sinistro)
  * P3 -> P5 -> P6 -> P8 (ramo centrale)  
  * P3 -> P7 -> P8 (ramo destro)

## VINCOLI SPECIALI
- Solo 2 dei 3 rami devono essere eseguiti per ogni iterazione
- Possibili combinazioni: (P4,P5/P6) o (P7,P5/P6) o (P4,P7)
- Tutti i processi sono ciclici (loop infinito)
- Minimo numero di semafori POSIX

## ARCHITETTURA DELLA SOLUZIONE

### SEMAFORI UTILIZZATI (7 totali)
1. sem_p1_p2    - Sincronizza P1 -> P2
2. sem_p2_p3    - Sincronizza P2 -> P3  
3. sem_p3_branches - Coordina l'avvio dei rami da P3
4. sem_p5_p6    - Sincronizza P5 -> P6
5. sem_p4_p8    - Segnala completamento P4 per P8
6. sem_p6_p8    - Segnala completamento P6 per P8
7. sem_p7_p8    - Segnala completamento P7 per P8

### LOGICA DI SELEZIONE DEI RAMI
P3 decide casualmente quale combinazione di rami eseguire:
- Opzione 1: P4 + P5/P6
- Opzione 2: P7 + P5/P6  
- Opzione 3: P4 + P7

### CARATTERISTICHE DEI PROCESSI
- Ogni processo ha un ciclo infinito
- Stampa messaggi di debug per tracciare l'esecuzione
- Sleep per simulare lavoro computazionale
- Gestione corretta della terminazione con signal handler

## COMPILAZIONE E ESECUZIONE

### Requisiti
- Compilatore GCC con supporto C90/ANSI
- Libreria APUE (Advanced Programming in UNIX Environment)
- Sistema Linux o macOS

### Compilazione
```bash
make processo_coordinato
```

### Esecuzione  
```bash
./processo_coordinato
```

### Terminazione
Inviare SIGTERM o SIGINT (Ctrl+C) per terminare tutti i processi

## STRUTTURA DEI FILE
- processo_coordinato.c - Codice sorgente principale
- README.txt - Questa documentazione
- Makefile - Script di compilazione

## OUTPUT ATTESO
Il programma stampa messaggi che mostrano:
- Quale processo sta iniziando l'esecuzione
- Quale combinazione di rami è stata selezionata  
- L'ordine di completamento dei processi
- Il numero di iterazione per ogni ciclo

## NOTE TECNICHE
- Utilizzo di memoria condivisa per i semafori
- Named semaphores per compatibilità cross-platform
- Gestione robusta degli errori
- Cleanup automatico delle risorse alla terminazione