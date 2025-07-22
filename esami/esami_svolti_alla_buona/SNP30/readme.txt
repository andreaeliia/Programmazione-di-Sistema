================================================================================
        SCANNER INODE RICORSIVO PER DIRECTORY E SOTTOALBERI
================================================================================

DESCRIZIONE GENERALE
====================

Questo programma scansiona ricorsivamente una directory specificata e stampa
in ordine numerico crescente tutti i numeri di inode dei file regolari
presenti nel sottoalbero che ha quella directory come radice.

L'inode (index node) è una struttura dati del filesystem che contiene 
metadati sui file, incluso un numero identificativo unico. Questo strumento
è utile per:

- Analisi del filesystem e debug
- Identificazione di file duplicati (stesso inode = hard link)
- Audit di sicurezza e forensics
- Monitoraggio dell'utilizzo degli inode
- Statistiche sui file presenti in directory complesse

FUNZIONALITA' PRINCIPALI
========================

✓ Attraversamento ricorsivo completo di directory e sottodirectory
✓ Identificazione automatica di file regolari (esclude directory, link, device)
✓ Raccolta efficiente di tutti i numeri di inode
✓ Ordinamento numerico crescente dei risultati
✓ Gestione robusta degli errori e dei permessi
✓ Supporto per alberi di directory di grandi dimensioni
✓ Output pulito e formattato per elaborazione automatica

ALGORITMO DI FUNZIONAMENTO
==========================

1. VALIDAZIONE INPUT:
   - Verifica che l'argomento sia una directory esistente
   - Controlla i permessi di lettura

2. INIZIALIZZAZIONE:
   - Alloca array dinamico per memorizzare gli inode
   - Imposta capacità iniziale di 1000 elementi

3. SCANSIONE RICORSIVA:
   - Apre la directory con opendir()
   - Legge ogni entry con readdir()
   - Per ogni entry (eccetto "." e ".."):
     * Se è un file regolare → raccoglie l'inode con lstat()
     * Se è una directory → scansione ricorsiva
     * Ignora link simbolici, device files, named pipe

4. GESTIONE MEMORIA DINAMICA:
   - Espande automaticamente l'array quando necessario
   - Incrementi di 500 elementi per ottimizzare performance

5. ORDINAMENTO E OUTPUT:
   - Ordina gli inode con qsort() (algoritmo efficiente O(n log n))
   - Stampa risultati in formato standard su stdout

FILES DEL PROGETTO
==================

inode_scanner.c  - Implementazione principale del programma
Makefile        - Script di compilazione multi-piattaforma
README.txt      - Questa documentazione

REQUISITI DI SISTEMA
====================

- Compilatore: GCC con supporto ANSI C (C90)
- Sistema: Linux, Mac OS, o qualsiasi sistema UNIX-like
- Librerie: apue.h (Advanced Programming in UNIX Environment)
- RAM: Minimo 64 MB per gestione array dinamici
- Spazio disco: Trascurabile (~50 KB per eseguibile)
- Permessi: Lettura sulle directory da scansionare

FUNZIONI UNIX UTILIZZATE:
- opendir(), readdir(), closedir() per navigazione directory
- lstat() per ottenere informazioni sui file senza seguire link simbolici
- stat() per validazione directory
- malloc(), realloc(), free() per gestione memoria dinamica
- qsort() per ordinamento efficiente

COMPILAZIONE
============

Il progetto include un Makefile compatibile con Linux e Mac OS.

Compilazione standard:
    make

Compilazione con informazioni dettagliate:
    make info

Verifica sintassi senza compilazione:
    make check

Test automatici:
    make test

Test con directory di esempio:
    make test-examples

Test con directory di sistema:
    make test-system

Benchmark prestazioni:
    make benchmark

Controllo memoria (se valgrind disponibile):
    make memcheck

Pulizia file compilati:
    make clean

Aiuto completo:
    make help

UTILIZZO
========

SINTASSI:
    ./inode_scanner <directory_path>

ARGOMENTI:
    directory_path    Path assoluto o relativo della directory da scansionare

ESEMPI DI USO:

1. Scansione directory corrente:
    ./inode_scanner .

2. Scansione directory specifica:
    ./inode_scanner /home/user/documents

3. Scansione directory di sistema:
    ./inode_scanner /tmp

4. Scansione con path assoluto:
    ./inode_scanner /usr/local/bin

5. Scansione directory home:
    ./inode_scanner ~

6. Redirezione output su file:
    ./inode_scanner /var/log > inodes.txt

7. Conteggio totale file:
    ./inode_scanner /etc | wc -l

8. Pipeline con altri comandi:
    ./inode_scanner . | sort -n | head -10

OUTPUT DEL PROGRAMMA
===================

ESEMPIO DI OUTPUT:

$ ./inode_scanner /tmp/test
Scansione directory: /tmp/test
Trovati 4 file regolari
Inode in ordine numerico crescente:
=====================================
1234567
1234890
1235001
1235678
=====================================
Scansione completata con successo

INTERPRETAZIONE OUTPUT:

- HEADER: Mostra la directory scansionata
- CONTEGGIO: Numero totale di file regolari trovati
- LISTA INODE: Un numero di inode per riga, in ordine crescente
- FOOTER: Conferma completamento operazione

FORMATO NUMERI INODE:
- Numeri interi senza formato speciale
- Un inode per riga per facilità di elaborazione
- Ordinamento numerico (non lessicografico)

GESTIONE ERRORI
===============

Il programma gestisce robustamente diversi tipi di errore:

ERRORI DI INPUT:
- Numero sbagliato di argomenti → Stampa usage e termina
- Directory inesistente → Messaggio di errore dettagliato
- Path non è una directory → Messaggio esplicativo

ERRORI DI PERMESSI:
- Directory non leggibile → Salta e continua
- File non accessibile → Segnala errore ma prosegue
- Link simbolici rotti → Ignora silenziosamente

ERRORI DI SISTEMA:
- Memoria insufficiente → Termina con messaggio di errore
- Path troppo lungo → Segnala e continua con altri file
- Filesystem corruption → Gestione graceful degli errori I/O

ERRORI DI NAVIGAZIONE:
- Cicli infiniti (link simbolici) → Prevenuti usando lstat()
- Directory eliminate durante scansione → Gestito correttamente
- Permessi che cambiano durante esecuzione → Adattamento dinamico

MESSAGGI DI ERRORE:
Tutti i messaggi utilizzano le funzioni err_ret() e err_sys() della libreria
apue.h per fornire informazioni dettagliate incluso errno.

LIMITAZIONI E CONSIDERAZIONI
============================

LIMITAZIONI TECNICHE:
1. Solo file regolari sono considerati (no directory, device, pipe, socket)
2. Link simbolici non vengono seguiti (usa lstat() invece di stat())
3. Array dinamico limitato dalla memoria disponibile
4. Path limitati a 4096 caratteri (MAX_PATH_LENGTH)

CONSIDERAZIONI PRESTAZIONI:
- Directory con milioni di file richiedono memoria significativa
- Ordinamento O(n log n) può essere lento per dataset enormi
- Scansione I/O intensiva per filesystem lenti o di rete
- Crescita lineare della memoria con il numero di file

CONSIDERAZIONI SICUREZZA:
- Non segue link simbolici per evitare directory traversal
- Gestisce permessi limitati senza privilegio escalation
- Non modifica il filesystem (solo lettura)
- Buffer overflow prevenuti con controlli dimensioni

CASI D'USO TIPICI:
- Audit filesystem per amministratori di sistema
- Debug problemi di hard link e duplicati
- Analisi forense di directory sospette
- Monitoring utilizzo inode per prevenire filesystem full
- Ricerca efficiente di file specifici per inode number

DETTAGLI TECNICI AVANZATI
=========================

STRUTTURE DATI:
```
struct inode_list {
    ino_t *inodes;     // Array dinamico di inode numbers
    size_t count;      // Numero corrente di elementi
    size_t capacity;   // Capacità massima corrente
};
```

GESTIONE MEMORIA:
- Allocazione iniziale: 1000 inode (~ 8 KB su sistemi 64-bit)
- Incremento automatico: +500 inode quando array pieno
- Deallocazione: Completa alla terminazione programma
- Strategia: Crescita lineare per bilanciare memoria/performance

ALGORITMO ORDINAMENTO:
- Funzione: qsort() dalla libreria standard C
- Complessità: O(n log n) nel caso medio
- Comparazione: Numerica diretta tra ino_t values
- Stabilità: Non necessaria per numeri unici

ATTRAVERSAMENTO DIRECTORY:
- Algoritmo: Depth-First Search ricorsivo
- Stack: Utilizza stack chiamate sistema (non esplicito)
- Cicli: Prevenuti non seguendo link simbolici
- Ordine: Determinato dall'ordine delle entry nel filesystem

COMPATIBILITA' FILESYSTEM:
- ext2/ext3/ext4 (Linux): Supporto completo
- XFS (Linux): Supporto completo  
- HFS+/APFS (Mac): Supporto completo
- NTFS: Limitato (se montato su sistemi Unix)
- NFS: Supportato ma possibili performance issues

TROUBLESHOOTING
===============

PROBLEMA: "Permission denied"
CAUSA: Mancano permessi di lettura su directory
SOLUZIONE: 
- Eseguire con sudo se necessario
- Verificare permessi con ls -la
- Scegliere directory accessibile

PROBLEMA: "No such file or directory"  
CAUSA: Path specificato non esiste
SOLUZIONE:
- Verificare correttezza del path
- Usare path assoluto se necessario
- Controllare che la directory esista

PROBLEMA: "Not a directory"
CAUSA: Path specificato è un file, non una directory
SOLUZIONE:
- Verificare che l'argomento sia una directory
- Usare dirname per ottenere directory contenente

PROBLEMA: "Memory allocation failed"
CAUSA: Memoria insufficiente per array di inode
SOLUZIONE:
- Chiudere altre applicazioni
- Aumentare memoria virtuale/swap
- Scansionare directory più piccole

PROBLEMA: Output vuoto
CAUSA: Directory non contiene file regolari
SOLUZIONE:
- Verificare contenuto con ls -la
- Controllare che ci siano effettivamente file (non solo directory)
- Controllare permessi sui file contenuti

PROBLEMA: Prestazioni lente
CAUSA: Directory molto grande o filesystem lento
SOLUZIONE:
- Usare directory più piccole per test
- Evitare filesystem di rete se possibile
- Monitorare uso CPU/memoria con top

PROBLEMA: "Argument list too long" 
CAUSA: Path troppo lungo per il sistema
SOLUZIONE:
- Accorciare il path usando cd
- Utilizzare path relativi
- Verificare limite PATH_MAX del sistema

ESEMPI AVANZATI
===============

ANALISI STATISTICHE:
# Conta totale file in una directory
./inode_scanner /home/user | wc -l

# Trova il primo e ultimo inode numericamente
./inode_scanner /tmp | head -1  # Primo
./inode_scanner /tmp | tail -1  # Ultimo

# Genera report con timestamp
echo "Scansione $(date):" > report.txt
./inode_scanner /var/log >> report.txt

RICERCA AVANZATA:
# Trova file con inode specifico
find /home -inum 1234567 -type f

# Confronta inode tra directory diverse  
./inode_scanner /dir1 > dir1_inodes.txt
./inode_scanner /dir2 > dir2_inodes.txt
comm -12 dir1_inodes.txt dir2_inodes.txt  # Inode comuni

AUTOMATIZZAZIONE:
# Script per monitoraggio periodico
#!/bin/bash
LOGDIR="/var/log/inode_monitoring"
mkdir -p $LOGDIR
DATE=$(date +%Y%m%d_%H%M%S)
./inode_scanner /important/data > $LOGDIR/scan_$DATE.txt

INTEGRAZIONE SISTEMA:
# Aggiungi a crontab per monitoraggio quotidiano
0 2 * * * /path/to/inode_scanner /data > /var/log/daily_inode_scan.log

CONFORMITA' STANDARD
=====================

Il codice è conforme a:
- Standard ANSI C (C90/C89)
- POSIX.1-2001 per system call
- Single UNIX Specification v3
- BSD e GNU/Linux compatibility

PORTABILITA':
- Testato su Linux (Ubuntu, CentOS, Debian)
- Testato su Mac OS X/macOS
- Compatibile con FreeBSD, OpenBSD
- Funziona su Solaris con modifiche minime

SICUREZZA:
- No buffer overflow (controlli dimensioni rigidi)
- No format string vulnerabilities  
- No race conditions (solo operazioni read-only)
- Defensive programming practices

PERFORMANCE:
- Complessità temporale: O(n + n log n) dove n = numero file
- Complessità spaziale: O(n) per storage inode
- Ottimizzato per cache locality durante ordinamento
- Minimal system call overhead

SUPPORT E CONTATTI
==================

Questo strumento è basato sui principi e esempi del libro:
"Advanced Programming in the UNIX Environment" di W. Richard Stevens

Per problemi relativi alla libreria apue.h consultare:
- Documentazione ufficiale del libro
- Repository GitHub del codice APUE
- Man pages delle system call utilizzate

ESTENSIONI POSSIBILI:
- Aggiunta opzioni command line (verbose, formato output)
- Supporto per filtri tipo file (solo immagini, documenti, etc)
- Output in formati strutturati (JSON, XML, CSV)
- Parallelizzazione per directory molto grandi
- Caching per evitare rescansioni duplicate
- Integrazione con database per tracking storico

================================================================================
                              FINE DOCUMENTAZIONE
================================================================================