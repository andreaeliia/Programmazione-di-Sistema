/*
 * Controllo Lampadine con Semafori System V
 * Implementazione C90 compatibile con Linux e Mac
 * Usa libreria apue.h
 */

#include "apue.h"
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>

/* Definizioni globali */
#define SEM_KEY 1234
#define RED_LAMP 0
#define GREEN_LAMP 1
#define MUTEX_SEM 2
#define NUM_SEMS 3

/* Struttura per operazioni sui semafori */
struct sembuf sem_op;

/* ID del semaforo globale */
static int semid = -1;

/* Funzioni per gestione semafori */
int init_semaphores(void);
void cleanup_semaphores(void);
int sem_wait(int sem_num);
int sem_signal(int sem_num);

/* API per controllo lampadine */
int on_red(void);
int off_red(void);
int on_green(void);
int off_green(void);

/* Processi */
void process_a(void);    /* Server che gestisce le lampadine */
void process_b(void);    /* Client per lampadina rossa */
void process_c(void);    /* Client per lampadina verde */

/* Gestore segnale per pulizia */
void signal_handler(int sig);

/*
 * Inizializza i semafori System V
 * Ritorna 0 se successo, -1 se errore
 */
int init_semaphores(void)
{
    int i;
    
    /* Crea il set di semafori */
    if ((semid = semget(SEM_KEY, NUM_SEMS, IPC_CREAT | 0666)) == -1) {
        err_sys("semget error");
        return -1;
    }
    
    /* Inizializza tutti i semafori a 0 (lampadine spente) */
    for (i = 0; i < NUM_SEMS - 1; i++) {
        if (semctl(semid, i, SETVAL, 0) == -1) {
            err_sys("semctl SETVAL error");
            return -1;
        }
    }
    
    /* Inizializza il mutex a 1 (risorsa disponibile) */
    if (semctl(semid, MUTEX_SEM, SETVAL, 1) == -1) {
        err_sys("semctl SETVAL error");
        return -1;
    }
    
    return 0;
}

/*
 * Pulisce i semafori System V
 */
void cleanup_semaphores(void)
{
    if (semid != -1) {
        if (semctl(semid, 0, IPC_RMID) == -1) {
            err_ret("semctl IPC_RMID error");
        }
    }
}

/*
 * Operazione wait (P) su semaforo
 */
int sem_wait(int sem_num)
{
    sem_op.sem_num = sem_num;
    sem_op.sem_op = -1;
    sem_op.sem_flg = 0;
    
    if (semop(semid, &sem_op, 1) == -1) {
        err_ret("semop wait error");
        return -1;
    }
    return 0;
}

/*
 * Operazione signal (V) su semaforo
 */
int sem_signal(int sem_num)
{
    sem_op.sem_num = sem_num;
    sem_op.sem_op = 1;
    sem_op.sem_flg = 0;
    
    if (semop(semid, &sem_op, 1) == -1) {
        err_ret("semop signal error");
        return -1;
    }
    return 0;
}

/*
 * API: Accende lampadina rossa
 */
int on_red(void)
{
    printf("LAMPADINA ROSSA: ACCESA\n");
    fflush(stdout);
    return sem_signal(RED_LAMP);
}

/*
 * API: Spegne lampadina rossa
 */
int off_red(void)
{
    printf("LAMPADINA ROSSA: SPENTA\n");
    fflush(stdout);
    return 0;
}

/*
 * API: Accende lampadina verde
 */
int on_green(void)
{
    printf("LAMPADINA VERDE: ACCESA\n");
    fflush(stdout);
    return sem_signal(GREEN_LAMP);
}

/*
 * API: Spegne lampadina verde
 */
int off_green(void)
{
    printf("LAMPADINA VERDE: SPENTA\n");
    fflush(stdout);
    return 0;
}

/*
 * Processo A: Server che gestisce le richieste
 */
void process_a(void)
{
    int red_requests = 0, green_requests = 0;
    
    printf("Processo A: Server lampadine avviato\n");
    
    while (red_requests < 5 || green_requests < 5) {
        /* Acquisisce accesso esclusivo */
        if (sem_wait(MUTEX_SEM) == -1) continue;
        
        /* Controlla richieste per lampadina rossa */
        if (semctl(semid, RED_LAMP, GETVAL) > 0 && red_requests < 5) {
            sem_wait(RED_LAMP);  /* Consuma richiesta */
            on_red();
            sleep(1);  /* Lampadina accesa per 1 secondo */
            off_red();
            red_requests++;
        }
        /* Controlla richieste per lampadina verde */
        else if (semctl(semid, GREEN_LAMP, GETVAL) > 0 && green_requests < 5) {
            sem_wait(GREEN_LAMP);  /* Consuma richiesta */
            on_green();
            sleep(1);  /* Lampadina accesa per 1 secondo */
            off_green();
            green_requests++;
        }
        
        /* Rilascia accesso esclusivo */
        sem_signal(MUTEX_SEM);
        
        usleep(10000);  /* Piccola pausa per permettere context switch */
    }
    
    printf("Processo A: Completato (R:%d, V:%d)\n", red_requests, green_requests);
}

/*
 * Processo B: Richiede lampadina rossa
 */
void process_b(void)
{
    int i;
    
    printf("Processo B: Client lampadina rossa avviato\n");
    
    for (i = 0; i < 5; i++) {
        printf("Processo B: Richiesta %d lampadina rossa\n", i + 1);
        sem_signal(RED_LAMP);  /* Invia richiesta */
        usleep(50000);  /* Pausa tra richieste */
    }
    
    printf("Processo B: Completato\n");
}

/*
 * Processo C: Richiede lampadina verde
 */
void process_c(void)
{
    int i;
    
    printf("Processo C: Client lampadina verde avviato\n");
    
    for (i = 0; i < 5; i++) {
        printf("Processo C: Richiesta %d lampadina verde\n", i + 1);
        sem_signal(GREEN_LAMP);  /* Invia richiesta */
        usleep(75000);  /* Pausa diversa per simulare concorrenza */
    }
    
    printf("Processo C: Completato\n");
}

/*
 * Gestore per segnali di terminazione
 */
void signal_handler(int sig)
{
    printf("\nRicevuto segnale %d, pulizia in corso...\n", sig);
    cleanup_semaphores();
    exit(0);
}

/*
 * Funzione principale
 */
int main(void)
{
    pid_t pid_a, pid_b, pid_c;
    int status;
    time_t start_time, end_time;
    
    /* Installa gestore segnali */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("=== CONTROLLO LAMPADINE - VERSIONE SEMAFORI ===\n");
    
    /* Inizializza semafori */
    if (init_semaphores() == -1) {
        err_sys("Errore inizializzazione semafori");
    }
    
    printf("Semafori inizializzati correttamente\n");
    
    /* Registra tempo di inizio */
    time(&start_time);
    
    /* Crea processo A (server) */
    if ((pid_a = fork()) == -1) {
        err_sys("fork error processo A");
    } else if (pid_a == 0) {
        process_a();
        exit(0);
    }
    
    /* Crea processo B (client rosso) */
    if ((pid_b = fork()) == -1) {
        err_sys("fork error processo B");
    } else if (pid_b == 0) {
        process_b();
        exit(0);
    }
    
    /* Crea processo C (client verde) */
    if ((pid_c = fork()) == -1) {
        err_sys("fork error processo C");
    } else if (pid_c == 0) {
        process_c();
        exit(0);
    }
    
    /* Aspetta terminazione di tutti i processi */
    waitpid(pid_a, &status, 0);
    waitpid(pid_b, &status, 0);
    waitpid(pid_c, &status, 0);
    
    /* Calcola tempo totale */
    time(&end_time);
    printf("\n=== STATISTICHE ===\n");
    printf("Tempo totale: %ld secondi\n", (long)(end_time - start_time));
    printf("Meccanismo: Semafori System V\n");
    
    /* Pulizia finale */
    cleanup_semaphores();
    
    printf("Programma terminato correttamente\n");
    return 0;
}