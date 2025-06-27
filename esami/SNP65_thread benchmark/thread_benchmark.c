#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#define MEASUREMENT_DURATION 5  // secondi per ogni test
#define NUM_THREADS_SCENARIO2 3

typedef struct {
    // Contatori risultati
    long long scenario1_count;
    long long scenario2_count;
    
    // Flag di controllo
    volatile int running_scenario1;
    volatile int running_scenario2;
    volatile int measurement_active;
    
    // Sincronizzazione
    pthread_mutex_t counter_mutex;
    pthread_cond_t start_condition;
    
    // Timing
    struct timespec start_time;
    int duration;
} ExperimentData;

// Struttura per passare dati ai thread worker
typedef struct {
    ExperimentData* data;
    int thread_id;
    int scenario;
} WorkerArgs;

// Variabile globale per l'esperimento
ExperimentData experiment;

// Funzione per ottenere timestamp ad alta risoluzione
double get_timestamp() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

// Genera numero casuale (thread-safe con seed locale)
int generate_random(unsigned int* seed) {
    return rand_r(seed);
}

// Thread worker per eseguire moltiplicazioni
void* worker_thread(void* arg) {
    WorkerArgs* args = (WorkerArgs*)arg;
    ExperimentData* data = args->data;
    int thread_id = args->thread_id;
    int scenario = args->scenario;
    
    // Seed unico per ogni thread
    unsigned int seed = time(NULL) + thread_id;
    
    printf("Worker Thread %d (Scenario %d): avviato\n", thread_id, scenario);
    
    // Aspetta segnale di start
    pthread_mutex_lock(&data->counter_mutex);
    if (scenario == 1) {
        while (!data->running_scenario1) {
            pthread_cond_wait(&data->start_condition, &data->counter_mutex);
        }
    } else {
        while (!data->running_scenario2) {
            pthread_cond_wait(&data->start_condition, &data->counter_mutex);
        }
    }
    pthread_mutex_unlock(&data->counter_mutex);
    
    // Loop di calcolo
    long long local_count = 0;
    while ((scenario == 1 && data->running_scenario1) || 
           (scenario == 2 && data->running_scenario2)) {
        
        // Genera numeri casuali e moltiplica
        int a = generate_random(&seed) % 10000;
        int b = generate_random(&seed) % 10000;
        volatile int result = a * b;  // volatile per evitare ottimizzazioni
        
        local_count++;
        
        // Aggiorna contatore ogni 1000 operazioni per ridurre overhead
        if (local_count % 1000 == 0) {
            pthread_mutex_lock(&data->counter_mutex);
            if (scenario == 1) {
                data->scenario1_count += 1000;
            } else {
                data->scenario2_count += 1000;
            }
            pthread_mutex_unlock(&data->counter_mutex);
            local_count = 0;
        }
    }
    
    // Aggiorna contatore finale
    if (local_count > 0) {
        pthread_mutex_lock(&data->counter_mutex);
        if (scenario == 1) {
            data->scenario1_count += local_count;
        } else {
            data->scenario2_count += local_count;
        }
        pthread_mutex_unlock(&data->counter_mutex);
    }
    
    printf("Worker Thread %d (Scenario %d): terminato\n", thread_id, scenario);
    return NULL;
}

// Thread di misura che controlla i tempi
void* measurement_thread(void* arg) {
    ExperimentData* data = (ExperimentData*)arg;
    
    printf("Thread di Misura: avviato\n");
    
    // SCENARIO 1: Single Thread
    printf("\n=== SCENARIO 1: Single Thread ===\n");
    
    // Reset contatori
    pthread_mutex_lock(&data->counter_mutex);
    data->scenario1_count = 0;
    data->running_scenario1 = 1;
    pthread_mutex_unlock(&data->counter_mutex);
    
    // Segnala start ai worker
    pthread_cond_broadcast(&data->start_condition);
    
    double start_time = get_timestamp();
    
    // Aspetta durata esperimento
    sleep(data->duration);
    
    // Ferma worker
    pthread_mutex_lock(&data->counter_mutex);
    data->running_scenario1 = 0;
    pthread_mutex_unlock(&data->counter_mutex);
    
    double end_time = get_timestamp();
    double scenario1_time = end_time - start_time;
    
    printf("Scenario 1 completato in %.3f secondi\n", scenario1_time);
    printf("Moltiplicazioni eseguite: %lld\n", data->scenario1_count);
    printf("Throughput: %.0f moltiplicazioni/secondo\n", 
           data->scenario1_count / scenario1_time);
    
    // Pausa tra scenari
    sleep(2);
    
    // SCENARIO 2: Multiple Threads
    printf("\n=== SCENARIO 2: %d Threads ===\n", NUM_THREADS_SCENARIO2);
    
    // Reset contatori
    pthread_mutex_lock(&data->counter_mutex);
    data->scenario2_count = 0;
    data->running_scenario2 = 1;
    pthread_mutex_unlock(&data->counter_mutex);
    
    // Segnala start ai worker
    pthread_cond_broadcast(&data->start_condition);
    
    start_time = get_timestamp();
    
    // Aspetta durata esperimento
    sleep(data->duration);
    
    // Ferma worker
    pthread_mutex_lock(&data->counter_mutex);
    data->running_scenario2 = 0;
    pthread_mutex_unlock(&data->counter_mutex);
    
    end_time = get_timestamp();
    double scenario2_time = end_time - start_time;
    
    printf("Scenario 2 completato in %.3f secondi\n", scenario2_time);
    printf("Moltiplicazioni eseguite: %lld\n", data->scenario2_count);
    printf("Throughput: %.0f moltiplicazioni/secondo\n", 
           data->scenario2_count / scenario2_time);
    
    // ANALISI RISULTATI
    printf("\n=== ANALISI RISULTATI ===\n");
    double speedup = (double)data->scenario2_count / data->scenario1_count;
    double efficiency = speedup / NUM_THREADS_SCENARIO2;
    
    printf("Speedup: %.2fx\n", speedup);
    printf("Efficienza: %.2f%% (%.2f/%d threads)\n", 
           efficiency * 100, speedup, NUM_THREADS_SCENARIO2);
    
    if (speedup > 1.1) {
        printf("RISULTATO: Il multithreading porta vantaggi significativi\n");
    } else if (speedup > 0.9) {
        printf("RISULTATO: Il multithreading non porta vantaggi significativi\n");
    } else {
        printf("RISULTATO: Il multithreading peggiora le performance\n");
    }
    
    data->measurement_active = 0;
    return NULL;
}

int main() {
    pthread_t measurement_tid;
    pthread_t worker_tids[NUM_THREADS_SCENARIO2];
    WorkerArgs worker_args[NUM_THREADS_SCENARIO2];
    
    printf("=== ESPERIMENTO PERFORMANCE THREADING ===\n");
    printf("Durata ogni test: %d secondi\n", MEASUREMENT_DURATION);
    printf("Numero thread Scenario 2: %d\n\n", NUM_THREADS_SCENARIO2);
    
    // Inizializza struttura esperimento
    experiment.scenario1_count = 0;
    experiment.scenario2_count = 0;
    experiment.running_scenario1 = 0;
    experiment.running_scenario2 = 0;
    experiment.measurement_active = 1;
    experiment.duration = MEASUREMENT_DURATION;
    
    // Inizializza sincronizzazione
    if (pthread_mutex_init(&experiment.counter_mutex, NULL) != 0) {
        perror("pthread_mutex_init");
        exit(EXIT_FAILURE);
    }
    
    if (pthread_cond_init(&experiment.start_condition, NULL) != 0) {
        perror("pthread_cond_init");
        exit(EXIT_FAILURE);
    }
    
    // Crea thread di misura
    if (pthread_create(&measurement_tid, NULL, measurement_thread, &experiment) != 0) {
        perror("pthread_create measurement");
        exit(EXIT_FAILURE);
    }
    
    // Crea thread worker per Scenario 1 (1 thread)
    worker_args[0].data = &experiment;
    worker_args[0].thread_id = 1;
    worker_args[0].scenario = 1;
    
    if (pthread_create(&worker_tids[0], NULL, worker_thread, &worker_args[0]) != 0) {
        perror("pthread_create worker scenario 1");
        exit(EXIT_FAILURE);
    }
    
    // Aspetta che finisca scenario 1
    pthread_join(worker_tids[0], NULL);
    
    // Crea thread worker per Scenario 2 (3 threads)
    for (int i = 0; i < NUM_THREADS_SCENARIO2; i++) {
        worker_args[i].data = &experiment;
        worker_args[i].thread_id = i + 1;
        worker_args[i].scenario = 2;
        
        if (pthread_create(&worker_tids[i], NULL, worker_thread, &worker_args[i]) != 0) {
            perror("pthread_create worker scenario 2");
            exit(EXIT_FAILURE);
        }
    }
    
    // Aspetta che finiscano tutti i worker scenario 2
    for (int i = 0; i < NUM_THREADS_SCENARIO2; i++) {
        pthread_join(worker_tids[i], NULL);
    }
    
    // Aspetta thread di misura
    pthread_join(measurement_tid, NULL);
    
    // Cleanup
    pthread_mutex_destroy(&experiment.counter_mutex);
    pthread_cond_destroy(&experiment.start_condition);
    
    printf("\nEsperimento completato.\n");
    return 0;
}