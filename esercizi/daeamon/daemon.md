# DAEMON - Guida Completa per Esame

## GUIDA VELOCE
- Fare il codice
- Per fare diventare un programma un daemon bisogna:
    1.Aggiungere le funzioni became_daemon(), signal_handler() e daemon_log()
    2.Cambiare ogni printf in daemon_log()
    3.cleanUp del daemon


## LIBRERIE NECESSARIE

```c
#include <stdio.h>      // fprintf, fopen, fclose, perror
#include <stdlib.h>     // exit, EXIT_SUCCESS, EXIT_FAILURE
#include <unistd.h>     // fork, setsid, chdir, close, getpid, sleep
#include <sys/types.h>  // pid_t, definizioni tipi
#include <sys/stat.h>   // umask (opzionale)
#include <fcntl.h>      // open, O_RDONLY, O_WRONLY
#include <signal.h>     // signal, kill, SIGTERM (per gestione segnali)
#include <errno.h>      // errno, strerror (per debug errori)
#include <string.h>     // strerror
#include <time.h>       // time, ctime (per timestamp nei log)
#include <stdarg.h>  // Per va_list, va_start, va_end
```

## COS'È UN DAEMON

### Definizione
Un **daemon** è un processo che gira in background nel sistema operativo, senza interfaccia utente diretta. Esempi nel sistema:
- `sshd` - server SSH
- `httpd` - server web Apache  
- `mysqld` - database MySQL
- `systemd` - gestore servizi Linux

### Caratteristiche Fondamentali
1. **Background execution**: Non è associato a un terminale
2. **Session independence**: Continua a girare anche se l'utente fa logout
3. **Auto-start**: Tipicamente avviato all'avvio del sistema
4. **Log-based communication**: Comunica tramite file di log, non stdout
5. **Signal handling**: Risponde a segnali del sistema operativo

## PROCESSO DI DAEMONIZZAZIONE

### Schema Completo
```
PROCESSO NORMALE
├─ fork() ────────────┐
│                     │
PROCESSO PADRE        PROCESSO FIGLIO
├─ exit(0)           ├─ setsid()
└─ [TERMINA]         ├─ chdir("/")
                     ├─ close(0,1,2)
                     ├─ open("/dev/null") x3
                     └─ [DIVENTA DAEMON]
```

### Spiegazione Dettagliata di Ogni Passo

#### PASSO 1: fork()
```c
pid_t pid = fork();
if (pid > 0) {
    // PROCESSO PADRE
    printf("Daemon creato con PID: %d\n", pid);
    exit(0);  // PADRE TERMINA
}
if (pid < 0) {
    perror("fork fallita");
    exit(1);
}
// PROCESSO FIGLIO continua...
```

**PERCHÉ SERVE:**
- Il figlio diventa "orfano" (senza padre)
- Il padre termina, liberando il terminale
- Il figlio viene "adottato" dal processo init (PID 1)

**VARIABILI:**
- `pid_t pid`: Tipo per Process ID, definito in `<sys/types.h>`
- Valore ritorno fork():
  - `> 0`: PID del figlio (sei nel padre)
  - `= 0`: Sei nel figlio
  - `< 0`: Errore

#### PASSO 2: setsid()
```c
if (setsid() < 0) {
    perror("setsid fallita");
    exit(1);
}
```

**PERCHÉ SERVE:**
- Crea una **nuova sessione**
- Il processo diventa **session leader**
- Si **stacca dal terminale di controllo**
- Non può più ricevere segnali da tastiera (Ctrl+C, Ctrl+Z)

**COSA SUCCEDE INTERNAMENTE:**
- Il processo ottiene un nuovo Session ID (SID)
- Diventa leader di un nuovo process group
- Perde il terminale di controllo

#### PASSO 3: chdir()
```c
if (chdir("/") < 0) {
    perror("chdir fallita");
    exit(1);
}
```

**PERCHÉ SERVE:**
- Evita di "bloccare" directory che potrebbero essere smontate
- Directory root (/) è sempre disponibile
- Prevenzione errori filesystem

#### PASSO 4: Chiusura File Descriptor Standard
```c
close(STDIN_FILENO);   // 0 - input da tastiera
close(STDOUT_FILENO);  // 1 - output su terminale  
close(STDERR_FILENO);  // 2 - errori su terminale
```

**PERCHÉ SERVE:**
- Il daemon non deve leggere da tastiera
- Non deve scrivere su terminale (che non esiste più)
- Libera risorse del sistema

**COSTANTI:**
```c
STDIN_FILENO  = 0  // File descriptor input standard
STDOUT_FILENO = 1  // File descriptor output standard  
STDERR_FILENO = 2  // File descriptor errori standard
```

#### PASSO 5: Reindirizzamento su /dev/null
```c
open("/dev/null", O_RDONLY);  // Nuovo stdin (fd 0)
open("/dev/null", O_WRONLY);  // Nuovo stdout (fd 1)
open("/dev/null", O_WRONLY);  // Nuovo stderr (fd 2)
```

**PERCHÉ SERVE:**
- Se il daemon prova a leggere/scrivere sui FD standard per errore
- `/dev/null` "assorbe" tutto senza generare errori
- Comportamento predicibile

## FUNZIONI ESSENZIALI

### fork()
```c
#include <unistd.h>
pid_t fork(void);
```
**RITORNA:**
- Nel padre: PID del figlio (> 0)
- Nel figlio: 0
- Errore: -1 (setta errno)

**ERRORI COMUNI:**
```c
// SBAGLIATO - non controlli il valore di ritorno
fork();

// GIUSTO - controlli sempre
pid_t pid = fork();
if (pid < 0) {
    perror("fork");
    exit(1);
}
```

### setsid()
```c
#include <unistd.h>
pid_t setsid(void);
```
**RITORNA:**
- Successo: nuovo Session ID
- Errore: -1 (setta errno)

**QUANDO FALLISCE:**
- Se il processo è già session leader
- Errori di sistema

### chdir()
```c
#include <unistd.h>
int chdir(const char *path);
```
**PARAMETRI:**
- `path`: Directory di destinazione

**RITORNA:**
- Successo: 0
- Errore: -1 (setta errno)

### getpid()
```c
#include <unistd.h>
pid_t getpid(void);
```
**RITORNA:** Process ID del processo corrente (mai fallisce)

## GESTIONE DEI LOG

### Perché i Daemon Usano File di Log
- Non hanno accesso al terminale
- Debugging e monitoring
- Audit trail per sicurezza
- Analisi post-mortem

### Pattern di Logging Thread-Safe
```c
#include <pthread.h>
#include <time.h>
#include <stdarg.h>  // Per va_list, va_start, va_end

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void daemon_log(const char *format, ...) {  // Aggiungi ...
    pthread_mutex_lock(&log_mutex);
    
    FILE *log = fopen("/tmp/daemon_server.log", "a");  // Cambia path
    if (log) {
        time_t now = time(NULL);
        char *timestr = ctime(&now);
        timestr[strlen(timestr)-1] = '\0';
        
        fprintf(log, "[%s] ", timestr);
        
        // AGGIUNGI QUESTA PARTE:
        va_list args;
        va_start(args, format);
        vfprintf(log, format, args);  // Usa vfprintf invece di fprintf
        va_end(args);
        
        fprintf(log, "\n");
        fflush(log);
        fclose(log);
    }
    
    pthread_mutex_unlock(&log_mutex);
}
```

**VARIABILI IMPORTANTI:**
- `pthread_mutex_t`: Tipo per mutex (sincronizzazione thread)
- `time_t`: Tipo per timestamp
- `fflush()`: Forza scrittura buffer su disco

## GESTIONE SEGNALI

### Segnali Importanti per Daemon
```c
#include <signal.h>

SIGTERM  // Terminazione gentile
SIGHUP   // Ricarica configurazione  
SIGINT   // Interruzione (Ctrl+C) - raramente ricevuto
SIGKILL  // Terminazione forzata (non catturabile)
```

### Handler per Terminazione Pulita
```c
volatile sig_atomic_t daemon_running = 1;

void signal_handler(int sig) {
    switch(sig) {
        case SIGTERM:
        case SIGINT:
            daemon_running = 0;  // Flag per terminazione
            break;
        case SIGHUP:
            // Ricarica configurazione
            reload_config();
            break;
    }
}

int main() {
    // Installa handler
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGHUP, signal_handler);
    
    // ... daemonizzazione ...
    
    // Main loop
    while (daemon_running) {
        // Lavoro del daemon
        sleep(1);
    }
    
    // Pulizia prima di terminare
    cleanup_resources();
    return 0;
}
```

**VARIABILI:**
- `volatile sig_atomic_t`: Tipo sicuro per variabili modificate da signal handler
- `volatile`: Dice al compilatore di non ottimizzare accessi alla variabile

## ESEMPI PRATICI

### Esempio 1: Daemon Minimale
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

void become_daemon() {
    pid_t pid = fork();
    if (pid > 0) exit(0);  // Padre termina
    if (pid < 0) exit(1);  // Errore
    
    setsid();              // Nuova sessione
    chdir("/");            // Directory root
    
    close(0); close(1); close(2);  // Chiudi std FDs
    open("/dev/null", O_RDONLY);   // stdin
    open("/dev/null", O_WRONLY);   // stdout
    open("/dev/null", O_WRONLY);   // stderr
}

int main() {
    printf("Diventando daemon...\n");
    
    become_daemon();
    
    // Ora sono un daemon
    FILE *log = fopen("/tmp/daemon.log", "w");
    fprintf(log, "Daemon avviato, PID: %d\n", getpid());
    fclose(log);
    
    // Simula lavoro
    for (int i = 0; i < 10; i++) {
        sleep(1);
    }
    
    return 0;
}
```

### Esempio 2: Daemon con Gestione Segnali
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <string.h>

volatile sig_atomic_t keep_running = 1;

void signal_handler(int sig) {
    keep_running = 0;
}

void daemon_log(const char *msg) {
    FILE *log = fopen("/tmp/daemon.log", "a");
    if (log) {
        time_t now = time(NULL);
        char *timestr = ctime(&now);
        timestr[strlen(timestr)-1] = '\0';
        fprintf(log, "[%s] %s\n", timestr, msg);
        fflush(log);
        fclose(log);
    }
}

void become_daemon() {
    pid_t pid = fork();
    if (pid > 0) {
        printf("Daemon creato con PID: %d\n", pid);
        exit(0);
    }
    if (pid < 0) {
        perror("fork");
        exit(1);
    }
    
    if (setsid() < 0) {
        perror("setsid");
        exit(1);
    }
    
    chdir("/");
    
    close(0); close(1); close(2);
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_WRONLY);
}

int main() {
    // Setup segnali PRIMA di diventare daemon
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    
    become_daemon();
    
    daemon_log("Daemon avviato");
    
    int counter = 0;
    while (keep_running) {
        char msg[100];
        snprintf(msg, sizeof(msg), "Iterazione %d", ++counter);
        daemon_log(msg);
        sleep(5);
    }
    
    daemon_log("Daemon terminato");
    return 0;
}
```

## ERRORI COMUNI E SOLUZIONI

### 1. Fork senza controllo errori
```c
// SBAGLIATO
fork();

// GIUSTO  
pid_t pid = fork();
if (pid < 0) {
    perror("fork failed");
    exit(1);
}
```

### 2. Non chiamare setsid()
```c
// SBAGLIATO - rimane legato al terminale
fork();
if (pid > 0) exit(0);

// GIUSTO
fork();
if (pid > 0) exit(0);
setsid();  // ESSENZIALE
```

### 3. Dimenticare fflush() nei log
```c
// SBAGLIATO - scrittura bufferizzata
fprintf(log, "messaggio\n");

// GIUSTO - forza scrittura immediata
fprintf(log, "messaggio\n");
fflush(log);
```

### 4. Non gestire segnali
```c
// SBAGLIATO - daemon non terminabile
while(1) {
    sleep(1);
}

// GIUSTO - terminazione controllata
volatile sig_atomic_t running = 1;
signal(SIGTERM, handler);
while(running) {
    sleep(1);
}
```

### 5. Directory di lavoro problematica
```c
// SBAGLIATO - può bloccare filesystem
// (non chiamare chdir)

// GIUSTO
chdir("/");  // Directory sempre disponibile
```

## DEBUGGING DAEMON

### Come Testare Durante Sviluppo
```c
#ifdef DEBUG
    // Non diventare daemon in modalità debug
    printf("Modalità debug - non divento daemon\n");
#else
    become_daemon();
#endif
```

**Compilazione:**
```bash
# Debug mode
gcc -DDEBUG -o daemon daemon.c

# Production mode  
gcc -o daemon daemon.c
```

### Verifica Daemon Attivo
```bash
# Trova il processo
ps aux | grep daemon_name

# Controlla log
tail -f /tmp/daemon.log

# Invia segnale di terminazione
kill -TERM <PID>
```

## CHECKLIST ESAME

✅ **Include necessarie:** unistd.h, sys/types.h, fcntl.h, signal.h
✅ **Fork e controllo errore:** Sempre controllare valore ritorno  
✅ **Setsid:** Chiamare dopo fork nel figlio
✅ **Chdir(/):** Cambiare directory di lavoro
✅ **Close standard FDs:** 0, 1, 2
✅ **Reindirizza /dev/null:** Aprire su FD 0, 1, 2
✅ **Logging:** Usare file, non stdout
✅ **Gestione segnali:** Handler per SIGTERM
✅ **Fflush:** Forzare scrittura log
✅ **Terminazione pulita:** Cleanup risorse

## TEMPLATE COMPLETO PER ESAME

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <string.h>

volatile sig_atomic_t daemon_running = 1;

void signal_handler(int sig) {
    daemon_running = 0;
}

void daemon_log(const char *message) {
    FILE *log = fopen("/tmp/daemon.log", "a");
    if (log) {
        time_t now = time(NULL);
        char *timestr = ctime(&now);
        timestr[strlen(timestr)-1] = '\0';
        fprintf(log, "[%s] %s\n", timestr, message);
        fflush(log);
        fclose(log);
    }
}

void become_daemon() {
    pid_t pid = fork();
    if (pid > 0) {
        printf("Daemon PID: %d\n", pid);
        exit(0);
    }
    if (pid < 0) {
        perror("fork");
        exit(1);
    }
    
    if (setsid() < 0) {
        perror("setsid");
        exit(1);
    }
    
    chdir("/");
    
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_WRONLY);
}

int main() {
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    
    become_daemon();
    
    daemon_log("Daemon avviato");
    
    while (daemon_running) {
        // Lavoro del daemon qui
        daemon_log("Daemon attivo");
        sleep(10);
    }
    
    daemon_log("Daemon terminato");
    return 0;
}
```