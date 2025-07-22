/*
 * SINCRONIZZAZIONE DI PROCESSI CON SEMAFORI POSIX
 * 
 * Argomenti trattati:
 * - Semafori POSIX named
 * - Sincronizzazione tra processi
 * - Fork e gestione processi figli
 * - Memoria condivisa
 * - Signal handling
 * - Grafi di precedenza aciclici
 * - Coordinamento di processi ciclici
 */

#include "apue.h"
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

/* Nomi dei semafori */
#define SEM_P1_P2       "/sem_p1_p2"
#define SEM_P2_P3       "/sem_p2_p3"
#define SEM_P3_BRANCHES "/sem_p3_branches"
#define SEM_P5_P6       "/sem_p5_p6"
#define SEM_P4_P8       "/sem_p4_p8"
#define SEM_P6_P8       "/sem_p6_p8"
#define SEM_P7_P8       "/sem_p7_p8"

/* Struttura per memoria condivisa */
typedef struct {
    int branch_selection;  /* 0=P4+P5/P6, 1=P7+P5/P6, 2=P4+P7 */
    int iteration;         /* Contatore iterazioni */
    int termination_flag;  /* Flag per terminazione controllata */
} shared_data_t;

/* Variabili globali */
static shared_data_t *shared_data = NULL;
static sem_t *sem_p1_p2, *sem_p2_p3, *sem_p3_branches;
static sem_t *sem_p5_p6, *sem_p4_p8, *sem_p6_p8, *sem_p7_p8;
static pid_t child_pids[8];

/* Prototipi delle funzioni */
static void init_semaphores(void);
static void cleanup_semaphores(void);
static void init_shared_memory(void);
static void cleanup_shared_memory(void);
static void signal_handler(int sig);
static void create_processes(void);
static void wait_for_children(void);

/* Funzioni dei processi */
static void processo_P1(void);
static void processo_P2(void);
static void processo_P3(void);
static void processo_P4(void);
static void processo_P5(void);
static void processo_P6(void);
static void processo_P7(void);
static void processo_P8(void);

/*
 * Funzione principale
 */
int main(void)
{
    printf("Avvio sistema di processi coordinati\n");
    
    /* Installazione signal handler per terminazione controllata */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* Inizializzazione risorse */
    init_shared_memory();
    init_semaphores();
    
    /* Creazione processi */
    create_processes();
    
    /* Attesa terminazione processi figli */
    wait_for_children();
    
    /* Cleanup risorse */
    cleanup_semaphores();
    cleanup_shared_memory();
    
    printf("Sistema terminato correttamente\n");
    return 0;
}

/*
 * Inizializzazione memoria condivisa
 */
static void init_shared_memory(void)
{
    int fd;
    
    /* Creazione segmento di memoria condivisa */
    fd = shm_open("/processo_coord_shm", O_CREAT | O_RDWR, 0666);
    if (fd == -1)
        err_sys("shm_open failed");
    
    /* Impostazione dimensione */
    if (ftruncate(fd, sizeof(shared_data_t)) == -1)
        err_sys("ftruncate failed");
    
    /* Mapping in memoria */
    shared_data = mmap(NULL, sizeof(shared_data_t), 
                       PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shared_data == MAP_FAILED)
        err_sys("mmap failed");
    
    close(fd);
    
    /* Inizializzazione dati condivisi */
    shared_data->branch_selection = 0;
    shared_data->iteration = 0;
    shared_data->termination_flag = 0;
    
    printf("Memoria condivisa inizializzata\n");
}

/*
 * Cleanup memoria condivisa
 */
static void cleanup_shared_memory(void)
{
    if (shared_data != NULL) {
        munmap(shared_data, sizeof(shared_data_t));
        shm_unlink("/processo_coord_shm");
    }
}

/*
 * Inizializzazione semafori POSIX
 */
static void init_semaphores(void)
{
    /* Rimozione eventuali semafori preesistenti */
    sem_unlink(SEM_P1_P2);
    sem_unlink(SEM_P2_P3);
    sem_unlink(SEM_P3_BRANCHES);
    sem_unlink(SEM_P5_P6);
    sem_unlink(SEM_P4_P8);
    sem_unlink(SEM_P6_P8);
    sem_unlink(SEM_P7_P8);
    
    /* Creazione semafori */
    sem_p1_p2 = sem_open(SEM_P1_P2, O_CREAT, 0666, 0);
    sem_p2_p3 = sem_open(SEM_P2_P3, O_CREAT, 0666, 0);
    sem_p3_branches = sem_open(SEM_P3_BRANCHES, O_CREAT, 0666, 0);
    sem_p5_p6 = sem_open(SEM_P5_P6, O_CREAT, 0666, 0);
    sem_p4_p8 = sem_open(SEM_P4_P8, O_CREAT, 0666, 0);
    sem_p6_p8 = sem_open(SEM_P6_P8, O_CREAT, 0666, 0);
    sem_p7_p8 = sem_open(SEM_P7_P8, O_CREAT, 0666, 0);
    
    if (sem_p1_p2 == SEM_FAILED || sem_p2_p3 == SEM_FAILED ||
        sem_p3_branches == SEM_FAILED || sem_p5_p6 == SEM_FAILED ||
        sem_p4_p8 == SEM_FAILED || sem_p6_p8 == SEM_FAILED ||
        sem_p7_p8 == SEM_FAILED)
        err_sys("sem_open failed");
    
    printf("Semafori inizializzati (7 totali)\n");
}

/*
 * Cleanup semafori
 */
static void cleanup_semaphores(void)
{
    sem_close(sem_p1_p2);
    sem_close(sem_p2_p3);
    sem_close(sem_p3_branches);
    sem_close(sem_p5_p6);
    sem_close(sem_p4_p8);
    sem_close(sem_p6_p8);
    sem_close(sem_p7_p8);
    
    sem_unlink(SEM_P1_P2);
    sem_unlink(SEM_P2_P3);
    sem_unlink(SEM_P3_BRANCHES);
    sem_unlink(SEM_P5_P6);
    sem_unlink(SEM_P4_P8);
    sem_unlink(SEM_P6_P8);
    sem_unlink(SEM_P7_P8);
}

/*
 * Signal handler per terminazione controllata
 */
static void signal_handler(int sig)
{
    int i;
    
    printf("\nRicevuto segnale %d - Terminazione processi...\n", sig);
    
    /* Imposta flag di terminazione */
    if (shared_data != NULL)
        shared_data->termination_flag = 1;
    
    /* Termina tutti i processi figli */
    for (i = 0; i < 8; i++) {
        if (child_pids[i] > 0) {
            kill(child_pids[i], SIGTERM);
        }
    }
    
    /* Cleanup e uscita */
    cleanup_semaphores();
    cleanup_shared_memory();
    exit(0);
}

/*
 * Creazione dei processi P1-P8
 */
static void create_processes(void)
{
    int i;
    
    /* Inizializzazione array PIDs */
    for (i = 0; i < 8; i++)
        child_pids[i] = 0;
    
    /* Creazione processo P1 */
    if ((child_pids[0] = fork()) == 0) {
        processo_P1();
        exit(0);
    }
    
    /* Creazione processo P2 */
    if ((child_pids[1] = fork()) == 0) {
        processo_P2();
        exit(0);
    }
    
    /* Creazione processo P3 */
    if ((child_pids[2] = fork()) == 0) {
        processo_P3();
        exit(0);
    }
    
    /* Creazione processo P4 */
    if ((child_pids[3] = fork()) == 0) {
        processo_P4();
        exit(0);
    }
    
    /* Creazione processo P5 */
    if ((child_pids[4] = fork()) == 0) {
        processo_P5();
        exit(0);
    }
    
    /* Creazione processo P6 */
    if ((child_pids[5] = fork()) == 0) {
        processo_P6();
        exit(0);
    }
    
    /* Creazione processo P7 */
    if ((child_pids[6] = fork()) == 0) {
        processo_P7();
        exit(0);
    }
    
    /* Creazione processo P8 */
    if ((child_pids[7] = fork()) == 0) {
        processo_P8();
        exit(0);
    }
    
    printf("Tutti i processi creati (P1-P8)\n");
    
    /* Avvio del ciclo - P1 può iniziare */
    sleep(1); /* Attesa inizializzazione processi */
    sem_post(sem_p1_p2);
}

/*
 * Attesa terminazione processi figli
 */
static void wait_for_children(void)
{
    int i, status;
    
    for (i = 0; i < 8; i++) {
        if (child_pids[i] > 0) {
            waitpid(child_pids[i], &status, 0);
        }
    }
}

/*
 * Processo P1 - Primo della catena
 */
static void processo_P1(void)
{
    /* Apertura semafori nel processo figlio */
    sem_p1_p2 = sem_open(SEM_P1_P2, 0);
    if (sem_p1_p2 == SEM_FAILED)
        err_sys("P1: sem_open failed");
    
    while (!shared_data->termination_flag) {
        /* Attesa autorizzazione per iniziare ciclo */
        sem_wait(sem_p1_p2);
        
        if (shared_data->termination_flag) break;
        
        printf("P1: Inizio iterazione %d\n", shared_data->iteration + 1);
        
        /* Simulazione lavoro */
        sleep(1);
        
        printf("P1: Completato - Attivazione P2\n");
        
        /* Segnalazione completamento a P2 */
        sem_post(sem_p2_p3);
    }
    
    sem_close(sem_p1_p2);
}

/*
 * Processo P2 - Secondo della catena
 */
static void processo_P2(void)
{
    sem_p2_p3 = sem_open(SEM_P2_P3, 0);
    if (sem_p2_p3 == SEM_FAILED)
        err_sys("P2: sem_open failed");
    
    while (!shared_data->termination_flag) {
        /* Attesa completamento P1 */
        sem_wait(sem_p2_p3);
        
        if (shared_data->termination_flag) break;
        
        printf("P2: In esecuzione\n");
        
        /* Simulazione lavoro */
        sleep(1);
        
        printf("P2: Completato - Attivazione P3\n");
        
        /* Segnalazione completamento a P3 */
        sem_post(sem_p3_branches);
    }
    
    sem_close(sem_p2_p3);
}

/*
 * Processo P3 - Decisore dei rami
 */
static void processo_P3(void)
{
    sem_p3_branches = sem_open(SEM_P3_BRANCHES, 0);
    sem_p5_p6 = sem_open(SEM_P5_P6, 0);
    sem_p4_p8 = sem_open(SEM_P4_P8, 0);
    sem_p7_p8 = sem_open(SEM_P7_P8, 0);
    
    if (sem_p3_branches == SEM_FAILED || sem_p5_p6 == SEM_FAILED ||
        sem_p4_p8 == SEM_FAILED || sem_p7_p8 == SEM_FAILED)
        err_sys("P3: sem_open failed");
    
    srand(time(NULL));
    
    while (!shared_data->termination_flag) {
        /* Attesa completamento P2 */
        sem_wait(sem_p3_branches);
        
        if (shared_data->termination_flag) break;
        
        printf("P3: In esecuzione\n");
        
        /* Incremento iterazione */
        shared_data->iteration++;
        
        /* Selezione casuale rami (0=P4+P5/P6, 1=P7+P5/P6, 2=P4+P7) */
        shared_data->branch_selection = rand() % 3;
        
        printf("P3: Selezione rami: ");
        switch (shared_data->branch_selection) {
            case 0:
                printf("P4 + P5/P6\n");
                sem_post(sem_p4_p8);    /* Attiva P4 */
                sem_post(sem_p5_p6);    /* Attiva P5 */
                break;
            case 1:
                printf("P7 + P5/P6\n");
                sem_post(sem_p7_p8);    /* Attiva P7 */
                sem_post(sem_p5_p6);    /* Attiva P5 */
                break;
            case 2:
                printf("P4 + P7\n");
                sem_post(sem_p4_p8);    /* Attiva P4 */
                sem_post(sem_p7_p8);    /* Attiva P7 */
                break;
        }
        
        printf("P3: Completato iterazione %d\n", shared_data->iteration);
    }
    
    sem_close(sem_p3_branches);
    sem_close(sem_p5_p6);
    sem_close(sem_p4_p8);
    sem_close(sem_p7_p8);
}

/*
 * Processo P4 - Ramo sinistro
 */
static void processo_P4(void)
{
    sem_p4_p8 = sem_open(SEM_P4_P8, 0);
    if (sem_p4_p8 == SEM_FAILED)
        err_sys("P4: sem_open failed");
    
    while (!shared_data->termination_flag) {
        /* Attesa attivazione da P3 */
        sem_wait(sem_p4_p8);
        
        if (shared_data->termination_flag) break;
        
        printf("P4: In esecuzione (ramo sinistro)\n");
        
        /* Simulazione lavoro */
        sleep(1);
        
        printf("P4: Completato\n");
        
        /* Segnalazione completamento a P8 */
        /* P8 gestirà autonomamente la sincronizzazione */
    }
    
    sem_close(sem_p4_p8);
}

/*
 * Processo P5 - Inizio ramo centrale
 */
static void processo_P5(void)
{
    sem_p5_p6 = sem_open(SEM_P5_P6, 0);
    sem_p6_p8 = sem_open(SEM_P6_P8, 0);
    
    if (sem_p5_p6 == SEM_FAILED || sem_p6_p8 == SEM_FAILED)
        err_sys("P5: sem_open failed");
    
    while (!shared_data->termination_flag) {
        /* Attesa attivazione da P3 */
        sem_wait(sem_p5_p6);
        
        if (shared_data->termination_flag) break;
        
        printf("P5: In esecuzione (ramo centrale)\n");
        
        /* Simulazione lavoro */
        sleep(1);
        
        printf("P5: Completato - Attivazione P6\n");
        
        /* Attivazione P6 */
        sem_post(sem_p6_p8);
    }
    
    sem_close(sem_p5_p6);
    sem_close(sem_p6_p8);
}

/*
 * Processo P6 - Fine ramo centrale
 */
static void processo_P6(void)
{
    sem_p6_p8 = sem_open(SEM_P6_P8, 0);
    if (sem_p6_p8 == SEM_FAILED)
        err_sys("P6: sem_open failed");
    
    while (!shared_data->termination_flag) {
        /* Attesa completamento P5 */
        sem_wait(sem_p6_p8);
        
        if (shared_data->termination_flag) break;
        
        printf("P6: In esecuzione (fine ramo centrale)\n");
        
        /* Simulazione lavoro */
        sleep(1);
        
        printf("P6: Completato\n");
        
        /* Segnalazione completamento a P8 */
        /* P8 gestirà autonomamente la sincronizzazione */
    }
    
    sem_close(sem_p6_p8);
}

/*
 * Processo P7 - Ramo destro
 */
static void processo_P7(void)
{
    sem_p7_p8 = sem_open(SEM_P7_P8, 0);
    if (sem_p7_p8 == SEM_FAILED)
        err_sys("P7: sem_open failed");
    
    while (!shared_data->termination_flag) {
        /* Attesa attivazione da P3 */
        sem_wait(sem_p7_p8);
        
        if (shared_data->termination_flag) break;
        
        printf("P7: In esecuzione (ramo destro)\n");
        
        /* Simulazione lavoro */
        sleep(1);
        
        printf("P7: Completato\n");
        
        /* Segnalazione completamento a P8 */
        /* P8 gestirà autonomamente la sincronizzazione */
    }
    
    sem_close(sem_p7_p8);
}

/*
 * Processo P8 - Sincronizzatore finale e riavvio ciclo
 */
static void processo_P8(void)
{
    sem_p4_p8 = sem_open(SEM_P4_P8, 0);
    sem_p6_p8 = sem_open(SEM_P6_P8, 0);
    sem_p7_p8 = sem_open(SEM_P7_P8, 0);
    sem_p1_p2 = sem_open(SEM_P1_P2, 0);
    
    if (sem_p4_p8 == SEM_FAILED || sem_p6_p8 == SEM_FAILED ||
        sem_p7_p8 == SEM_FAILED || sem_p1_p2 == SEM_FAILED)
        err_sys("P8: sem_open failed");
    
    while (!shared_data->termination_flag) {
        int branch_sel = shared_data->branch_selection;
        
        printf("P8: Attesa completamento rami...\n");
        
        /* Attesa completamento rami in base alla selezione di P3 */
        switch (branch_sel) {
            case 0: /* P4 + P5/P6 */
                /* Non aspetta sui semafori, ma verifica completamento indiretto */
                sleep(3); /* Attesa completamento P4 e P6 */
                break;
            case 1: /* P7 + P5/P6 */
                sleep(3); /* Attesa completamento P7 e P6 */
                break;
            case 2: /* P4 + P7 */
                sleep(3); /* Attesa completamento P4 e P7 */
                break;
        }
        
        if (shared_data->termination_flag) break;
        
        printf("P8: Tutti i rami completati - Fine iterazione %d\n", 
               shared_data->iteration);
        printf("----------------------------------------\n");
        
        /* Pausa tra iterazioni */
        sleep(2);
        
        /* Riavvio ciclo attivando P1 */
        if (!shared_data->termination_flag) {
            sem_post(sem_p1_p2);
        }
    }
    
    sem_close(sem_p4_p8);
    sem_close(sem_p6_p8);
    sem_close(sem_p7_p8);
    sem_close(sem_p1_p2);
}