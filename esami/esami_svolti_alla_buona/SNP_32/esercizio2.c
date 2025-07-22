/*
 * Esercizio 2: Programma che lancia più volte un altro programma per determinare
 * sperimentalmente se la posizione in memoria di una funzione di libreria dinamica
 * varia tra i lanci, calcolando media e deviazione standard.
 * 
 * Autore: Generato automaticamente  
 * Compilazione: make esercizio2 && make target_program
 */

#include "apue.h"
#include <sys/wait.h>
#include <math.h>

#define NUM_LAUNCHES 50
#define MAX_LINE 256
#define TARGET_PROGRAM "./target_program"

/*
 * Struttura per contenere i risultati statistici
 */
struct statistics {
    double mean;
    double std_deviation;
    unsigned long min_addr;
    unsigned long max_addr;
    int valid_samples;
};

/*
 * Funzione per calcolare la media di un array di indirizzi
 */
double calculate_mean(unsigned long addresses[], int count)
{
    double sum = 0.0;
    int i;
    
    for (i = 0; i < count; i++) {
        sum += (double)addresses[i];
    }
    
    return sum / count;
}

/*
 * Funzione per calcolare la deviazione standard
 */
double calculate_std_deviation(unsigned long addresses[], int count, double mean)
{
    double sum_squared_diff = 0.0;
    int i;
    
    for (i = 0; i < count; i++) {
        double diff = (double)addresses[i] - mean;
        sum_squared_diff += diff * diff;
    }
    
    return sqrt(sum_squared_diff / count);
}

/*
 * Funzione per trovare valore minimo e massimo
 */
void find_min_max(unsigned long addresses[], int count, 
                  unsigned long *min, unsigned long *max)
{
    int i;
    *min = addresses[0];
    *max = addresses[0];
    
    for (i = 1; i < count; i++) {
        if (addresses[i] < *min) *min = addresses[i];
        if (addresses[i] > *max) *max = addresses[i];
    }
}

/*
 * Funzione per eseguire il programma target e ottenere l'indirizzo della funzione
 */
unsigned long launch_and_get_address(void)
{
    int pipefd[2];
    pid_t pid;
    unsigned long address = 0;
    char line[MAX_LINE];
    FILE *fp;
    
    /* Crea la pipe */
    if (pipe(pipefd) < 0) {
        err_sys("Errore pipe");
    }
    
    /* Fork per eseguire il programma target */
    if ((pid = fork()) < 0) {
        err_sys("Errore fork");
    } else if (pid == 0) {
        /* Processo figlio: esegue il programma target */
        close(pipefd[0]); /* Chiude lettura */
        
        /* Redirige stdout verso la pipe */
        if (dup2(pipefd[1], STDOUT_FILENO) < 0) {
            err_sys("Errore dup2");
        }
        close(pipefd[1]);
        
        /* Esegue il programma target */
        if (execl(TARGET_PROGRAM, "target_program", (char *)NULL) < 0) {
            err_sys("Errore execl");
        }
    } else {
        /* Processo padre: legge l'output */
        close(pipefd[1]); /* Chiude scrittura */
        
        /* Legge l'indirizzo dalla pipe */
        if ((fp = fdopen(pipefd[0], "r")) != NULL) {
            if (fgets(line, MAX_LINE, fp) != NULL) {
                sscanf(line, "%lx", &address);
            }
            fclose(fp);
        }
        
        close(pipefd[0]);
        
        /* Aspetta la terminazione del figlio */
        waitpid(pid, NULL, 0);
    }
    
    return address;
}

/*
 * Funzione per stampare le statistiche
 */
void print_statistics(struct statistics *stats, unsigned long addresses[], int count)
{
    int i;
    
    printf("\n=== RISULTATI STATISTICI ===\n");
    printf("Numero di lanci: %d\n", count);
    printf("Campioni validi: %d\n", stats->valid_samples);
    printf("Indirizzo minimo: 0x%lx\n", stats->min_addr);
    printf("Indirizzo massimo: 0x%lx\n", stats->max_addr);
    printf("Range: 0x%lx (%lu bytes)\n", 
           stats->max_addr - stats->min_addr, 
           stats->max_addr - stats->min_addr);
    printf("Media: %.2f (0x%lx)\n", stats->mean, (unsigned long)stats->mean);
    printf("Deviazione standard: %.2f\n", stats->std_deviation);
    
    printf("\n=== ANALISI ASLR ===\n");
    if (stats->max_addr != stats->min_addr) {
        printf("ASLR ATTIVO: Gli indirizzi variano tra i lanci\n");
        printf("Variazione osservata: %lu bytes\n", stats->max_addr - stats->min_addr);
    } else {
        printf("ASLR DISATTIVO: Tutti gli indirizzi sono identici\n");
    }
    
    /* Mostra alcuni indirizzi campione */
    printf("\nPrimi 10 indirizzi rilevati:\n");
    for (i = 0; i < 10 && i < count; i++) {
        printf("%2d: 0x%lx\n", i + 1, addresses[i]);
    }
}

/*
 * Funzione principale
 */
int main(void)
{
    unsigned long addresses[NUM_LAUNCHES];
    struct statistics stats;
    int i, valid_count = 0;
    
    printf("=== ESERCIZIO 2: Analisi ASLR delle Librerie Dinamiche ===\n");
    printf("Analisi della posizione della funzione printf() in %d lanci\n\n", NUM_LAUNCHES);
    
    /* Verifica esistenza del programma target */
    if (access(TARGET_PROGRAM, F_OK) < 0) {
        printf("ERRORE: Il programma target '%s' non esiste!\n", TARGET_PROGRAM);
        printf("Compilare prima con: make target_program\n");
        exit(1);
    }
    
    printf("Avvio %d istanze del programma target...\n", NUM_LAUNCHES);
    
    /* Lancia il programma target più volte */
    for (i = 0; i < NUM_LAUNCHES; i++) {
        addresses[i] = launch_and_get_address();
        
        if (addresses[i] != 0) {
            valid_count++;
            if (i % 10 == 0) {
                printf("Lancio %d/%d: 0x%lx\n", i + 1, NUM_LAUNCHES, addresses[i]);
            }
        } else {
            printf("Errore nel lancio %d\n", i + 1);
        }
    }
    
    if (valid_count == 0) {
        printf("ERRORE: Nessun indirizzo valido ottenuto!\n");
        exit(1);
    }
    
    /* Compatta l'array rimuovendo gli zeri */
    int j = 0;
    for (i = 0; i < NUM_LAUNCHES; i++) {
        if (addresses[i] != 0) {
            addresses[j++] = addresses[i];
        }
    }
    
    /* Calcola le statistiche */
    stats.valid_samples = valid_count;
    stats.mean = calculate_mean(addresses, valid_count);
    stats.std_deviation = calculate_std_deviation(addresses, valid_count, stats.mean);
    find_min_max(addresses, valid_count, &stats.min_addr, &stats.max_addr);
    
    /* Stampa i risultati */
    print_statistics(&stats, addresses, valid_count);
    
    return 0;
}