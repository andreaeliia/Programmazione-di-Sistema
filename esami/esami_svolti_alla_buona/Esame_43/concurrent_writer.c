#include "apue.h"
#include <sys/shm.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>

#define SHM_KEY 0x2345
#define LOCK_FILE "/tmp/record_lock.tmp"
#define MAX_RECORDS 1000
#define WRITE_INTERVAL 3
#define NUM_PROCESSES 3

/* Struttura per un record nella memoria condivisa */
struct shared_record {
    pid_t pid;
    int random_number;
    time_t timestamp;
};

/* Struttura header della memoria condivisa */
struct shared_memory {
    int record_count;
    struct shared_record records[MAX_RECORDS];
};

/* Variabili globali per la gestione dei segnali */
static volatile sig_atomic_t should_terminate = 0;
static int shmid = -1;
static int lock_fd = -1;
static pid_t child_pids[NUM_PROCESSES];

/* Prototipi delle funzioni */
static void signal_handler(int signo);
static void setup_signals(void);
static int create_shared_memory(void);
static int create_lock_file(void);
static void acquire_record_lock(int fd);
static void release_record_lock(int fd);
static void child_process_main(char process_id);
static void add_record_to_shared_memory(void *shm_ptr, pid_t pid, int random_num, time_t timestamp);
static void print_shared_memory_contents(void *shm_ptr);
static void cleanup_resources(void);
static void wait_for_children(void);

/*
 * Gestore del segnale SIGINT
 */
static void
signal_handler(int signo)
{
    int i;
    
    if (signo == SIGINT) {
        should_terminate = 1;
        
        /* Termina tutti i processi figli */
        for (i = 0; i < NUM_PROCESSES; i++) {
            if (child_pids[i] > 0) {
                kill(child_pids[i], SIGTERM);
            }
        }
    }
}

/*
 * Configura i gestori dei segnali
 */
static void
setup_signals(void)
{
    struct sigaction sa;
    
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    if (sigaction(SIGINT, &sa, NULL) < 0) {
        err_sys("sigaction error for SIGINT");
    }
}

/*
 * Crea e inizializza la memoria condivisa
 */
static int
create_shared_memory(void)
{
    int id;
    void *ptr;
    struct shared_memory *shm;
    
    /* Crea la memoria condivisa */
    if ((id = shmget(SHM_KEY, sizeof(struct shared_memory), IPC_CREAT | IPC_EXCL | 0666)) < 0) {
        if (errno == EEXIST) {
            /* Rimuove la memoria condivisa esistente */
            if ((id = shmget(SHM_KEY, 0, 0)) >= 0) {
                shmctl(id, IPC_RMID, NULL);
            }
            /* Riprova a crearla */
            if ((id = shmget(SHM_KEY, sizeof(struct shared_memory), IPC_CREAT | IPC_EXCL | 0666)) < 0) {
                err_sys("shmget error on retry");
            }
        } else {
            err_sys("shmget error");
        }
    }
    
    /* Collega la memoria condivisa per l'inizializzazione */
    if ((ptr = shmat(id, 0, 0)) == (void *)-1) {
        err_sys("shmat error for initialization");
    }
    
    /* Inizializza il contatore dei record */
    shm = (struct shared_memory *)ptr;
    shm->record_count = 0;
    
    /* Scollega la memoria condivisa */
    if (shmdt(ptr) < 0) {
        err_sys("shmdt error after initialization");
    }
    
    return id;
}

/*
 * Crea il file per il record locking
 */
static int
create_lock_file(void)
{
    int fd;
    
    /* Rimuove il file se esiste */
    unlink(LOCK_FILE);
    
    /* Crea il file di lock */
    if ((fd = creat(LOCK_FILE, 0666)) < 0) {
        err_sys("creat error for lock file");
    }
    
    close(fd);
    
    /* Riapre il file in lettura/scrittura */
    if ((fd = open(LOCK_FILE, O_RDWR)) < 0) {
        err_sys("open error for lock file");
    }
    
    return fd;
}

/*
 * Acquisisce il lock esclusivo sul file
 */
static void
acquire_record_lock(int fd)
{
    struct flock fl;
    
    fl.l_type = F_WRLCK;    /* Lock esclusivo */
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 1;           /* Lock sul primo byte */
    
    if (fcntl(fd, F_SETLKW, &fl) < 0) {
        if (errno != EINTR) {
            err_sys("fcntl F_SETLKW error");
        }
    }
}

/*
 * Rilascia il lock sul file
 */
static void
release_record_lock(int fd)
{
    struct flock fl;
    
    fl.l_type = F_UNLCK;    /* Unlock */
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 1;
    
    if (fcntl(fd, F_SETLK, &fl) < 0) {
        err_sys("fcntl F_SETLK unlock error");
    }
}

/*
 * Aggiunge un record alla memoria condivisa (con sincronizzazione)
 */
static void
add_record_to_shared_memory(void *shm_ptr, pid_t pid, int random_num, time_t timestamp)
{
    struct shared_memory *shm = (struct shared_memory *)shm_ptr;
    
    if (shm->record_count < MAX_RECORDS) {
        shm->records[shm->record_count].pid = pid;
        shm->records[shm->record_count].random_number = random_num;
        shm->records[shm->record_count].timestamp = timestamp;
        shm->record_count++;
    }
}

/*
 * Funzione principale del processo figlio
 */
static void
child_process_main(char process_id)
{
    void *shm_ptr;
    int local_lock_fd;
    pid_t my_pid;
    int random_num;
    time_t current_time;
    
    my_pid = getpid();
    
    /* Apre il file di lock */
    if ((local_lock_fd = open(LOCK_FILE, O_RDWR)) < 0) {
        err_sys("child: open error for lock file");
    }
    
    /* Collega la memoria condivisa */
    if ((shm_ptr = shmat(shmid, 0, 0)) == (void *)-1) {
        err_sys("child: shmat error");
    }
    
    /* Inizializza il generatore di numeri casuali */
    srand((unsigned int)(my_pid + time(NULL)));
    
    printf("Processo %c (PID: %d) avviato\n", process_id, my_pid);
    
    /* Loop principale del processo figlio */
    while (!should_terminate) {
        /* Genera numero casuale */
        random_num = rand() % 1000;
        current_time = time(NULL);
        
        /* Acquisisce il lock per la scrittura critica */
        acquire_record_lock(local_lock_fd);
        
        /* Aggiunge il record alla memoria condivisa */
        add_record_to_shared_memory(shm_ptr, my_pid, random_num, current_time);
        
        printf("Processo %c: scritto record (PID=%d, NUM=%d, TIME=%ld)\n", 
               process_id, my_pid, random_num, (long)current_time);
        
        /* Rilascia il lock */
        release_record_lock(local_lock_fd);
        
        /* Attende per l'intervallo specificato */
        sleep(WRITE_INTERVAL);
    }
    
    /* Pulizia del processo figlio */
    if (shmdt(shm_ptr) < 0) {
        err_ret("child: shmdt error");
    }
    
    close(local_lock_fd);
    printf("Processo %c (PID: %d) terminato\n", process_id, my_pid);
    exit(0);
}

/*
 * Stampa il contenuto della memoria condivisa
 */
static void
print_shared_memory_contents(void *shm_ptr)
{
    struct shared_memory *shm = (struct shared_memory *)shm_ptr;
    int i;
    char time_str[26];
    
    printf("\n=== CONTENUTO MEMORIA CONDIVISA ===\n");
    printf("Numero totale di record: %d\n\n", shm->record_count);
    
    if (shm->record_count == 0) {
        printf("Nessun record presente.\n");
        return;
    }
    
    printf("%-6s %-8s %-12s %s\n", "Indice", "PID", "Numero", "Timestamp");
    printf("%-6s %-8s %-12s %s\n", "------", "---", "------", "---------");
    
    for (i = 0; i < shm->record_count; i++) {
        /* Converte il timestamp in stringa leggibile */
        strcpy(time_str, ctime(&shm->records[i].timestamp));
        /* Rimuove il carattere newline */
        if (time_str[strlen(time_str) - 1] == '\n') {
            time_str[strlen(time_str) - 1] = '\0';
        }
        
        printf("%-6d %-8d %-12d %s\n", 
               i + 1,
               (int)shm->records[i].pid,
               shm->records[i].random_number,
               time_str);
    }
    printf("\n");
}

/*
 * Attende la terminazione di tutti i processi figli
 */
static void
wait_for_children(void)
{
    int i;
    int status;
    pid_t wpid;
    
    /* Attende tutti i processi figli */
    for (i = 0; i < NUM_PROCESSES; i++) {
        if (child_pids[i] > 0) {
            if ((wpid = waitpid(child_pids[i], &status, 0)) > 0) {
                printf("Processo figlio %d terminato\n", wpid);
            }
        }
    }
}

/*
 * Pulisce tutte le risorse utilizzate
 */
static void
cleanup_resources(void)
{
    /* Rimuove il file di lock */
    if (lock_fd >= 0) {
        close(lock_fd);
        unlink(LOCK_FILE);
    }
    
    /* Rimuove la memoria condivisa */
    if (shmid >= 0) {
        if (shmctl(shmid, IPC_RMID, NULL) < 0) {
            err_ret("shmctl IPC_RMID error");
        }
    }
}

int
main(void)
{
    int i;
    void *shm_ptr;
    char process_names[] = {'A', 'B', 'C'};
    
    printf("=== CONCURRENT PROCESSES CON RECORD LOCKING ===\n");
    printf("Avvio di %d processi concorrenti...\n", NUM_PROCESSES);
    printf("Premere Ctrl-C per terminare e vedere i risultati.\n\n");
    
    /* Inizializza l'array dei PID dei figli */
    for (i = 0; i < NUM_PROCESSES; i++) {
        child_pids[i] = -1;
    }
    
    /* Configura i gestori dei segnali */
    setup_signals();
    
    /* Crea la memoria condivisa */
    shmid = create_shared_memory();
    
    /* Crea il file per il record locking */
    lock_fd = create_lock_file();
    
    /* Crea i processi figli */
    for (i = 0; i < NUM_PROCESSES; i++) {
        if ((child_pids[i] = fork()) < 0) {
            err_sys("fork error");
        } else if (child_pids[i] == 0) {
            /* Processo figlio */
            child_process_main(process_names[i]);
            /* Non dovrebbe mai arrivare qui */
            exit(0);
        }
    }
    
    /* Processo padre: attende il segnale di terminazione */
    printf("Processo padre in attesa... (PID: %d)\n", getpid());
    
    while (!should_terminate) {
        pause(); /* Attende i segnali */
    }
    
    printf("\nSegnale di terminazione ricevuto. Terminazione in corso...\n");
    
    /* Attende la terminazione di tutti i figli */
    wait_for_children();
    
    /* Collega la memoria condivisa per stampare i risultati */
    if ((shm_ptr = shmat(shmid, 0, SHM_RDONLY)) == (void *)-1) {
        err_sys("shmat error for final read");
    }
    
    /* Stampa il contenuto della memoria condivisa */
    print_shared_memory_contents(shm_ptr);
    
    /* Scollega la memoria condivisa */
    if (shmdt(shm_ptr) < 0) {
        err_ret("shmdt error");
    }
    
    /* Pulizia delle risorse */
    cleanup_resources();
    
    printf("Programma terminato con successo.\n");
    exit(0);
}