/*
 * profile_fork.c - Profiling prestazioni usando processi fork
 * Lancia 10 processi figli che eseguono funzione dummy
 * Misura tempi clock, system e user per ogni esecuzione e totale
 * Standard C90 compatibile
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#define NUM_ITERATIONS 10
#define DUMMY_WORK_SIZE 1000000

/* Struttura per memorizzare misurazioni temporali */
typedef struct {
    clock_t clock_time;     /* Tempo clock totale */
    double user_time;       /* Tempo CPU user mode */
    double system_time;     /* Tempo CPU system mode */
    double wall_time;       /* Tempo reale trascorso */
} TimeProfile;

/* Funzione dummy che simula lavoro computazionale */
void dummy_function() {
    int i, j;
    volatile double result = 0.0;
    volatile int array[1000];
    
    printf("  Processo figlio PID %d: Inizio esecuzione dummy\n", getpid());
    
    /* Simulazione calcolo intensivo */
    for (i = 0; i < DUMMY_WORK_SIZE; i++) {
        result += (double)i * 3.14159 / (i + 1);
        
        /* Operazioni su array per simulare accesso memoria */
        if (i % 1000 == 0) {
            for (j = 0; j < 1000; j++) {
                array[j] = i + j;
            }
        }
        
        /* Simulazione I/O occasionale */
        if (i % 100000 == 0) {
            /* Operazione che simula accesso a risorse */
            getpid(); /* System call leggera */
        }
    }
    
    printf("  Processo figlio PID %d: Dummy completato (risultato: %.2f)\n", 
           getpid(), result);
}

/* Converte clock_t in secondi */
double clock_to_seconds(clock_t clocks) {
    return (double)clocks / CLOCKS_PER_SEC;
}

/* Misura tempi di esecuzione per un singolo fork */
TimeProfile measure_single_fork(int iteration) {
    pid_t pid;
    int status;
    clock_t start_clock, end_clock;
    struct tms start_times, end_times;
    struct rusage usage;
    struct timespec start_wall, end_wall;
    TimeProfile profile = {0};
    
    printf("\n--- Iterazione %d ---\n", iteration + 1);
    
    /* Misura tempo reale inizio */
    clock_gettime(CLOCK_REALTIME, &start_wall);
    
    /* Misura tempi CPU inizio */
    start_clock = clock();
    times(&start_times);
    
    /* Fork del processo */
    pid = fork();
    
    if (pid == -1) {
        perror("fork fallito");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        /* Processo figlio */
        dummy_function();
        exit(EXIT_SUCCESS);
    }
    else {
        /* Processo padre */
        /* Attende terminazione figlio */
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid fallito");
            exit(EXIT_FAILURE);
        }
        
        /* Misura tempi fine */
        end_clock = clock();
        times(&end_times);
        clock_gettime(CLOCK_REALTIME, &end_wall);
        
        /* Ottiene statistiche risorse del processo terminato */
        if (getrusage(RUSAGE_CHILDREN, &usage) == -1) {
            perror("getrusage fallito");
        }
        
        /* Calcola differenze temporali */
        profile.clock_time = end_clock - start_clock;
        profile.wall_time = (end_wall.tv_sec - start_wall.tv_sec) + 
                           (end_wall.tv_nsec - start_wall.tv_nsec) / 1000000000.0;
        
        /* Tempi CPU del processo figlio */
        profile.user_time = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1000000.0;
        profile.system_time = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1000000.0;
        
        /* Verifica stato uscita */
        if (WIFEXITED(status)) {
            printf("  Processo figlio terminato con codice %d\n", WEXITSTATUS(status));
        } else {
            printf("  Processo figlio terminato in modo anomalo\n");
        }
        
        /* Stampa risultati iterazione */
        printf("  Tempi iterazione %d:\n", iteration + 1);
        printf("    Clock time:  %.4f s\n", clock_to_seconds(profile.clock_time));
        printf("    Wall time:   %.4f s\n", profile.wall_time);
        printf("    User time:   %.4f s\n", profile.user_time);
        printf("    System time: %.4f s\n", profile.system_time);
    }
    
    return profile;
}

/* Stampa statistiche riassuntive */
void print_summary_statistics(TimeProfile profiles[], int count) {
    TimeProfile total = {0};
    TimeProfile avg = {0};
    TimeProfile min = {0x7FFFFFFF, 1e9, 1e9, 1e9};
    TimeProfile max = {0};
    int i;
    
    printf("\n=== ANALISI STATISTICA ===\n");
    
    /* Calcola totali, minimi e massimi */
    for (i = 0; i < count; i++) {
        /* Totali */
        total.clock_time += profiles[i].clock_time;
        total.wall_time += profiles[i].wall_time;
        total.user_time += profiles[i].user_time;
        total.system_time += profiles[i].system_time;
        
        /* Minimi */
        if (profiles[i].clock_time < min.clock_time) min.clock_time = profiles[i].clock_time;
        if (profiles[i].wall_time < min.wall_time) min.wall_time = profiles[i].wall_time;
        if (profiles[i].user_time < min.user_time) min.user_time = profiles[i].user_time;
        if (profiles[i].system_time < min.system_time) min.system_time = profiles[i].system_time;
        
        /* Massimi */
        if (profiles[i].clock_time > max.clock_time) max.clock_time = profiles[i].clock_time;
        if (profiles[i].wall_time > max.wall_time) max.wall_time = profiles[i].wall_time;
        if (profiles[i].user_time > max.user_time) max.user_time = profiles[i].user_time;
        if (profiles[i].system_time > max.system_time) max.system_time = profiles[i].system_time;
    }
    
    /* Calcola medie */
    avg.clock_time = total.clock_time / count;
    avg.wall_time = total.wall_time / count;
    avg.user_time = total.user_time / count;
    avg.system_time = total.system_time / count;
    
    /* Stampa tabella riassuntiva */
    printf("┌─────────────┬────────┬────────┬────────┬────────┐\n");
    printf("│ Metrica     │ Totale │ Media  │ Min    │ Max    │\n");
    printf("├─────────────┼────────┼────────┼────────┼────────┤\n");
    printf("│ Clock time  │ %6.3f │ %6.3f │ %6.3f │ %6.3f │\n",
           clock_to_seconds(total.clock_time), clock_to_seconds(avg.clock_time),
           clock_to_seconds(min.clock_time), clock_to_seconds(max.clock_time));
    printf("│ Wall time   │ %6.3f │ %6.3f │ %6.3f │ %6.3f │\n",
           total.wall_time, avg.wall_time, min.wall_time, max.wall_time);
    printf("│ User time   │ %6.3f │ %6.3f │ %6.3f │ %6.3f │\n",
           total.user_time, avg.user_time, min.user_time, max.user_time);
    printf("│ System time │ %6.3f │ %6.3f │ %6.3f │ %6.3f │\n",
           total.system_time, avg.system_time, min.system_time, max.system_time);
    printf("└─────────────┴────────┴────────┴────────┴────────┘\n");
    
    /* Calcola overhead fork */
    printf("\nANALISI OVERHEAD:\n");
    printf("CPU totale utilizzata: %.3f s (User: %.3f + System: %.3f)\n",
           total.user_time + total.system_time, total.user_time, total.system_time);
    printf("Tempo reale totale: %.3f s\n", total.wall_time);
    printf("Overhead fork/wait: %.3f s (%.1f%%)\n", 
           total.wall_time - (total.user_time + total.system_time),
           ((total.wall_time - (total.user_time + total.system_time)) / total.wall_time) * 100);
}

/* Funzione principale */
int main() {
    TimeProfile profiles[NUM_ITERATIONS];
    clock_t total_start_clock, total_end_clock;
    struct timespec total_start_wall, total_end_wall;
    int i;
    
    printf("=== PROFILING PRESTAZIONI CON FORK ===\n");
    printf("Esecuzioni programmate: %d\n", NUM_ITERATIONS);
    printf("Dimensione lavoro dummy: %d iterazioni\n", DUMMY_WORK_SIZE);
    printf("PID processo principale: %d\n\n", getpid());
    
    /* Misura tempo totale inizio */
    total_start_clock = clock();
    clock_gettime(CLOCK_REALTIME, &total_start_wall);
    
    /* Esegue tutte le iterazioni */
    for (i = 0; i < NUM_ITERATIONS; i++) {
        profiles[i] = measure_single_fork(i);
    }
    
    /* Misura tempo totale fine */
    total_end_clock = clock();
    clock_gettime(CLOCK_REALTIME, &total_end_wall);
    
    /* Stampa statistiche finali */
    print_summary_statistics(profiles, NUM_ITERATIONS);
    
    /* Tempo totale del programma */
    printf("\nTEMPO TOTALE PROGRAMMA:\n");
    printf("Clock time totale: %.3f s\n", 
           clock_to_seconds(total_end_clock - total_start_clock));
    printf("Wall time totale:  %.3f s\n",
           (total_end_wall.tv_sec - total_start_wall.tv_sec) + 
           (total_end_wall.tv_nsec - total_start_wall.tv_nsec) / 1000000000.0);
    
    printf("\nProfiling con fork completato.\n");
    return EXIT_SUCCESS;
}