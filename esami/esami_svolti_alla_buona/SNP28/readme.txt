================================================================================
        SIMULAZIONE INCROCIO STRADALE A 4 VIE CON SEMAFORI POSIX
================================================================================

DESCRIZIONE GENERALE
====================

Questo progetto simula un incrocio stradale a 4 vie utilizzando un'architettura
multi-processo con sincronizzazione tramite semafori POSIX. Il sistema modella
realisticamente il comportamento di un incrocio urbano con:

- Generazione casuale di veicoli sulle 4 vie (Nord, Sud, Est, Ovest)
- Gestione centralizzata del semaforo con cicli verde/giallo/rosso
- Code di veicoli in attesa per ogni via
- Transito controllato secondo lo stato del semaforo
- Statistiche di traffico in tempo reale

ARCHITETTURA DEL SISTEMA
========================

Il sistema è composto da 5 processi concorrenti:

PROCESSO PRINCIPALE:
- Coordina l'intera simulazione
- Crea e gestisce gli altri processi
- Monitora lo stato dell'incrocio
- Stampa statistiche periodiche

4 PROCESSI VIA (Nord, Sud, Est, Ovest):
- Generano casualmente veicoli in arrivo
- Gestiscono code di veicoli in attesa
- Processano il transito quando hanno il verde
- Mantengono statistiche locali

PROCESSO CONTROLLORE SEMAFORO:
- Gestisce i cicli di semaforo centralizzato
- Coordina l'accesso esclusivo all'incrocio
- Implementa sequenza: Verde → Giallo → Rosso
- Garantisce sicurezza del traffico

MECCANISMI DI SINCRONIZZAZIONE
=============================

SEMAFORI POSIX NAMED:
- 4 semafori per le vie (/traffic_north, /traffic_south, /traffic_east, /traffic_west)
- 1 semaforo mutex per accesso esclusivo (/traffic_mutex)
- Gestione stato verde/rosso per ogni direzione

MEMORIA CONDIVISA:
- Struttura condivisa tra tutti i processi (/traffic_shm)
- Code circolari per veicoli di ogni via
- Stato corrente dei semafori (rosso/giallo/verde)
- Contatori statistiche globali

STRUTTURE DATI:
- Code FIFO per veicoli in attesa (20 veicoli max per via)
- Stati del semaforo: RED_LIGHT, YELLOW_LIGHT, GREEN_LIGHT
- Timestamp per calcolo tempi di attesa

ALGORITMO DI SIMULAZIONE
========================

GENERAZIONE VEICOLI:
1. Ogni processo via genera veicoli con probabilità del 33%
2. Intervallo di generazione: ogni 2 secondi
3. Veicoli con ID unico e timestamp di arrivo
4. Inserimento in coda se spazio disponibile

CONTROLLO SEMAFORO:
1. Ciclo sequenziale: Nord → Sud → Est → Ovest → Nord...
2. Fase verde: 5 secondi per via attiva
3. Fase giallo: 1 secondo di transizione
4. Fase rosso: immediato per tutte le altre vie

TRANSITO VEICOLI:
1. Solo veicoli con verde possono transitare
2. Un veicolo al secondo può attraversare l'incrocio
3. Rimozione dalla coda al completamento transito
4. Calcolo tempo di attesa totale

FILES DEL PROGETTO
==================

traffic_intersection.c  - Implementazione principale della simulazione
Makefile               - Script di compilazione multi-piattaforma
README.txt             - Questa documentazione

PARAMETRI DI CONFIGURAZIONE
===========================

Le seguenti costanti possono essere modificate nel codice per personalizzare
la simulazione:

#define NUM_LANES 4                    // Numero di vie (fisso a 4)
#define MAX_VEHICLES_PER_LANE 20       // Capacità massima coda per via
#define GREEN_LIGHT_DURATION 5         // Durata fase verde (secondi)
#define YELLOW_LIGHT_DURATION 1        // Durata fase giallo (secondi)
#define VEHICLE_GENERATION_INTERVAL 2  // Intervallo generazione veicoli
#define TRANSIT_TIME 1                 // Tempo transito per veicolo
#define SIM_DURATION 30                // Durata totale simulazione

REQUISITI DI SISTEMA
====================

- Compilatore: GCC con supporto ANSI C (C90)
- Sistema: Linux o Mac OS con supporto POSIX
- Librerie:
  * apue.h (Advanced Programming in UNIX Environment)
  * semaphore.h (Semafori POSIX)
  * sys/mman.h (Memoria condivisa)
- Permessi: Capacità di creare semafori e memoria condivisa
- RAM: Almeno 16 MB per strutture dati

COMPILAZIONE
============

Il progetto usa un Makefile compatibile con Linux e Mac OS.

Compilazione standard:
    make

Compilazione con informazioni dettagliate:
    make info

Test della simulazione:
    make test

Test breve (10 secondi):
    make test-short

Controllo risorse sistema:
    make check-resources

Pulizia risorse rimaste:
    make clean-resources

Debug avanzato:
    make debug

Controllo memoria:
    make memcheck

Stress test:
    make stress-test

Aiuto completo:
    make help

ESECUZIONE
==========

Esecuzione diretta:
    ./traffic_intersection

Il programma avvierà automaticamente:
1. Inizializzazione risorse condivise
2. Creazione dei 5 processi
3. Simulazione per 30 secondi (default)
4. Stampa statistiche ogni 5 secondi
5. Terminazione pulita con statistiche finali

Per interrompere premere Ctrl+C (terminazione sicura).

OUTPUT DELLA SIMULAZIONE
========================

ESEMPIO DI OUTPUT:

=== SIMULAZIONE INCROCIO STRADALE A 4 VIE ===
Durata simulazione: 30 secondi
Tempo verde: 5 sec, Tempo giallo: 1 sec
Risorse inizializzate correttamente
Processo Via NORD (PID: 12345) avviato
Processo Via SUD (PID: 12346) avviato
Processo Via EST (PID: 12347) avviato
Processo Via OVEST (PID: 12348) avviato
Controllore Semaforo (PID: 12349) avviato

=== SEMAFORO: Via NORD VERDE ===
Via NORD: Veicolo #1 generato (Coda: 1 veicoli)
Via EST: Veicolo #1 generato (Coda: 1 veicoli)
Via NORD: Veicolo #1 transitato (Tempo attesa: 2 sec, Coda: 0)

=== STATO INCROCIO ===
Via NORD : VERDE (0 veicoli in coda)
Via SUD  : ROSSO (2 veicoli in coda)
Via EST  : ROSSO (1 veicoli in coda)
Via OVEST: ROSSO (0 veicoli in coda)
Totale generati: 4, Totale transitati: 1
=====================

=== STATISTICHE FINALI ===
Veicoli generati totali: 45
Veicoli transitati totali: 38
Efficienza: 84.4%

INTERPRETAZIONE RISULTATI
=========================

METRICHE PRINCIPALI:

1. VEICOLI GENERATI: Numero totale di veicoli arrivati all'incrocio
2. VEICOLI TRANSITATI: Numero di veicoli che hanno attraversato l'incrocio
3. EFFICIENZA: Percentuale di veicoli transitati rispetto ai generati
4. TEMPO ATTESA: Tempo medio che i veicoli aspettano in coda

INDICATORI DI PERFORMANCE:

- Efficienza > 80%: Buone prestazioni dell'incrocio
- Efficienza 60-80%: Prestazioni accettabili
- Efficienza < 60%: Congestione significativa

- Tempo attesa < 10 sec: Flusso scorrevole
- Tempo attesa 10-30 sec: Flusso normale  
- Tempo attesa > 30 sec: Possibile congestione

GESTIONE ERRORI E SICUREZZA
===========================

CONTROLLI IMPLEMENTATI:
- Verifica successo creazione semafori e memoria condivisa
- Controllo overflow delle code (max 20 veicoli per via)
- Gestione segnali per terminazione pulita (SIGINT, SIGTERM)
- Sincronizzazione sicura con mutex per accesso esclusivo
- Pulizia automatica di tutte le risorse alla terminazione

GESTIONE FAULT:
- Terminazione anomala: Pulizia automatica via signal handler
- Deadlock prevention: Timeout impliciti nei semafori
- Memory leaks: Unmapping esplicito della memoria condivisa
- Risorse orfane: Cleanup con make clean-resources

SINCRONIZZAZIONE DETTAGLIATA
============================

PATTERN DI ACCESSO:
1. Acquisizione mutex prima di ogni operazione su strutture condivise
2. Operazione atomica su code o contatori
3. Rilascio immediato del mutex
4. Semafori via per segnalazione stato verde/rosso

PREVENZIONE RACE CONDITIONS:
- Accesso mutuamente esclusivo a memoria condivisa
- Operazioni atomiche su code circolari
- Sequenziamento deterministico del controllore semaforo

EVITARE DEADLOCK:
- Ordine fisso di acquisizione risorse
- Timeout impliciti nelle system call
- Terminazione coordinata di tutti i processi

PERSONALIZZAZIONE E ESTENSIONI
==============================

MODIFICHE SEMPLICI:
- Cambiare durata fasi semaforo (GREEN_LIGHT_DURATION)
- Modificare frequenza generazione veicoli (VEHICLE_GENERATION_INTERVAL)
- Aumentare capacità code (MAX_VEHICLES_PER_LANE)
- Estendere durata simulazione (SIM_DURATION)

ESTENSIONI AVANZATE:
- Aggiunta corsie di svolta (destra/sinistra)
- Semafori intelligenti basati su densità traffico
- Veicoli con priorità (ambulanze, autobus)
- Analisi statistica avanzata con log su file
- Interfaccia grafica per visualizzazione real-time
- Integrazione con sensori di traffico simulati

TROUBLESHOOTING
===============

PROBLEMA: "sem_open failed"
CAUSA: Permessi insufficienti o limite sistema
SOLUZIONE: 
- Verificare permessi: chmod 666 /dev/shm/*
- Aumentare limite: echo 128 > /proc/sys/kernel/sem

PROBLEMA: "shm_open failed" 
CAUSA: Spazio insufficiente in /dev/shm
SOLUZIONE:
- Verificare spazio: df -h /dev/shm
- Pulire file vecchi: make clean-resources

PROBLEMA: Processi zombie
CAUSA: Mancata attesa terminazione processi figli
SOLUZIONE:
- Verificare waitpid() nel codice
- Killare processi manualmente: kill -9 <PID>

PROBLEMA: "Address already in use"
CAUSA: Risorse non pulite da esecuzione precedente
SOLUZIONE: 
- Pulizia manuale: make clean-resources
- Reboot sistema in casi estremi

PROBLEMA: Prestazioni scarse
CAUSA: Sistema sovraccarico o configurazione subottimale
SOLUZIONE:
- Ridurre VEHICLE_GENERATION_INTERVAL
- Aumentare GREEN_LIGHT_DURATION
- Verificare carico sistema con top/htop

ANALISI PRESTAZIONI
===================

FATTORI CHE INFLUENZANO L'EFFICIENZA:

1. TIMING SEMAFORO:
   - Verde troppo breve: veicoli non riescono a transitare
   - Verde troppo lungo: altre vie accumulate eccessivamente
   - Giallo adeguato: necessario per sicurezza

2. FREQUENZA GENERAZIONE:
   - Alta frequenza: saturazione code, bassa efficienza
   - Bassa frequenza: sottoutilizzo incrocio
   - Bilanciamento: 33% probabilità ogni 2 secondi

3. CAPACITÀ CODE:
   - Code piccole: perdita veicoli per overflow
   - Code grandi: maggiore memoria, tempi attesa alti
   - Ottimale: 20 veicoli per via per simulazioni brevi

OTTIMIZZAZIONE SUGGERTA:
- Monitorare rapporto generazione/transito in tempo reale
- Adattare dinamicamente durata fasi semaforo
- Implementare algoritmi di semaforo intelligente

CONFORMITA' STANDARD
=====================

Il codice è conforme a:
- Standard ANSI C (C90/C89)
- POSIX.1-2001 per semafori named
- POSIX.1-2001 per memoria condivisa (shm_open)
- Single UNIX Specification v3
- Thread-safe e signal-safe operations

SICUREZZA E ROBUSTEZZA:
- No buffer overflow (dimensioni fisse controllate)
- No memory leaks (pulizia esplicita)
- No race conditions (sincronizzazione corretta)
- Graceful degradation in caso di errori

BIBLIOGRAFIA
============

- "Advanced Programming in the UNIX Environment" - W. Richard Stevens
- "POSIX Programmer's Guide" - Donald Lewine
- "The Design and Implementation of the 4.3BSD UNIX Operating System"
- POSIX.1-2008 Standard Documentation
- "Operating System Concepts" - Silberschatz, Galvin, Gagne

================================================================================
                              FINE DOCUMENTAZIONE
================================================================================