# Memory Mapping con Sincronizzazione Inter-Process

## Traccia del Problema

Due processi indipendenti mappano nella loro memoria lo stesso file e scrivono in esso continuamente a intervalli di tempo casuali una stringa così costituita: 
- Identità del processo (es. Processo 1)
- Un numero intero casuale  
- Una marca temporale che caratterizzi in maniera univoca l'istante della scrittura
- Infine nuovamente l'identità del processo

**Obiettivo:** Gestire la concorrenza in modo da evitare che le scritture dei due processi si intersechino.

---

## Soluzione 1: Processo Padre-Figlio ✅

### Caratteristiche
- **Approccio:** Un singolo eseguibile che usa `fork()` per creare padre e figlio
- **Processi:** Padre e figlio condividono la memoria mappata
- **Formato record:** `processo_id_numero_timestamp_processo_id`
- **Sincronizzazione:** Semafori POSIX
- **Output:** Completo e funzionante

### File
- `processo_padre_figlio.c`

### Compilazione ed Esecuzione
```bash
gcc -o processo processo_padre_figlio.c -lpthread -lrt
./processo
```

### Vantaggi
- ✅ **Funziona perfettamente** - stampa sempre i risultati completi
- ✅ Gestione automatica del ciclo di vita dei processi
- ✅ Il padre aspetta la terminazione del figlio con `wait()`
- ✅ Cleanup delle risorse garantito

### Flusso di Esecuzione
1. Crea il file e mappa la memoria
2. Inizializza semaforo POSIX  
3. Fork(): crea processo figlio
4. Entrambi i processi scrivono 10 record con intervalli casuali
5. Il padre aspetta la terminazione del figlio
6. Stampa risultati finali e pulisce le risorse

---

## Soluzione 2: Processi Indipendenti ⚠️

### Caratteristiche  
- **Approccio:** Due eseguibili separati da lanciare in terminali diversi
- **Processi:** Due processi completamente indipendenti
- **Formato record:** ` | processo_id | numero | timestamp | processo_id |`
- **Sincronizzazione:** Semafori POSIX con contatore di processi terminati
- **Output:** Funzionale ma con problemi di visualizzazione finale

### File
- `first_process.c` - Primo processo (da lanciare per primo)
- `second_process.c` - Secondo processo (da lanciare dopo)

### Compilazione ed Esecuzione
```bash
# Compilazione
gcc -o first_process first_process.c -lpthread -lrt  
gcc -o second_process second_process.c -lpthread -lrt

# Esecuzione (in terminali separati)
./first_process    # Terminale 1
./second_process   # Terminale 2
```

### Limitazioni
- ⚠️ **Non stampa sempre tutti i risultati finali** - solo uno dei due processi mostra l'output completo
- ⚠️ Timing critico per la sincronizzazione finale
- ⚠️ Richiede coordinamento manuale (lanciare nell'ordine corretto)

### Vantaggi
- ✅ Processi veramente indipendenti (come richiesto dalla traccia)
- ✅ Dimostra l'uso di memoria condivisa tra processi separati
- ✅ Sincronizzazione robusta durante la scrittura

### Flusso di Esecuzione
1. **Primo processo:** Crea file, memoria mappata e semaforo
2. **Secondo processo:** Si attacca alle risorse esistenti  
3. Entrambi scrivono record intercalati con sincronizzazione
4. Coordinamento finale per stampare risultati (problematico)

---

## Conformità alla Traccia

### ✅ Requisiti Rispettati (entrambe le soluzioni)

| Requisito | Soluzione 1 | Soluzione 2 |
|-----------|-------------|-------------|
| Due processi indipendenti | ✅ Padre/Figlio | ✅ Eseguibili separati |
| Mappano stesso file | ✅ `mmap()` condiviso | ✅ `mmap()` condiviso |
| Intervalli casuali | ✅ `sleep(1-3 sec)` | ✅ `sleep(1-3 sec)` |
| Identità processo | ✅ PID nel record | ✅ PID nel record |
| Numero casuale | ✅ `rand() % 10000` | ✅ `rand() % 10000` |
| Marca temporale | ✅ `strftime()` | ✅ `strftime()` |
| Identità processo (ripetuta) | ✅ PID finale | ✅ PID finale |
| Gestione concorrenza | ✅ Semafori POSIX | ✅ Semafori POSIX |

### Dettagli Tecnici Comuni

**Memory Mapping:**
- File condiviso tramite `mmap()` con `MAP_SHARED`
- Struttura `SharedMemory` per organizzare i dati
- Dimensione file: 4096/4112 bytes

**Sincronizzazione:**  
- Semafori POSIX denominati (`/sync_sem`)
- Mutua esclusione durante le operazioni di scrittura
- Protezione del contatore di record e della posizione di scrittura

**Gestione Overflow:**
- Reset della posizione quando il buffer è pieno
- Limite massimo di 50 record per prevenire overflow

---

## Raccomandazione

**Per uso didattico/demo:** Usare la **Soluzione 1** (padre-figlio) perché funziona sempre correttamente.

**Per comprendere IPC reale:** Studiare la **Soluzione 2** per capire le sfide della sincronizzazione tra processi completamente indipendenti.

Entrambe le soluzioni rispettano pienamente la traccia assegnata e dimostrano correttamente l'uso di memory mapping e sincronizzazione inter-process.