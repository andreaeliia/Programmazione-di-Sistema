# Appunti: Programmazione di Sistema - Misurazione Tempi e Esecuzione Processi

## 1. Esecuzione di Processi in C

### 1.1 System Call `system()`
La più semplice per eseguire comandi:

```c
#include <stdlib.h>
int system(const char *command);
```

**Esempio:**
```c
#include <stdio.h>
#include <stdlib.h>

int main() {
    int status = system("ls -l");
    if (status == -1) {
        perror("system");
        return 1;
    }
    printf("Comando terminato con status: %d\n", status);
    return 0;
}
```

### 1.2 Approccio con `fork()` + `exec()`
Più controllo sull'esecuzione:

```c
#include <unistd.h>
#include <sys/wait.h>

pid_t pid = fork();
if (pid == 0) {
    // Processo figlio
    execl("/bin/sh", "sh", "-c", comando, (char*)NULL);
    perror("exec");
    exit(1);
} else if (pid > 0) {
    // Processo padre
    int status;
    wait(&status);
} else {
    perror("fork");
}
```

## 2. Misurazione dei Tempi

### 2.1 Tipi di Tempo
- **Wall Clock Time (Tempo di orologio)**: Tempo reale trascorso
- **User Time**: Tempo CPU speso in modalità utente
- **System Time**: Tempo CPU speso in modalità kernel

### 2.2 System Call `times()`

```c
#include <sys/times.h>
#include <unistd.h>

struct tms {
    clock_t tms_utime;  // user time
    clock_t tms_stime;  // system time
    clock_t tms_cutime; // user time dei figli
    clock_t tms_cstime; // system time dei figli
};

clock_t times(struct tms *buf);
```

**Esempio di utilizzo:**
```c
#include <sys/times.h>
#include <unistd.h>
#include <stdio.h>

void misura_tempo_processo() {
    struct tms start, end;
    clock_t start_wall, end_wall;
    long ticks_per_sec = sysconf(_SC_CLK_TCK);
    
    start_wall = times(&start);
    
    // Esegui il comando qui
    system("sleep 1");
    
    end_wall = times(&end);
    
    double wall_time = (double)(end_wall - start_wall) / ticks_per_sec;
    double user_time = (double)(end.tms_cutime - start.tms_cutime) / ticks_per_sec;
    double sys_time = (double)(end.tms_cstime - start.tms_cstime) / ticks_per_sec;
    
    printf("Wall: %.6f, User: %.6f, Sys: %.6f\n", wall_time, user_time, sys_time);
}
```

### 2.3 `clock_gettime()` per Alta Precisione

```c
#include <time.h>

struct timespec {
    time_t tv_sec;   // secondi
    long tv_nsec;    // nanosecondi
};

int clock_gettime(clockid_t clk_id, struct timespec *tp);
```

**Esempio per tempo di orologio ad alta precisione:**
```c
#include <time.h>
#include <stdio.h>

double get_wall_time() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

void misura_wall_time() {
    double start = get_wall_time();
    
    // Esegui comando
    system("ls > /dev/null");
    
    double end = get_wall_time();
    double elapsed = end - start;
    
    printf("Tempo elapsed: %.6f secondi\n", elapsed);
}
```

### 2.4 `getrusage()` per Statistiche Dettagliate

```c
#include <sys/resource.h>

struct rusage {
    struct timeval ru_utime; // user time
    struct timeval ru_stime; // system time
    // ... altri campi per memoria, I/O, etc.
};

int getrusage(int who, struct rusage *usage);
```

**Esempio:**
```c
#include <sys/resource.h>
#include <sys/wait.h>
#include <stdio.h>

void misura_con_getrusage() {
    struct rusage usage;
    pid_t pid = fork();
    
    if (pid == 0) {
        execl("/bin/sleep", "sleep", "1", (char*)NULL);
    } else {
        wait(NULL);
        getrusage(RUSAGE_CHILDREN, &usage);
        
        double user_time = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1000000.0;
        double sys_time = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1000000.0;
        
        printf("User: %.6f, System: %.6f\n", user_time, sys_time);
    }
}
```

## 3. Conversioni di Tempo e Precisione

### 3.1 Conversioni Comuni
```c
// Da struct timeval a secondi (precisione microsecondo)
double timeval_to_seconds(struct timeval *tv) {
    return tv->tv_sec + tv->tv_usec / 1000000.0;
}

// Da struct timespec a secondi (precisione nanosecondo)
double timespec_to_seconds(struct timespec *ts) {
    return ts->tv_sec + ts->tv_nsec / 1000000000.0;
}

// Da clock_t a secondi
double clock_to_seconds(clock_t ticks) {
    return (double)ticks / sysconf(_SC_CLK_TCK);
}
```

### 3.2 Formattazione Output con Precisione Microsecondo
```c
// Stampa con 6 cifre decimali (precisione microsecondo)
printf("Tempo: %.6f secondi\n", tempo_in_secondi);

// Su stderr
fprintf(stderr, "Media tempi - Wall: %.6f, User: %.6f, Sys: %.6f\n", 
        wall_avg, user_avg, sys_avg);
```

## 4. Gestione Argomenti del Programma

### 4.1 Parsing Argomenti
```c
#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <comando>\n", argv[0]);
        return 1;
    }
    
    // argv[1] contiene il comando da eseguire
    char *comando = argv[1];
    
    // Se ci sono più argomenti, ricostruisci il comando completo
    char comando_completo[1024] = "";
    for (int i = 1; i < argc; i++) {
        strcat(comando_completo, argv[i]);
        if (i < argc - 1) strcat(comando_completo, " ");
    }
    
    return 0;
}
```

## 5. Calcoli Statistici

### 5.1 Media di un Array
```c
double calcola_media(double *valori, int n) {
    double somma = 0.0;
    for (int i = 0; i < n; i++) {
        somma += valori[i];
    }
    return somma / n;
}
```

### 5.2 Esempio con Array di Tempi
```c
#define NUM_ESECUZIONI 10

int main() {
    double wall_times[NUM_ESECUZIONI];
    double user_times[NUM_ESECUZIONI];
    double sys_times[NUM_ESECUZIONI];
    
    for (int i = 0; i < NUM_ESECUZIONI; i++) {
        // Misura tempi per l'esecuzione i-esima
        // wall_times[i] = ...
        // user_times[i] = ...
        // sys_times[i] = ...
    }
    
    double wall_avg = calcola_media(wall_times, NUM_ESECUZIONI);
    double user_avg = calcola_media(user_times, NUM_ESECUZIONI);
    double sys_avg = calcola_media(sys_times, NUM_ESECUZIONI);
    
    fprintf(stderr, "Medie - Wall: %.6f, User: %.6f, Sys: %.6f\n", 
            wall_avg, user_avg, sys_avg);
    
    return 0;
}
```

## 6. Gestione Errori

### 6.1 Controllo Errori System Call
```c
#include <errno.h>
#include <string.h>

// Esempio con system()
int status = system(comando);
if (status == -1) {
    fprintf(stderr, "Errore nell'esecuzione: %s\n", strerror(errno));
    return 1;
}

// Esempio con clock_gettime()
struct timespec ts;
if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
    perror("clock_gettime");
    return 1;
}
```

## 7. Pattern Completo per Misurazione

### 7.1 Template di Misurazione
```c
typedef struct {
    double wall_time;
    double user_time;
    double sys_time;
} tempo_esecuzione_t;

tempo_esecuzione_t misura_esecuzione(char *comando) {
    tempo_esecuzione_t risultato = {0, 0, 0};
    
    // Tempo di inizio (wall clock)
    struct timespec start_wall, end_wall;
    clock_gettime(CLOCK_REALTIME, &start_wall);
    
    // Esegui comando e misura user/sys time
    struct rusage usage;
    pid_t pid = fork();
    
    if (pid == 0) {
        // Figlio: esegui comando
        execl("/bin/sh", "sh", "-c", comando, (char*)NULL);
        perror("exec");
        exit(1);
    } else if (pid > 0) {
        // Padre: aspetta e misura
        wait(NULL);
        getrusage(RUSAGE_CHILDREN, &usage);
        
        // Tempo di fine (wall clock)
        clock_gettime(CLOCK_REALTIME, &end_wall);
        
        // Calcola risultati
        risultato.wall_time = timespec_to_seconds(&end_wall) - timespec_to_seconds(&start_wall);
        risultato.user_time = timeval_to_seconds(&usage.ru_utime);
        risultato.sys_time = timeval_to_seconds(&usage.ru_stime);
    } else {
        perror("fork");
    }
    
    return risultato;
}
```

## 8. Esercizi Affini - Variazioni sul Tema

### 8.1 Possibili Variazioni
1. **Statistiche aggiuntive**: Calcolare anche deviazione standard, min, max
2. **Diversi comandi**: Eseguire più comandi diversi e confrontarli
3. **Limiti di risorse**: Misurare anche memoria, I/O, page fault
4. **Scheduling**: Cambiare priorità del processo e misurare l'impatto
5. **Parallel execution**: Eseguire comandi in parallelo invece che sequenzialmente
6. **Output dettagliato**: Salvare tutti i tempi su file CSV
7. **Timeout**: Gestire comandi che impiegano troppo tempo
8. **Sampling**: Misurare a intervalli regolari durante l'esecuzione

### 8.2 System Call Correlate da Conoscere
- `setrlimit()`: Impostare limiti di risorse
- `getrlimit()`: Ottenere limiti di risorse  
- `setpriority()`: Cambiare priorità processo
- `sched_setscheduler()`: Cambiare algoritmo di scheduling
- `wait4()`: Come wait() ma con statistiche rusage
- `waitpid()`: Aspettare un processo specifico

### 8.3 Macro e Costanti Utili
```c
// Per sysconf()
_SC_CLK_TCK         // Ticks per secondo
_SC_PAGESIZE        // Dimensione pagina
_SC_NPROCESSORS_ONLN // Numero di CPU

// Per clock_gettime()
CLOCK_REALTIME      // Tempo reale
CLOCK_PROCESS_CPUTIME_ID // CPU time del processo
CLOCK_MONOTONIC     // Tempo monotono

// Per getrusage()
RUSAGE_SELF         // Statistiche del processo corrente
RUSAGE_CHILDREN     // Statistiche dei processi figli
```

## 9. Debugging e Testing

### 9.1 Comandi di Test Utili
```bash
# Comando veloce
./programma "echo hello"

# Comando con tempo misurabile
./programma "sleep 0.1"

# Comando CPU-intensive
./programma "dd if=/dev/zero of=/dev/null bs=1M count=100"

# Comando che usa molto sistema
./programma "find /usr -name '*.so' 2>/dev/null | wc -l"
```

### 9.2 Verifica Risultati
Puoi confrontare con il comando `time`:
```bash
time comando
```

E verificare che i tuoi risultati siano consistenti.