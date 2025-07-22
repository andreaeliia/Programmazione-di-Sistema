/*
 * Calcolo Parallelo delle Mantisse con Multi-Threading e Multi-Pipe
 * Implementazione C90 compatibile con Linux e Mac
 * Usa libreria apue.h
 */

#include "apue.h"
#include <pthread.h>
#include <math.h>
#include <sys/time.h>
#include <sys/wait.h>

/* Costanti del programma */
#define MAX_NUMBERS 10000000
#define MAX_THREADS 32
#define MAX_PIPES 16
#define BUFFER_SIZE 1024

/* Struttura per passare dati ai thread */
struct thread_data {
    int thread_id;
    int start_num;
    int end_num;
    int pipe_count;
    int *write_fds;
    pthread_mutex_t *pipe_mutexes;
};

/* Struttura per thread di lettura */
struct reader_data {
    int thread_id;
    int pipe_fd;
    double *result_table;
    int *table_index;
    pthread_mutex_t *table_mutex;
};

/* Struttura per risultato mantissa */
struct mantissa_result {
    int number;
    double mantissa;
};

/* Struttura per configurazione esperimento */
struct experiment_config {
    int n_calc_threads;
    int m_read_threads;
    int p_pipes;
    double execution_time;
    double speedup;
};

/* Variabili globali per sincronizzazione */
static pthread_mutex_t *pipe_mutexes;
static pthread_mutex_t table_mutex = PTHREAD_MUTEX_INITIALIZER;
static int table_index = 0;

/* Funzioni per calcolo mantissa */
double calculate_mantissa(int number);
void* calculator_thread(void *arg);
void* reader_thread(void *arg);

/* Funzioni di supporto */
int create_pipes(int pipe_count, int write_fds[], int read_fds[]);
void close_pipes(int pipe_count, int write_fds[], int read_fds[]);
double get_time_diff(struct timeval start, struct timeval end);

/* Funzioni esperimento */
void run_experiment(int n_threads, int m_threads, int p_pipes, 
                   struct experiment_config *config);
void print_results(struct experiment_config configs[], int config_count);
void find_optimal_configuration(void);

/*
 * Calcola la mantissa di un numero intero
 * La mantissa è la parte frazionaria della rappresentazione in virgola mobile
 */
double calculate_mantissa(int number)
{
    double value;
    int exponent;
    
    if (number == 0) {
        return 0.0;
    }
    
    value = (double)number;
    
    /* Estrae mantissa usando frexp() che restituisce mantissa e esponente */
    return frexp(value, &exponent);
}

/*
 * Thread per calcolare le mantisse
 * Ogni thread calcola mantisse per un range specifico di numeri
 */
void* calculator_thread(void *arg)
{
    struct thread_data *data = (struct thread_data*)arg;
    struct mantissa_result result;
    int i, pipe_index;
    ssize_t bytes_written;
    
    printf("Thread calcolo %d: numeri %d-%d\n", 
           data->thread_id, data->start_num, data->end_num);
    
    for (i = data->start_num; i <= data->end_num; i++) {
        /* Calcola mantissa */
        result.number = i;
        result.mantissa = calculate_mantissa(i);
        
        /* Seleziona pipe usando round-robin */
        pipe_index = i % data->pipe_count;
        
        /* Acquisisce lock per la pipe specifica */
        pthread_mutex_lock(&data->pipe_mutexes[pipe_index]);
        
        /* Scrive risultato nella pipe */
        bytes_written = write(data->write_fds[pipe_index], &result, sizeof(result));
        if (bytes_written != sizeof(result)) {
            err_ret("write error in calculator thread %d", data->thread_id);
        }
        
        /* Rilascia lock */
        pthread_mutex_unlock(&data->pipe_mutexes[pipe_index]);
    }
    
    printf("Thread calcolo %d completato\n", data->thread_id);
    return NULL;
}

/*
 * Thread per leggere dalle pipe e inserire in tabella
 */
void* reader_thread(void *arg)
{
    struct reader_data *data = (struct reader_data*)arg;
    struct mantissa_result result;
    ssize_t bytes_read;
    
    printf("Thread lettura %d avviato\n", data->thread_id);
    
    while (1) {
        /* Legge dalla pipe */
        bytes_read = read(data->pipe_fd, &result, sizeof(result));
        
        if (bytes_read == 0) {
            /* Fine stream - pipe chiusa */
            break;
        } else if (bytes_read != sizeof(result)) {
            if (bytes_read > 0) {
                err_ret("partial read in reader thread %d", data->thread_id);
            }
            break;
        }
        
        /* Acquisisce lock per accesso esclusivo alla tabella */
        pthread_mutex_lock(data->table_mutex);
        
        /* Inserisce risultato nella tabella */
        if (*data->table_index < MAX_NUMBERS) {
            data->result_table[*data->table_index] = result.mantissa;
            (*data->table_index)++;
        }
        
        /* Rilascia lock */
        pthread_mutex_unlock(data->table_mutex);
    }
    
    printf("Thread lettura %d completato\n", data->thread_id);
    return NULL;
}

/*
 * Crea le pipe per comunicazione tra processi
 */
int create_pipes(int pipe_count, int write_fds[], int read_fds[])
{
    int i, pipe_fds[2];
    
    for (i = 0; i < pipe_count; i++) {
        if (pipe(pipe_fds) == -1) {
            err_ret("pipe creation error for pipe %d", i);
            return -1;
        }
        
        read_fds[i] = pipe_fds[0];
        write_fds[i] = pipe_fds[1];
    }
    
    return 0;
}

/*
 * Chiude tutte le pipe
 */
void close_pipes(int pipe_count, int write_fds[], int read_fds[])
{
    int i;
    
    for (i = 0; i < pipe_count; i++) {
        if (write_fds[i] != -1) {
            close(write_fds[i]);
        }
        if (read_fds[i] != -1) {
            close(read_fds[i]);
        }
    }
}

/*
 * Calcola differenza di tempo in secondi
 */
double get_time_diff(struct timeval start, struct timeval end)
{
    return (double)(end.tv_sec - start.tv_sec) + 
           (double)(end.tv_usec - start.tv_usec) / 1000000.0;
}

/*
 * Esegue esperimento con configurazione specifica
 */
void run_experiment(int n_threads, int m_threads, int p_pipes, 
                   struct experiment_config *config)
{
    pid_t child_pid;
    int write_fds[MAX_PIPES], read_fds[MAX_PIPES];
    pthread_t calc_threads[MAX_THREADS], read_threads[MAX_THREADS];
    struct thread_data calc_data[MAX_THREADS];
    struct reader_data read_data[MAX_THREADS];
    struct timeval start_time, end_time;
    double *result_table;
    int i, numbers_per_thread;
    int status;
    
    printf("\n=== ESPERIMENTO: N=%d, M=%d, P=%d ===\n", 
           n_threads, m_threads, p_pipes);
    
    /* Inizializza configurazione */
    config->n_calc_threads = n_threads;
    config->m_read_threads = m_threads;
    config->p_pipes = p_pipes;
    
    /* Crea le pipe */
    if (create_pipes(p_pipes, write_fds, read_fds) == -1) {
        err_sys("pipe creation failed");
    }
    
    /* Alloca memoria per tabella risultati */
    result_table = (double*)malloc(MAX_NUMBERS * sizeof(double));
    if (result_table == NULL) {
        err_sys("malloc failed for result table");
    }
    
    /* Inizializza mutex per le pipe */
    pipe_mutexes = (pthread_mutex_t*)malloc(p_pipes * sizeof(pthread_mutex_t));
    for (i = 0; i < p_pipes; i++) {
        pthread_mutex_init(&pipe_mutexes[i], NULL);
    }
    
    /* Reset indice tabella */
    table_index = 0;
    
    /* Registra tempo di inizio */
    gettimeofday(&start_time, NULL);
    
    /* Fork per creare processo child */
    if ((child_pid = fork()) == -1) {
        err_sys("fork failed");
    } else if (child_pid == 0) {
        /* Processo child - lettore */
        
        /* Chiude lato scrittura delle pipe */
        for (i = 0; i < p_pipes; i++) {
            close(write_fds[i]);
        }
        
        /* Crea thread di lettura */
        for (i = 0; i < m_threads; i++) {
            read_data[i].thread_id = i;
            read_data[i].pipe_fd = read_fds[i % p_pipes];
            read_data[i].result_table = result_table;
            read_data[i].table_index = &table_index;
            read_data[i].table_mutex = &table_mutex;
            
            if (pthread_create(&read_threads[i], NULL, reader_thread, 
                             &read_data[i]) != 0) {
                err_sys("pthread_create failed for reader thread %d", i);
            }
        }
        
        /* Aspetta terminazione thread di lettura */
        for (i = 0; i < m_threads; i++) {
            pthread_join(read_threads[i], NULL);
        }
        
        /* Chiude pipe di lettura */
        for (i = 0; i < p_pipes; i++) {
            close(read_fds[i]);
        }
        
        printf("Processo child: Inseriti %d risultati in tabella\n", table_index);
        
        free(result_table);
        free(pipe_mutexes);
        exit(0);
        
    } else {
        /* Processo parent - calcolatore */
        
        /* Chiude lato lettura delle pipe */
        for (i = 0; i < p_pipes; i++) {
            close(read_fds[i]);
        }
        
        /* Calcola numeri per thread */
        numbers_per_thread = MAX_NUMBERS / n_threads;
        
        /* Crea thread di calcolo */
        for (i = 0; i < n_threads; i++) {
            calc_data[i].thread_id = i;
            calc_data[i].start_num = i * numbers_per_thread + 1;
            calc_data[i].end_num = (i == n_threads - 1) ? 
                                   MAX_NUMBERS : (i + 1) * numbers_per_thread;
            calc_data[i].pipe_count = p_pipes;
            calc_data[i].write_fds = write_fds;
            calc_data[i].pipe_mutexes = pipe_mutexes;
            
            if (pthread_create(&calc_threads[i], NULL, calculator_thread, 
                             &calc_data[i]) != 0) {
                err_sys("pthread_create failed for calculator thread %d", i);
            }
        }
        
        /* Aspetta terminazione thread di calcolo */
        for (i = 0; i < n_threads; i++) {
            pthread_join(calc_threads[i], NULL);
        }
        
        /* Chiude pipe di scrittura per segnalare fine */
        close_pipes(p_pipes, write_fds, read_fds);
        
        /* Aspetta terminazione processo child */
        waitpid(child_pid, &status, 0);
        
        /* Registra tempo di fine */
        gettimeofday(&end_time, NULL);
        
        /* Calcola tempo di esecuzione */
        config->execution_time = get_time_diff(start_time, end_time);
        
        printf("Tempo di esecuzione: %.3f secondi\n", config->execution_time);
    }
    
    /* Pulizia mutex */
    for (i = 0; i < p_pipes; i++) {
        pthread_mutex_destroy(&pipe_mutexes[i]);
    }
    free(pipe_mutexes);
    free(result_table);
}

/*
 * Stampa risultati degli esperimenti
 */
void print_results(struct experiment_config configs[], int config_count)
{
    int i;
    double baseline_time = 0.0;
    
    printf("\n=== RISULTATI ESPERIMENTI ===\n");
    printf("%-4s %-4s %-4s %-10s %-8s\n", "N", "M", "P", "Tempo(s)", "Speedup");
    printf("----------------------------------------\n");
    
    /* Trova tempo baseline (configurazione più semplice) */
    for (i = 0; i < config_count; i++) {
        if (configs[i].n_calc_threads == 1 && 
            configs[i].m_read_threads == 1 && 
            configs[i].p_pipes == 1) {
            baseline_time = configs[i].execution_time;
            break;
        }
    }
    
    /* Se non trovato baseline, usa il primo */
    if (baseline_time == 0.0) {
        baseline_time = configs[0].execution_time;
    }
    
    /* Calcola e stampa speedup */
    for (i = 0; i < config_count; i++) {
        configs[i].speedup = baseline_time / configs[i].execution_time;
        
        printf("%-4d %-4d %-4d %-10.3f %-8.2f\n",
               configs[i].n_calc_threads,
               configs[i].m_read_threads,
               configs[i].p_pipes,
               configs[i].execution_time,
               configs[i].speedup);
    }
}

/*
 * Trova configurazione ottimale attraverso esperimenti
 */
void find_optimal_configuration(void)
{
    struct experiment_config configs[64];
    int config_count = 0;
    int n, m, p;
    int best_config = 0;
    double max_speedup = 0.0;
    
    printf("Avvio sperimentazione per trovare configurazione ottimale...\n");
    
    /* Test con diverse configurazioni */
    for (n = 1; n <= 8; n *= 2) {      /* Thread calcolo: 1, 2, 4, 8 */
        for (m = 1; m <= 4; m *= 2) {  /* Thread lettura: 1, 2, 4 */
            for (p = 1; p <= 4; p *= 2) {  /* Pipe: 1, 2, 4 */
                if (config_count < 64) {
                    run_experiment(n, m, p, &configs[config_count]);
                    
                    /* Aggiorna migliore configurazione */
                    if (configs[config_count].speedup > max_speedup) {
                        max_speedup = configs[config_count].speedup;
                        best_config = config_count;
                    }
                    
                    config_count++;
                }
            }
        }
    }
    
    /* Stampa risultati */
    print_results(configs, config_count);
    
    printf("\n=== CONFIGURAZIONE OTTIMALE ===\n");
    printf("N (thread calcolo): %d\n", configs[best_config].n_calc_threads);
    printf("M (thread lettura): %d\n", configs[best_config].m_read_threads);
    printf("P (pipe): %d\n", configs[best_config].p_pipes);
    printf("Tempo: %.3f secondi\n", configs[best_config].execution_time);
    printf("Speedup: %.2fx\n", configs[best_config].speedup);
}

/*
 * Funzione principale
 */
int main(void)
{
    printf("=== CALCOLO PARALLELO MANTISSE ===\n");
    printf("Range numeri: 1 - %d\n", MAX_NUMBERS);
    printf("Avvio esperimenti per determinare configurazione ottimale...\n");
    
    /* Esegue sperimentazione */
    find_optimal_configuration();
    
    printf("\nEsperimenti completati con successo!\n");
    return 0;
}