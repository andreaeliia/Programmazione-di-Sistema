# ESERCIZI DI PROGRAMMAZIONE DI SISTEMA
## Memory Mapping e Analisi ASLR

### DESCRIZIONE DEGLI ESERCIZI

**Esercizio 1: Comunicazione tramite Memory Mapping**
- Verifica sperimentale della persistenza della comunicazione tra processi 
  tramite memory mapping dopo la chiusura dei file descriptor
- Implementazione di comunicazione padre-figlio usando mmap()
- Test della condivisione di memoria dopo close()

**Esercizio 2: Analisi ASLR delle Librerie Dinamiche**  
- Determinazione sperimentale della variabilità degli indirizzi di funzioni
  di libreria dinamica tra diversi lanci di un programma
- Calcolo statistico di media e deviazione standard delle posizioni in memoria
- Analisi dell'Address Space Layout Randomization (ASLR)

---

### STRUTTURA DEI FILE

```
esercizi/
├── esercizio1.c          # Comunicazione tramite memory mapping
├── esercizio2.c          # Analisi ASLR - programma principale
├── target_program.c      # Programma target per esercizio 2
├── Makefile             # File di compilazione multipiattaforma
└── README.txt           # Questo file
```

---

### REQUISITI DI SISTEMA

- **Compilatore**: GCC con supporto ANSI C (C90)
- **Piattaforme supportate**: Linux e macOS
- **Librerie richieste**: 
  - apue.h (Advanced Programming in UNIX Environment)
  - libapueLinux.a (su Linux) o libapueMacOS.a (su macOS)
  - libm (per funzioni matematiche in esercizio2)

**Struttura directory richiesta:**
```
./include/apue.h         # Header APUE
./lib/libapueLinux.a     # Libreria APUE per Linux
./lib/libapueMacOS.a     # Libreria APUE per macOS
```

---

### COMPILAZIONE

**Compilazione di tutti i programmi:**
```bash
make all
```

**Compilazione singola:**
```bash
make esercizio1          # Solo esercizio 1
make esercizio2          # Solo esercizio 2  
make target_program      # Solo programma target
```

**Pulizia:**
```bash
make clean              # Rimuove binari e file temporanei
```

---

### ESECUZIONE

**Esercizio 1:**
```bash
make run1
# oppure
./esercizio1
```

**Esercizio 2:**
```bash
make run2
# oppure  
./esercizio2
```

**Test completo:**
```bash
make test               # Esegue entrambi gli esercizi
```

---

### DETTAGLI TECNICI

**Esercizio 1 - Memory Mapping:**
- Crea un file temporaneo "shared_file.tmp" 
- Utilizza mmap() per condividere memoria tra padre e figlio
- Testa comunicazione prima e dopo close() del file descriptor
- Dimostra che il mapping persiste anche dopo chiusura del fd

**Esercizio 2 - Analisi ASLR:**
- Lancia 50 istanze del target_program
- Raccoglie gli indirizzi della funzione printf()
- Calcola statistiche: media, deviazione standard, range
- Determina se ASLR è attivo confrontando gli indirizzi

---

### RISULTATI ATTESI

**Esercizio 1:**
- I processi continuano a comunicare anche dopo close()
- Conferma che mmap() mantiene la condivisione in memoria
- Output dettagliato delle fasi di comunicazione

**Esercizio 2:**
- Su sistemi con ASLR attivo: indirizzi variabili
- Su sistemi con ASLR disattivo: indirizzi costanti
- Statistiche complete sulla distribuzione degli indirizzi
- Analisi del range di variazione

---

### NOTE TECNICHE

- **Compatibilità**: Il codice è conforme allo standard ANSI C (C90)
- **Portabilità**: Supporta sia Linux che macOS tramite flag condizionali
- **Memory Safety**: Gestione corretta di mmap/munmap e apertura/chiusura file
- **Error Handling**: Utilizzo delle funzioni err_sys() della libreria APUE
- **Sincronizzazione**: Gestione corretta dei processi padre-figlio

---

### POSSIBILI PROBLEMI E SOLUZIONI

**Errore "target_program non esiste":**
- Compilare prima: `make target_program`

**Errore di linking:**
- Verificare presenza di apue.h in ./include/
- Verificare presenza della libreria in ./lib/

**Permission denied:**
- Rendere eseguibili i programmi: `chmod +x esercizio1 esercizio2 target_program`

**Risultati inconsistenti esercizio 2:**
- Su alcune distribuzioni ASLR può essere disabilitato
- Verificare con: `cat /proc/sys/kernel/randomize_va_space` (Linux)

---

### AUTORE

Codice generato automaticamente per esercizi di programmazione di sistema.
Conforme agli standard APUE (Advanced Programming in UNIX Environment).