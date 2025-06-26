# Process Management - Riferimento Completo

## 🎯 Concetti Fondamentali

### **Cos'è un Processo?**
- **Programma in esecuzione**: istanza di un programma caricato in memoria
- **PID**: Process IDentifier univoco
- **PPID**: Parent Process ID (processo genitore)
- **Memory space**: spazio di memoria virtuale dedicato
- **File descriptors**: tabella dei file aperti

### **Ciclo di Vita Processo**
1. **Creation**: fork() o exec()
2. **Running**: esecuzione codice
3. **Waiting**: attesa I/O o eventi
4. **Zombie**: terminato ma non ancora "raccolto" dal padre
5. **Terminated**: completamente rimosso dal sistema

### **Stati Processo**
- **R** (Running): in esecuzione o pronto
- **S** (Sleeping): in attesa interrompibile
- **D** (Disk sleep): in attesa non interrompibile
- **Z** (Zombie): terminato ma non raccolto
- **T** (Stopped): fermato da segnale

---

## 🔧 Fork - Creazione Processi

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

// Esempio base fork
void basic_fork_example() {
    pid_t pid;
    
    printf("=== ESEMPIO FORK BASE ===\n");
    printf("Prima del fork - PID: %d, PPID: %d\n", getpid(), getppid());
    
    pid = fork();
    
    if (pid < 0) {
        // ERRORE FORK
        perror("fork failed");
        exit(EXIT_FAILURE);
        
    } else if (pid == 0) {
        // PROCESSO FIGLIO
        printf("FIGLIO - PID: %d, PPID: %d, fork() returned: %d\n", 
               getpid(), getppid(), pid);
        
        // Lavoro del figlio
        for (int i = 1; i <= 3; i++) {
            printf("Figlio: iterazione %d\n", i);
            sleep(1);
        }
        
        printf("Figlio termina\n");
        exit(42);  // Exit status personalizzato
        
    } else {
        // PROCESSO PADRE
        printf("PADRE - PID: %d, PPID: %d, child PID: %d\n", 
               getpid(), getppid(), pid);
        
        // Lavoro del padre
        for (int i = 1; i <= 2; i++) {
            printf("Padre: iterazione %d\n", i);
            sleep(1);
        }
        
        // Aspetta terminazione figlio
        int status;
        pid_t child_pid = wait(&status);
        
        printf("Padre: figlio %d terminato con status %d\n", child_pid, status);
        
        if (WIFEXITED(status)) {
            printf("Padre: figlio uscito normalmente con codice %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Padre: figlio terminato da segnale %d\n", WTERMSIG(status));
        }
    }
}

// Fork multipli con gestione errori
void multiple_fork_example() {
    printf("\n=== FORK MULTIPLI ===\n");
    
    int num_children = 3;
    pid_t children[num_children];
    
    printf("Padre: creo %d processi figlio\n", num_children);
    
    for (int i = 0; i < num_children; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("fork failed");
            // Termina figli già creati
            for (int j = 0; j < i; j++) {
                kill(children[j], SIGTERM);
            }
            exit(EXIT_FAILURE);
            
        } else if (pid == 0) {
            // PROCESSO FIGLIO
            printf("Figlio %d: PID %d avviato\n", i, getpid());
            
            // Simula lavoro diverso per ogni figlio
            int work_time = 2 + i;
            printf("Figlio %d: lavoro per %d secondi\n", i, work_time);
            sleep(work_time);
            
            printf("Figlio %d: completato\n", i);
            exit(i + 10);  // Exit code distintivo
            
        } else {
            // PROCESSO PADRE
            children[i] = pid;
            printf("Padre: creato figlio %d con PID %d\n", i, pid);
        }
    }
    
    // Padre aspetta tutti i figli
    printf("Padre: aspetto terminazione di tutti i figli\n");
    
    for (int i = 0; i < num_children; i++) {
        int status;
        pid_t terminated_pid = wait(&status);
        
        printf("Padre: figlio PID %d terminato", terminated_pid);
        
        if (WIFEXITED(status)) {
            printf(" con exit code %d\n", WEXITSTATUS(status));
        } else {
            printf(" in modo anomalo\n");
        }
    }
    
    printf("Padre: tutti i figli terminati\n");
}

// Fork con condivisione dati (attenzione!)
void fork_data_sharing_example() {
    printf("\n=== CONDIVISIONE DATI CON FORK ===\n");
    
    int shared_var = 100;        // Variabile "condivisa"
    int *heap_var = malloc(sizeof(int));
    *heap_var = 200;
    
    printf("Prima del fork: shared_var=%d, heap_var=%d\n", shared_var, *heap_var);
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // FIGLIO - modifica variabili
        printf("Figlio: modifico variabili\n");
        shared_var = 999;
        *heap_var = 888;
        
        printf("Figlio: shared_var=%d, heap_var=%d\n", shared_var, *heap_var);
        
        free(heap_var);  // Ogni processo deve fare free della sua copia
        exit(0);
        
    } else {
        // PADRE - aspetta e controlla variabili
        wait(NULL);
        
        printf("Padre dopo wait: shared_var=%d, heap_var=%d\n", shared_var, *heap_var);
        printf("Padre: le modifiche del figlio NON sono visibili (memory copy)\n");
        
        free(heap_var);
    }
}

// Fork con pipe per comunicazione
void fork_with_pipe_example() {
    printf("\n=== FORK CON PIPE ===\n");
    
    int pipefd[2];
    pid_t pid;
    char message[] = "Messaggio dal padre al figlio";
    char buffer[256];
    
    // Crea pipe PRIMA del fork
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return;
    }
    
    pid = fork();
    
    if (pid == 0) {
        // FIGLIO - legge dalla pipe
        close(pipefd[1]);  // Chiudi scrittura
        
        printf("Figlio: aspetto messaggio dalla pipe...\n");
        
        int bytes_read = read(pipefd[0], buffer, sizeof(buffer));
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("Figlio: ricevuto '%s'\n", buffer);
        }
        
        close(pipefd[0]);
        exit(0);
        
    } else {
        // PADRE - scrive nella pipe
        close(pipefd[0]);  // Chiudi lettura
        
        printf("Padre: invio messaggio...\n");
        write(pipefd[1], message, strlen(message));
        
        close(pipefd[1]);  // Chiudi per segnalare EOF
        wait(NULL);
        
        printf("Padre: comunicazione completata\n");
    }
}

// Gestione errori avanzata con fork
pid_t safe_fork() {
    pid_t pid;
    int retry_count = 0;
    const int max_retries = 3;
    
    while (retry_count < max_retries) {
        pid = fork();
        
        if (pid >= 0) {
            return pid;  // Successo
        }
        
        if (errno == EAGAIN || errno == ENOMEM) {
            // Errore temporaneo - retry
            retry_count++;
            printf("Fork fallito (tentativo %d/%d): %s\n", 
                   retry_count, max_retries, strerror(errno));
            sleep(1);  // Aspetta prima di riprovare
        } else {
            // Errore fatale
            perror("fork - errore fatale");
            return -1;
        }
    }
    
    printf("Fork fallito dopo %d tentativi\n", max_retries);
    return -1;
}

// Fork bomb protection (esempio educativo - NON ESEGUIRE!)
void fork_bomb_protection_example() {
    printf("\n=== PROTEZIONE FORK BOMB (DEMO) ===\n");
    
    // Limite massimo processi figlio
    const int max_processes = 5;
    int active_processes = 0;
    
    for (int i = 0; i < 10; i++) {  // Prova a creare 10 processi
        if (active_processes >= max_processes) {
            printf("Limite processi raggiunto (%d), aspetto...\n", max_processes);
            
            // Aspetta che almeno un processo termini
            wait(NULL);
            active_processes--;
        }
        
        pid_t pid = fork();
        
        if (pid == 0) {
            // Figlio - simula lavoro breve
            printf("Processo figlio %d (PID %d) avviato\n", i, getpid());
            sleep(2);
            printf("Processo figlio %d terminato\n", i);
            exit(0);
            
        } else if (pid > 0) {
            active_processes++;
            printf("Creato processo %d/%d (PID %d)\n", i+1, 10, pid);
            
        } else {
            perror("fork");
            break;
        }
    }
    
    // Aspetta tutti i processi rimanenti
    while (active_processes > 0) {
        wait(NULL);
        active_processes--;
    }
    
    printf("Tutti i processi terminati\n");
}

// MAIN per esempi fork
int main() {
    printf("ESEMPI FORK - Process Management\n");
    printf("=================================\n\n");
    
    basic_fork_example();
    sleep(1);
    
    multiple_fork_example();
    sleep(1);
    
    fork_data_sharing_example();
    sleep(1);
    
    fork_with_pipe_example();
    sleep(1);
    
    fork_bomb_protection_example();
    
    printf("\nTutti gli esempi fork completati\n");
    return 0;
}
```

---

## ⚡ Exec - Sostituzione Programma

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

// Esempio exec base
void basic_exec_example() {
    printf("=== ESEMPIO EXEC BASE ===\n");
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // FIGLIO - esegue comando esterno
        printf("Figlio: eseguo comando 'ls -la'\n");
        
        // execl: lista argomenti terminata da NULL
        execl("/bin/ls", "ls", "-la", NULL);
        
        // Se arrivi qui, exec è fallito
        perror("execl fallito");
        exit(EXIT_FAILURE);
        
    } else if (pid > 0) {
        // PADRE - aspetta completamento
        int status;
        wait(&status);
        
        printf("Padre: comando completato con status %d\n", status);
        
    } else {
        perror("fork");
    }
}

// Diverse varianti di exec
void exec_variants_example() {
    printf("\n=== VARIANTI EXEC ===\n");
    
    // Array per processi figlio
    const int num_tests = 4;
    pid_t children[num_tests];
    
    // Test 1: execl
    children[0] = fork();
    if (children[0] == 0) {
        printf("Test 1 - execl: echo con parametri\n");
        execl("/bin/echo", "echo", "Ciao", "dal", "figlio", "1", NULL);
        perror("execl");
        exit(1);
    }
    
    // Test 2: execlp (cerca in PATH)
    children[1] = fork();
    if (children[1] == 0) {
        printf("Test 2 - execlp: date\n");
        execlp("date", "date", "+%Y-%m-%d %H:%M:%S", NULL);
        perror("execlp");
        exit(1);
    }
    
    // Test 3: execv (array di argomenti)
    children[2] = fork();
    if (children[2] == 0) {
        printf("Test 3 - execv: wc con array\n");
        char *args[] = {"wc", "-l", "/etc/passwd", NULL};
        execv("/usr/bin/wc", args);
        perror("execv");
        exit(1);
    }
    
    // Test 4: execvp (array + PATH)
    children[3] = fork();
    if (children[3] == 0) {
        printf("Test 4 - execvp: ps\n");
        char *args[] = {"ps", "aux", NULL};
        execvp("ps", args);
        perror("execvp");
        exit(1);
    }
    
    // Padre aspetta tutti i figli
    for (int i = 0; i < num_tests; i++) {
        int status;
        pid_t completed = wait(&status);
        printf("Padre: processo completato (PID %d, status %d)\n", completed, status);
    }
}

// Exec con controllo environment
void exec_with_environment() {
    printf("\n=== EXEC CON ENVIRONMENT ===\n");
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // Figlio - modifica environment
        printf("Figlio: eseguo comando con environment personalizzato\n");
        
        // Crea environment personalizzato
        char *env[] = {
            "PATH=/bin:/usr/bin",
            "HOME=/tmp",
            "USER=testuser",
            "CUSTOM_VAR=Hello World",
            NULL
        };
        
        // execle: lista args + environment
        execle("/usr/bin/env", "env", NULL, env);
        
        perror("execle");
        exit(1);
        
    } else {
        wait(NULL);
        printf("Padre: comando environment completato\n");
    }
}

// Exec con gestione errori
int safe_exec(const char* program, char* const args[]) {
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork per exec");
        return -1;
    }
    
    if (pid == 0) {
        // Figlio - esegue programma
        execvp(program, args);
        
        // Se arriviamo qui, exec è fallito
        fprintf(stderr, "Impossibile eseguire '%s': %s\n", program, strerror(errno));
        exit(127);  // Exit code standard per "command not found"
    }
    
    // Padre - aspetta e verifica risultato
    int status;
    if (wait(&status) != pid) {
        perror("wait");
        return -1;
    }
    
    if (WIFEXITED(status)) {
        int exit_code = WEXITSTATUS(status);
        if (exit_code == 127) {
            printf("Comando '%s' non trovato\n", program);
            return -1;
        }
        return exit_code;
    } else if (WIFSIGNALED(status)) {
        printf("Comando '%s' terminato da segnale %d\n", program, WTERMSIG(status));
        return -1;
    }
    
    return 0;
}

// Shell semplice con exec
void simple_shell_example() {
    printf("\n=== SHELL SEMPLICE ===\n");
    printf("Comandi: scrivi comando, 'exit' per uscire\n");
    
    char input[256];
    char *args[64];
    
    while (1) {
        printf("simple_shell> ");
        fflush(stdout);
        
        // Leggi input
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        
        // Rimuovi newline
        input[strcspn(input, "\n")] = 0;
        
        // Comando vuoto
        if (strlen(input) == 0) {
            continue;
        }
        
        // Comando exit
        if (strcmp(input, "exit") == 0) {
            break;
        }
        
        // Parse semplice (separato da spazi)
        int argc = 0;
        char *token = strtok(input, " ");
        
        while (token != NULL && argc < 63) {
            args[argc++] = token;
            token = strtok(NULL, " ");
        }
        args[argc] = NULL;
        
        if (argc > 0) {
            // Esegui comando
            int result = safe_exec(args[0], args);
            printf("Comando completato con codice %d\n", result);
        }
    }
    
    printf("Shell terminata\n");
}

// Esempio completo: processo che spawna worker
void master_worker_example() {
    printf("\n=== MASTER-WORKER PATTERN ===\n");
    
    const int num_workers = 3;
    const char* tasks[] = {"sleep 1", "echo Task1", "echo Task2", "date", "whoami"};
    const int num_tasks = sizeof(tasks) / sizeof(tasks[0]);
    
    printf("Master: avvio %d worker per %d task\n", num_workers, num_tasks);
    
    for (int task = 0; task < num_tasks; task++) {
        printf("Master: assegno task '%s'\n", tasks[task]);
        
        pid_t worker = fork();
        
        if (worker == 0) {
            // Worker - esegue task
            printf("Worker %d (PID %d): eseguo '%s'\n", task, getpid(), tasks[task]);
            
            // Parse comando semplice
            char task_copy[256];
            strncpy(task_copy, tasks[task], sizeof(task_copy));
            
            char *args[16];
            int argc = 0;
            char *token = strtok(task_copy, " ");
            
            while (token && argc < 15) {
                args[argc++] = token;
                token = strtok(NULL, " ");
            }
            args[argc] = NULL;
            
            if (argc > 0) {
                execvp(args[0], args);
            }
            
            perror("exec task");
            exit(1);
            
        } else if (worker > 0) {
            // Master - continua con prossimo task
            printf("Master: worker %d avviato (PID %d)\n", task, worker);
            
        } else {
            perror("fork worker");
        }
        
        // Limita workers concorrenti
        if ((task + 1) % num_workers == 0 || task == num_tasks - 1) {
            // Aspetta workers correnti
            for (int w = 0; w < num_workers && task - w >= 0; w++) {
                int status;
                pid_t completed = wait(&status);
                printf("Master: worker completato (PID %d)\n", completed);
            }
        }
    }
    
    printf("Master: tutti i task completati\n");
}

// MAIN per esempi exec
int main() {
    printf("ESEMPI EXEC - Process Management\n");
    printf("=================================\n\n");
    
    basic_exec_example();
    sleep(1);
    
    exec_variants_example();
    sleep(1);
    
    exec_with_environment();
    sleep(1);
    
    master_worker_example();
    
    printf("\nPer testare la shell semplice, decommenta la riga seguente:\n");
    // simple_shell_example();  // Decommenta per testare
    
    printf("\nTutti gli esempi exec completati\n");
    return 0;
}
```

---

## 🕐 Wait - Gestione Terminazione Processi

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

// Wait base
void basic_wait_example() {
    printf("=== WAIT BASE ===\n");
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // Figlio - simula lavoro
        printf("Figlio: lavoro per 3 secondi\n");
        sleep(3);
        printf("Figlio: termino con exit code 42\n");
        exit(42);
        
    } else {
        // Padre - wait semplice
        printf("Padre: aspetto figlio...\n");
        
        int status;
        pid_t child_pid = wait(&status);
        
        printf("Padre: figlio %d terminato\n", child_pid);
        
        // Analizza status
        if (WIFEXITED(status)) {
            printf("Padre: uscita normale, exit code = %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Padre: terminato da segnale %d\n", WTERMSIG(status));
            if (WCOREDUMP(status)) {
                printf("Padre: core dump generato\n");
            }
        } else if (WIFSTOPPED(status)) {
            printf("Padre: fermato da segnale %d\n", WSTOPSIG(status));
        }
    }
}

// Waitpid per controllo specifico
void waitpid_example() {
    printf("\n=== WAITPID SPECIFICO ===\n");
    
    pid_t children[3];
    
    // Crea 3 figli con tempi diversi
    for (int i = 0; i < 3; i++) {
        children[i] = fork();
        
        if (children[i] == 0) {
            // Figlio
            int sleep_time = (i + 1) * 2;  // 2, 4, 6 secondi
            printf("Figlio %d (PID %d): dormo %d secondi\n", i, getpid(), sleep_time);
            sleep(sleep_time);
            printf("Figlio %d: termino\n", i);
            exit(i + 10);
        }
    }
    
    // Padre aspetta specifico figlio (ultimo creato)
    printf("Padre: aspetto specificamente figlio %d (PID %d)\n", 2, children[2]);
    
    int status;
    pid_t waited = waitpid(children[2], &status, 0);
    
    printf("Padre: figlio specifico %d terminato con status %d\n", waited, WEXITSTATUS(status));
    
    // Aspetta gli altri
    printf("Padre: aspetto gli altri figli...\n");
    while (wait(NULL) > 0) {
        printf("Padre: un altro figlio terminato\n");
    }
}

// Wait non-bloccante
void nonblocking_wait_example() {
    printf("\n=== WAIT NON-BLOCCANTE ===\n");
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // Figlio - lavoro lungo
        printf("Figlio: lavoro lungo (10 secondi)\n");
        sleep(10);
        exit(0);
        
    } else {
        // Padre - controllo periodico
        printf("Padre: controllo periodico figlio...\n");
        
        int status;
        pid_t result;
        
        do {
            result = waitpid(pid, &status, WNOHANG);  // Non-bloccante
            
            if (result == 0) {
                printf("Padre: figlio ancora in esecuzione...\n");
                sleep(2);  // Fai altro lavoro
            } else if (result == pid) {
                printf("Padre: figlio terminato!\n");
                break;
            } else {
                perror("waitpid");
                break;
            }
            
        } while (1);
    }
}

// Gestione zombie prevention
void zombie_prevention_example() {
    printf("\n=== PREVENZIONE ZOMBIE ===\n");
    
    // Signal handler per SIGCHLD
    signal(SIGCHLD, SIG_IGN);  // Automatico cleanup figli
    
    printf("Padre: creo figli che terminano rapidamente\n");
    
    for (int i = 0; i < 5; i++) {
        pid_t pid = fork();
        
        if (pid == 0) {
            // Figlio - termina subito
            printf("Figlio %d: termino immediatamente\n", i);
            exit(i);
            
        } else {
            printf("Padre: creato figlio %d (PID %d)\n", i, pid);
        }
        
        sleep(1);  // Pausa tra creazioni
    }
    
    printf("Padre: tutti i figli creati, nessun zombie dovrebbe esistere\n");
    
    sleep(2);
    printf("Padre: fine esempio\n");
}

// Wait con timeout
int wait_with_timeout(pid_t pid, int *status, int timeout_seconds) {
    time_t start_time = time(NULL);
    
    while (1) {
        pid_t result = waitpid(pid, status, WNOHANG);
        
        if (result == pid) {
            return 1;  // Processo terminato
        } else if (result < 0) {
            return -1;  // Errore
        }
        
        // Controlla timeout
        if (time(NULL) - start_time >= timeout_seconds) {
            return 0;  // Timeout
        }
        
        usleep(100000);  // 100ms
    }
}

void wait_timeout_example() {
    printf("\n=== WAIT CON TIMEOUT ===\n");
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // Figlio - lavoro variabile
        int work_time = 7;
        printf("Figlio: lavoro per %d secondi\n", work_time);
        sleep(work_time);
        exit(0);
        
    } else {
        // Padre - wait con timeout
        int timeout = 5;
        printf("Padre: aspetto figlio con timeout %d secondi\n", timeout);
        
        int status;
        int result = wait_with_timeout(pid, &status, timeout);
        
        if (result == 1) {
            printf("Padre: figlio terminato entro timeout\n");
        } else if (result == 0) {
            printf("Padre: timeout raggiunto, termino figlio\n");
            kill(pid, SIGTERM);
            sleep(1);
            
            // Verifica terminazione
            if (waitpid(pid, &status, WNOHANG) == 0) {
                printf("Padre: figlio non risponde, forzo terminazione\n");
                kill(pid, SIGKILL);
            }
            
            wait(&status);  // Cleanup finale
        } else {
            printf("Padre: errore wait\n");
        }
    }
}

// Wait per gruppi di processi
void process_group_wait_example() {
    printf("\n=== WAIT GRUPPO PROCESSI ===\n");
    
    // Crea nuovo gruppo di processi
    pid_t group_leader = fork();
    
    if (group_leader == 0) {
        // Figlio diventa leader gruppo
        setpgid(0, 0);  // Crea nuovo gruppo con PID come PGID
        
        printf("Leader gruppo (PID %d, PGID %d)\n", getpid(), getpgrp());
        
        // Crea altri membri del gruppo
        for (int i = 0; i < 2; i++) {
            pid_t member = fork();
            
            if (member == 0) {
                printf("Membro gruppo %d (PID %d, PGID %d)\n", i, getpid(), getpgrp());
                sleep(3 + i);
                exit(0);
            }
        }
        
        // Leader aspetta membri
        while (wait(NULL) > 0) {
            printf("Leader: un membro terminato\n");
        }
        
        printf("Leader: tutti i membri terminati, esco\n");
        exit(0);
        
    } else {
        // Padre originale
        printf("Padre: creato gruppo con leader PID %d\n", group_leader);
        
        // Aspetta terminazione gruppo
        wait(NULL);
        printf("Padre: gruppo terminato\n");
    }
}

// MAIN per esempi wait
int main() {
    printf("ESEMPI WAIT - Process Management\n");
    printf("=================================\n\n");
    
    basic_wait_example();
    sleep(1);
    
    waitpid_example();
    sleep(1);
    
    nonblocking_wait_example();
    sleep(1);
    
    zombie_prevention_example();
    sleep(1);
    
    wait_timeout_example();
    sleep(1);
    
    process_group_wait_example();
    
    printf("\nTutti gli esempi wait completati\n");
    return 0;
}
```

---

## 🛠️ Utility Process Management

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <string.h>

// Struttura per gestire pool di processi
typedef struct {
    pid_t *pids;
    int *statuses;
    int capacity;
    int count;
    time_t *start_times;
} ProcessPool;

ProcessPool* create_process_pool(int capacity) {
    ProcessPool* pool = malloc(sizeof(ProcessPool));
    pool->pids = calloc(capacity, sizeof(pid_t));
    pool->statuses = calloc(capacity, sizeof(int));
    pool->start_times = calloc(capacity, sizeof(time_t));
    pool->capacity = capacity;
    pool->count = 0;
    return pool;
}

int add_process_to_pool(ProcessPool* pool, pid_t pid) {
    if (pool->count >= pool->capacity) {
        return -1;  // Pool pieno
    }
    
    pool->pids[pool->count] = pid;
    pool->start_times[pool->count] = time(NULL);
    pool->count++;
    return 0;
}

int wait_any_process(ProcessPool* pool) {
    if (pool->count == 0) {
        return -1;  // Nessun processo
    }
    
    int status;
    pid_t completed = wait(&status);
    
    if (completed > 0) {
        // Trova e rimuovi dal pool
        for (int i = 0; i < pool->count; i++) {
            if (pool->pids[i] == completed) {
                pool->statuses[i] = status;
                
                // Sposta ultimo elemento in questa posizione
                pool->pids[i] = pool->pids[pool->count - 1];
                pool->start_times[i] = pool->start_times[pool->count - 1];
                pool->count--;
                
                return completed;
            }
        }
    }
    
    return -1;
}

void cleanup_process_pool(ProcessPool* pool) {
    // Termina tutti i processi rimanenti
    for (int i = 0; i < pool->count; i++) {
        kill(pool->pids[i], SIGTERM);
    }
    
    // Aspetta terminazione
    while (pool->count > 0) {
        wait_any_process(pool);
    }
    
    free(pool->pids);
    free(pool->statuses);
    free(pool->start_times);
    free(pool);
}

// Process launcher sicuro
typedef struct {
    char *program;
    char **args;
    char **env;
    int timeout_seconds;
    int capture_output;
} ProcessConfig;

typedef struct {
    pid_t pid;
    int exit_code;
    char *output;
    size_t output_length;
    int timed_out;
} ProcessResult;

// Wait con timeout per utility
int wait_with_timeout_util(pid_t pid, int *status, int timeout_seconds) {
    time_t start_time = time(NULL);
    
    while (1) {
        pid_t result = waitpid(pid, status, WNOHANG);
        
        if (result == pid) {
            return 1;  // Processo terminato
        } else if (result < 0) {
            return -1;  // Errore
        }
        
        if (time(NULL) - start_time >= timeout_seconds) {
            return 0;  // Timeout
        }
        
        usleep(100000);  // 100ms
    }
}

ProcessResult* launch_process(ProcessConfig* config) {
    ProcessResult* result = calloc(1, sizeof(ProcessResult));
    int output_pipe[2] = {-1, -1};
    
    // Setup pipe per output se richiesto
    if (config->capture_output) {
        if (pipe(output_pipe) < 0) {
            perror("pipe for output");
            free(result);
            return NULL;
        }
    }
    
    result->pid = fork();
    
    if (result->pid == 0) {
        // Figlio
        if (config->capture_output) {
            close(output_pipe[0]);  // Chiudi lettura
            dup2(output_pipe[1], STDOUT_FILENO);  // Redirect stdout
            dup2(output_pipe[1], STDERR_FILENO);  // Redirect stderr
            close(output_pipe[1]);
        }
        
        if (config->env) {
            execve(config->program, config->args, config->env);
        } else {
            execvp(config->program, config->args);
        }
        
        perror("exec");
        exit(127);
        
    } else if (result->pid > 0) {
        // Padre
        if (config->capture_output) {
            close(output_pipe[1]);  // Chiudi scrittura
            
            // Leggi output
            char buffer[4096];
            size_t total_read = 0;
            int bytes_read;
            
            result->output = malloc(4096);
            
            while ((bytes_read = read(output_pipe[0], buffer, sizeof(buffer))) > 0) {
                result->output = realloc(result->output, total_read + bytes_read + 1);
                memcpy(result->output + total_read, buffer, bytes_read);
                total_read += bytes_read;
            }
            
            if (result->output) {
                result->output[total_read] = '\0';
                result->output_length = total_read;
            }
            
            close(output_pipe[0]);
        }
        
        // Wait con timeout
        int status;
        if (config->timeout_seconds > 0) {
            int wait_result = wait_with_timeout_util(result->pid, &status, config->timeout_seconds);
            
            if (wait_result == 0) {
                // Timeout
                result->timed_out = 1;
                kill(result->pid, SIGTERM);
                sleep(1);
                if (waitpid(result->pid, &status, WNOHANG) == 0) {
                    kill(result->pid, SIGKILL);
                }
                wait(&status);
            }
        } else {
            wait(&status);
        }
        
        if (WIFEXITED(status)) {
            result->exit_code = WEXITSTATUS(status);
        } else {
            result->exit_code = -1;
        }
        
    } else {
        // Errore fork
        perror("fork");
        if (config->capture_output) {
            close(output_pipe[0]);
            close(output_pipe[1]);
        }
        free(result);
        return NULL;
    }
    
    return result;
}

void free_process_result(ProcessResult* result) {
    if (result) {
        free(result->output);
        free(result);
    }
}

// Esempio uso ProcessPool
void process_pool_example() {
    printf("=== PROCESS POOL EXAMPLE ===\n");
    
    ProcessPool* pool = create_process_pool(5);
    
    // Crea alcuni processi worker
    for (int i = 0; i < 3; i++) {
        pid_t pid = fork();
        
        if (pid == 0) {
            // Worker
            printf("Worker %d (PID %d) iniziato\n", i, getpid());
            sleep(2 + i);
            printf("Worker %d completato\n", i);
            exit(i);
        } else {
            add_process_to_pool(pool, pid);
            printf("Aggiunto worker %d al pool\n", i);
        }
    }
    
    // Aspetta tutti i processi
    while (pool->count > 0) {
        pid_t completed = wait_any_process(pool);
        printf("Processo %d terminato dal pool\n", completed);
    }
    
    cleanup_process_pool(pool);
    printf("Pool processi pulito\n");
}

// Esempio uso ProcessConfig
void process_config_example() {
    printf("\n=== PROCESS CONFIG EXAMPLE ===\n");
    
    // Configura processo
    ProcessConfig config = {0};
    config.program = "/bin/echo";
    
    char *args[] = {"echo", "Hello from process launcher", NULL};
    config.args = args;
    config.timeout_seconds = 5;
    config.capture_output = 1;
    
    // Lancia processo
    ProcessResult* result = launch_process(&config);
    
    if (result) {
        printf("Processo lanciato (PID %d)\n", result->pid);
        printf("Exit code: %d\n", result->exit_code);
        printf("Timed out: %s\n", result->timed_out ? "Sì" : "No");
        
        if (result->output) {
            printf("Output: %s\n", result->output);
        }
        
        free_process_result(result);
    }
}

// MAIN per esempi utility
int main() {
    printf("ESEMPI UTILITY - Process Management\n");
    printf("====================================\n\n");
    
    process_pool_example();
    sleep(1);
    
    process_config_example();
    
    printf("\nTutti gli esempi utility completati\n");
    return 0;
}
```

---

## 📋 Checklist Process Management

### ✅ **Fork**
- [ ] Controlla valore di ritorno fork()
- [ ] Gestisci errore (pid < 0)
- [ ] Codice separato per padre (pid > 0) e figlio (pid == 0)
- [ ] Exit esplicito nel figlio
- [ ] Wait nel padre per evitare zombie

### ✅ **Exec**
- [ ] Fork prima di exec per non sostituire processo corrente
- [ ] Usa variante exec appropriata (execl, execv, execp)
- [ ] Gestisci errore exec (non ritorna se successo)
- [ ] Path assoluto o PATH environment
- [ ] NULL-terminate lista argomenti

### ✅ **Wait**
- [ ] Wait dopo fork per raccogliere exit status
- [ ] Controlla tipo terminazione (WIFEXITED, WIFSIGNALED)
- [ ] Usa WNOHANG per wait non-bloccante
- [ ] Signal handler per SIGCHLD se necessario
- [ ] Timeout per processi che non rispondono

### ✅ **Best Practices**
- [ ] Gestione errori su tutte le system call
- [ ] Cleanup risorse (pipe, file descriptor)
- [ ] Evita processi zombie
- [ ] Limita numero processi concorrenti
- [ ] Signal handling appropriato

---

## 🎯 Compilazione e Test

```bash
# Compila esempi fork
gcc -o fork_examples fork_examples.c

# Compila esempi exec
gcc -o exec_examples exec_examples.c

# Compila esempi wait
gcc -o wait_examples wait_examples.c

# Compila esempi utility
gcc -o utility_examples utility_examples.c

# Test esempi
./fork_examples
./exec_examples
./wait_examples
./utility_examples

# Monitor processi
ps aux | grep examples

# Test con strace (debugging)
strace -f ./fork_examples

# Test performance
time ./fork_examples

# Controllo memory leaks
valgrind --tool=memcheck ./fork_examples
```

## 🚀 Process Management Patterns

| **Pattern** | **Uso** | **Vantaggi** |
|-------------|---------|--------------|
| **Fork-Exec** | Eseguire programmi esterni | Isolamento, sicurezza |
| **Master-Worker** | Elaborazione parallela | Scalabilità, fault tolerance |
| **Process Pool** | Gestione risorse | Controllo overhead |
| **Pipeline** | Elaborazione a catena | Throughput alto |
| **Daemon** | Servizi background | Persistenza, autonomia |