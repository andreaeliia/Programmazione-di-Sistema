#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

#define RUN_COMMAND 10

// Funzione per calcolare la media
double media(double *valori, int size) {
    double totale = 0.0;  // ← Corretto: double invece di int
    for (int i = 0; i < size; i++) {
        totale += valori[i];
    }
    return totale / size;
}

// Funzione per convertire timespec in secondi
double timespec_to_seconds(struct timespec *ts) {
    return ts->tv_sec + ts->tv_nsec / 1000000000.0;
}

int main(int argc, char* argv[]) {
    // Controllo argomenti
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <comando>\n", argv[0]);
        return 1;
    }

    char *comando = argv[1];
    
    // Array per le misurazioni
    double misurazioni_wall[RUN_COMMAND];
    double misurazioni_user[RUN_COMMAND];
    double misurazioni_sys[RUN_COMMAND];
    
    // Variabili per calcolare le differenze
    struct rusage prev_usage = {0};
    double prev_user = 0.0, prev_sys = 0.0;

    // Loop per 10 esecuzioni
    for (int i = 0; i < RUN_COMMAND; i++) {
        struct timespec start_wall, end_wall;
        struct rusage usage;
        
        // Tempo wall di inizio
        clock_gettime(CLOCK_REALTIME, &start_wall);
        
        // Esegui il comando con fork + exec per misurare correttamente
        pid_t pid = fork();
        
        if (pid == 0) {
            // Processo figlio: esegui il comando
            execl("/bin/sh", "sh", "-c", comando, (char*)NULL);
            perror("exec");
            exit(1);
        } else if (pid > 0) {
            // Processo padre: aspetta e misura
            wait(NULL);
            
            // Tempo wall di fine
            clock_gettime(CLOCK_REALTIME, &end_wall);
            
            // Ottieni statistiche CUMULATIVE del figlio
            getrusage(RUSAGE_CHILDREN, &usage);
            
            // Calcola i tempi
            double wall_time = timespec_to_seconds(&end_wall) - timespec_to_seconds(&start_wall);
            
            // Calcola tempi correnti sottraendo i precedenti
            double total_user = usage.ru_utime.tv_sec + usage.ru_utime.tv_usec / 1000000.0;
            double total_sys = usage.ru_stime.tv_sec + usage.ru_stime.tv_usec / 1000000.0;
            
            double user_time = total_user - prev_user;
            double sys_time = total_sys - prev_sys;
            
            // Aggiorna i valori precedenti per la prossima iterazione
            prev_user = total_user;
            prev_sys = total_sys;
            
            // Salva le misurazioni
            misurazioni_wall[i] = wall_time;
            misurazioni_user[i] = user_time;
            misurazioni_sys[i] = sys_time;
            
            // Debug: mostra ogni misurazione (opzionale)
            printf("Esecuzione %d - Wall: %.6f, User: %.6f, Sys: %.6f\n", 
                   i+1, wall_time, user_time, sys_time);
                   
        } else {
            perror("fork");
            return 1;
        }
    }
    
    // Calcola le medie DOPO il loop
    double media_wall = media(misurazioni_wall, RUN_COMMAND);
    double media_user = media(misurazioni_user, RUN_COMMAND);
    double media_sys = media(misurazioni_sys, RUN_COMMAND);   // ← Corretto!
    
    // Output finale su stderr (come richiesto)
    fprintf(stderr, "Medie su %d esecuzioni:\n", RUN_COMMAND);
    fprintf(stderr, "Wall time: %.6f secondi\n", media_wall);
    fprintf(stderr, "User time: %.6f secondi\n", media_user);
    fprintf(stderr, "System time: %.6f secondi\n", media_sys);
    
    return 0;
}