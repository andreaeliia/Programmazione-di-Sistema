/*
 * Simulazione Incrocio Stradale a 4 Vie con Semafori POSIX
 * Implementazione C90 compatibile con Linux e Mac
 * Usa libreria apue.h
 */

#include "apue.h"
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

/* Costanti del sistema */
#define NUM_LANES 4
#define MAX_VEHICLES_PER_LANE 20
#define GREEN_LIGHT_DURATION 5
#define YELLOW_LIGHT_DURATION 1
#define VEHICLE_GENERATION_INTERVAL 2
#define TRANSIT_TIME 1
#define SIM_DURATION 30

/* Nomi delle vie */
#define NORTH 0
#define SOUTH 1
#define EAST 2
#define WEST 3

/* Stati del semaforo */
typedef enum {
    RED_LIGHT,
    YELLOW_LIGHT,
    GREEN_LIGHT
} light_state_t;

/* Struttura per un veicolo */
struct vehicle {
    int id;
    time_t arrival_time;
    int lane;
};

/* Struttura per una coda di veicoli */
struct vehicle_queue {
    struct vehicle vehicles[MAX_VEHICLES_PER_LANE];
    int front;
    int rear;
    int count;
    light_state_t current_light;
};

/* Struttura per memoria condivisa */
struct shared_memory {
    struct vehicle_queue lanes[NUM_LANES];
    int total_vehicles_generated;
    int total_vehicles_processed;
    int simulation_running;
};

/* Nomi dei semafori POSIX */
#define SEM_NORTH "/traffic_north"
#define SEM_SOUTH "/traffic_south"  
#define SEM_EAST "/traffic_east"
#define SEM_WEST "/traffic_west"
#define SEM_MUTEX "/traffic_mutex"
#define SHM_NAME "/traffic_shm"

/* Variabili globali */
static struct shared_memory *shared_mem;
static sem_t *sem_lanes[NUM_LANES];
static sem_t *sem_mutex;
static const char *lane_names[] = {"NORD", "SUD", "EST", "OVEST"};
static const char *sem_names[] = {SEM_NORTH, SEM_SOUTH, SEM_EAST, SEM_WEST};

/* Funzioni per gestione code */
int is_queue_empty(struct vehicle_queue *queue);
int is_queue_full(struct vehicle_queue *queue);
int enqueue_vehicle(struct vehicle_queue *queue, struct vehicle *veh);
int dequeue_vehicle(struct vehicle_queue *queue, struct vehicle *veh);

/* Funzioni per gestione semafori e memoria condivisa */
int init_shared_resources(void);
void cleanup_shared_resources(void);
int init_semaphores(void);
void cleanup_semaphores(void);

/* Processi della simulazione */
void lane_process(int lane_id);
void traffic_light_controller(void);
void generate_vehicle(int lane_id, int vehicle_id);
void process_vehicle_transit(int lane_id);

/* Funzioni di utilità */
void print_intersection_status(void);
void signal_handler(int sig);

/*
 * Controlla se la coda è vuota
 */
int is_queue_empty(struct vehicle_queue *queue)
{
    return (queue->count == 0);
}

/*
 * Controlla se la coda è piena
 */
int is_queue_full(struct vehicle_queue *queue)
{
    return (queue->count >= MAX_VEHICLES_PER_LANE);
}

/*
 * Aggiunge un veicolo alla coda
 */
int enqueue_vehicle(struct vehicle_queue *queue, struct vehicle *veh)
{
    if (is_queue_full(queue)) {
        return -1;  /* Coda piena */
    }
    
    queue->vehicles[queue->rear] = *veh;
    queue->rear = (queue->rear + 1) % MAX_VEHICLES_PER_LANE;
    queue->count++;
    
    return 0;
}

/*
 * Rimuove un veicolo dalla coda
 */
int dequeue_vehicle(struct vehicle_queue *queue, struct vehicle *veh)
{
    if (is_queue_empty(queue)) {
        return -1;  /* Coda vuota */
    }
    
    *veh = queue->vehicles[queue->front];
    queue->front = (queue->front + 1) % MAX_VEHICLES_PER_LANE;
    queue->count--;
    
    return 0;
}

/*
 * Inizializza la memoria condivisa
 */
int init_shared_resources(void)
{
    int shm_fd;
    int i;
    
    /* Crea memoria condivisa */
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        err_ret("shm_open failed");
        return -1;
    }
    
    /* Imposta dimensione memoria condivisa */
    if (ftruncate(shm_fd, sizeof(struct shared_memory)) == -1) {
        err_ret("ftruncate failed");
        close(shm_fd);
        return -1;
    }
    
    /* Mappa memoria condivisa */
    shared_mem = (struct shared_memory*)mmap(NULL, sizeof(struct shared_memory),
                                           PROT_READ | PROT_WRITE, MAP_SHARED, 
                                           shm_fd, 0);
    if (shared_mem == MAP_FAILED) {
        err_ret("mmap failed");
        close(shm_fd);
        return -1;
    }
    
    close(shm_fd);
    
    /* Inizializza strutture dati */
    for (i = 0; i < NUM_LANES; i++) {
        shared_mem->lanes[i].front = 0;
        shared_mem->lanes[i].rear = 0;
        shared_mem->lanes[i].count = 0;
        shared_mem->lanes[i].current_light = RED_LIGHT;
    }
    
    shared_mem->total_vehicles_generated = 0;
    shared_mem->total_vehicles_processed = 0;
    shared_mem->simulation_running = 1;
    
    return 0;
}

/*
 * Pulizia memoria condivisa
 */
void cleanup_shared_resources(void)
{
    if (shared_mem != NULL) {
        munmap(shared_mem, sizeof(struct shared_memory));
    }
    shm_unlink(SHM_NAME);
}

/*
 * Inizializza i semafori POSIX
 */
int init_semaphores(void)
{
    int i;
    
    /* Crea semafori per ogni via (inizialmente rosso = 0) */
    for (i = 0; i < NUM_LANES; i++) {
        sem_unlink(sem_names[i]);  /* Rimuove semaforo esistente */
        sem_lanes[i] = sem_open(sem_names[i], O_CREAT, 0666, 0);
        if (sem_lanes[i] == SEM_FAILED) {
            err_ret("sem_open failed for lane %d", i);
            return -1;
        }
    }
    
    /* Crea semaforo mutex per accesso esclusivo */
    sem_unlink(SEM_MUTEX);
    sem_mutex = sem_open(SEM_MUTEX, O_CREAT, 0666, 1);
    if (sem_mutex == SEM_FAILED) {
        err_ret("sem_open failed for mutex");
        return -1;
    }
    
    return 0;
}

/*
 * Pulizia semafori POSIX
 */
void cleanup_semaphores(void)
{
    int i;
    
    for (i = 0; i < NUM_LANES; i++) {
        if (sem_lanes[i] != NULL) {
            sem_close(sem_lanes[i]);
            sem_unlink(sem_names[i]);
        }
    }
    
    if (sem_mutex != NULL) {
        sem_close(sem_mutex);
        sem_unlink(SEM_MUTEX);
    }
}

/*
 * Genera un veicolo per una specifica via
 */
void generate_vehicle(int lane_id, int vehicle_id)
{
    struct vehicle new_vehicle;
    
    new_vehicle.id = vehicle_id;
    new_vehicle.arrival_time = time(NULL);
    new_vehicle.lane = lane_id;
    
    /* Acquisisce mutex per accesso esclusivo alla memoria condivisa */
    sem_wait(sem_mutex);
    
    if (!is_queue_full(&shared_mem->lanes[lane_id])) {
        if (enqueue_vehicle(&shared_mem->lanes[lane_id], &new_vehicle) == 0) {
            shared_mem->total_vehicles_generated++;
            printf("Via %s: Veicolo #%d generato (Coda: %d veicoli)\n", 
                   lane_names[lane_id], vehicle_id, 
                   shared_mem->lanes[lane_id].count);
        }
    } else {
        printf("Via %s: Coda piena! Veicolo #%d perso\n", 
               lane_names[lane_id], vehicle_id);
    }
    
    /* Rilascia mutex */
    sem_post(sem_mutex);
}

/*
 * Gestisce il transito dei veicoli quando hanno il verde
 */
void process_vehicle_transit(int lane_id)
{
    struct vehicle veh;
    
    /* Controlla se la via ha il verde */
    if (shared_mem->lanes[lane_id].current_light != GREEN_LIGHT) {
        return;
    }
    
    /* Acquisisce mutex per accesso esclusivo */
    sem_wait(sem_mutex);
    
    /* Rimuove veicolo dalla coda se presente */
    if (!is_queue_empty(&shared_mem->lanes[lane_id])) {
        if (dequeue_vehicle(&shared_mem->lanes[lane_id], &veh) == 0) {
            shared_mem->total_vehicles_processed++;
            printf("Via %s: Veicolo #%d transitato (Tempo attesa: %ld sec, Coda: %d)\n",
                   lane_names[lane_id], veh.id, 
                   time(NULL) - veh.arrival_time,
                   shared_mem->lanes[lane_id].count);
        }
    }
    
    /* Rilascia mutex */
    sem_post(sem_mutex);
}

/*
 * Processo che gestisce una specifica via
 */
void lane_process(int lane_id)
{
    int vehicle_counter = 1;
    time_t last_generation = time(NULL);
    time_t last_transit = time(NULL);
    
    printf("Processo Via %s (PID: %d) avviato\n", lane_names[lane_id], getpid());
    
    while (shared_mem->simulation_running) {
        time_t current_time = time(NULL);
        
        /* Genera veicoli casualmente */
        if (current_time - last_generation >= VEHICLE_GENERATION_INTERVAL) {
            if (rand() % 3 == 0) {  /* 33% probabilità di generare veicolo */
                generate_vehicle(lane_id, vehicle_counter++);
            }
            last_generation = current_time;
        }
        
        /* Processa transito veicoli se ha il verde */
        if (current_time - last_transit >= TRANSIT_TIME) {
            process_vehicle_transit(lane_id);
            last_transit = current_time;
        }
        
        sleep(1);  /* Controllo ogni secondo */
    }
    
    printf("Processo Via %s terminato\n", lane_names[lane_id]);
}

/*
 * Processo che controlla il semaforo centrale
 */
void traffic_light_controller(void)
{
    int current_green_lane = NORTH;
    time_t phase_start_time;
    int phase; /* 0=verde, 1=giallo, 2=pausa */
    
    printf("Controllore Semaforo (PID: %d) avviato\n", getpid());
    
    while (shared_mem->simulation_running) {
        phase_start_time = time(NULL);
        
        /* FASE VERDE */
        printf("\n=== SEMAFORO: Via %s VERDE ===\n", lane_names[current_green_lane]);
        
        /* Imposta luci: verde per via corrente, rosso per altre */
        sem_wait(sem_mutex);
        shared_mem->lanes[current_green_lane].current_light = GREEN_LIGHT;
        for (phase = 0; phase < NUM_LANES; phase++) {
            if (phase != current_green_lane) {
                shared_mem->lanes[phase].current_light = RED_LIGHT;
            }
        }
        sem_post(sem_mutex);
        
        /* Permette transito per durata verde */
        for (phase = 0; phase < GREEN_LIGHT_DURATION; phase++) {
            if (!shared_mem->simulation_running) break;
            sem_post(sem_lanes[current_green_lane]);  /* Segnala verde */
            sleep(1);
        }
        
        /* FASE GIALLO */
        printf("=== SEMAFORO: Via %s GIALLO ===\n", lane_names[current_green_lane]);
        
        sem_wait(sem_mutex);
        shared_mem->lanes[current_green_lane].current_light = YELLOW_LIGHT;
        sem_post(sem_mutex);
        
        sleep(YELLOW_LIGHT_DURATION);
        
        /* FASE ROSSO */
        printf("=== SEMAFORO: Via %s ROSSO ===\n", lane_names[current_green_lane]);
        
        sem_wait(sem_mutex);
        shared_mem->lanes[current_green_lane].current_light = RED_LIGHT;
        sem_post(sem_mutex);
        
        /* Passa alla prossima via */
        current_green_lane = (current_green_lane + 1) % NUM_LANES;
        
        sleep(1);  /* Breve pausa tra cicli */
    }
    
    printf("Controllore Semaforo terminato\n");
}

/*
 * Stampa lo stato corrente dell'incrocio
 */
void print_intersection_status(void)
{
    int i;
    const char *light_str[] = {"ROSSO", "GIALLO", "VERDE"};
    
    printf("\n=== STATO INCROCIO ===\n");
    
    sem_wait(sem_mutex);
    
    for (i = 0; i < NUM_LANES; i++) {
        printf("Via %-5s: %s (%d veicoli in coda)\n", 
               lane_names[i], 
               light_str[shared_mem->lanes[i].current_light],
               shared_mem->lanes[i].count);
    }
    
    printf("Totale generati: %d, Totale transitati: %d\n",
           shared_mem->total_vehicles_generated,
           shared_mem->total_vehicles_processed);
    
    sem_post(sem_mutex);
    
    printf("=====================\n\n");
}

/*
 * Gestore segnali per terminazione pulita
 */
void signal_handler(int sig)
{
    printf("\nRicevuto segnale %d, terminazione simulazione...\n", sig);
    if (shared_mem != NULL) {
        shared_mem->simulation_running = 0;
    }
}

/*
 * Funzione principale
 */
int main(void)
{
    pid_t lane_pids[NUM_LANES];
    pid_t controller_pid;
    int i, status;
    time_t simulation_start;
    
    printf("=== SIMULAZIONE INCROCIO STRADALE A 4 VIE ===\n");
    printf("Durata simulazione: %d secondi\n", SIM_DURATION);
    printf("Tempo verde: %d sec, Tempo giallo: %d sec\n", 
           GREEN_LIGHT_DURATION, YELLOW_LIGHT_DURATION);
    
    /* Installa gestore segnali */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* Inizializza generatore numeri casuali */
    srand((unsigned int)time(NULL));
    
    /* Inizializza risorse condivise */
    if (init_shared_resources() == -1) {
        err_sys("Inizializzazione memoria condivisa fallita");
    }
    
    if (init_semaphores() == -1) {
        cleanup_shared_resources();
        err_sys("Inizializzazione semafori fallita");
    }
    
    printf("Risorse inizializzate correttamente\n");
    
    /* Crea processo controllore semaforo */
    if ((controller_pid = fork()) == -1) {
        err_sys("fork failed for traffic controller");
    } else if (controller_pid == 0) {
        traffic_light_controller();
        exit(0);
    }
    
    /* Crea processi per le 4 vie */
    for (i = 0; i < NUM_LANES; i++) {
        if ((lane_pids[i] = fork()) == -1) {
            err_sys("fork failed for lane %d", i);
        } else if (lane_pids[i] == 0) {
            lane_process(i);
            exit(0);
        }
    }
    
    /* Registra tempo inizio simulazione */
    simulation_start = time(NULL);
    
    /* Processo principale: monitora simulazione */
    while (shared_mem->simulation_running && 
           (time(NULL) - simulation_start) < SIM_DURATION) {
        
        sleep(5);  /* Stampa stato ogni 5 secondi */
        print_intersection_status();
    }
    
    /* Termina simulazione */
    printf("Simulazione completata, terminazione processi...\n");
    shared_mem->simulation_running = 0;
    
    /* Aspetta terminazione controllore semaforo */
    waitpid(controller_pid, &status, 0);
    
    /* Aspetta terminazione processi vie */
    for (i = 0; i < NUM_LANES; i++) {
        waitpid(lane_pids[i], &status, 0);
    }
    
    /* Stampa statistiche finali */
    print_intersection_status();
    
    printf("\n=== STATISTICHE FINALI ===\n");
    printf("Veicoli generati totali: %d\n", shared_mem->total_vehicles_generated);
    printf("Veicoli transitati totali: %d\n", shared_mem->total_vehicles_processed);
    printf("Efficienza: %.1f%%\n", 
           shared_mem->total_vehicles_generated > 0 ? 
           (100.0 * shared_mem->total_vehicles_processed / shared_mem->total_vehicles_generated) : 0.0);
    
    /* Pulizia risorse */
    cleanup_semaphores();
    cleanup_shared_resources();
    
    printf("Simulazione terminata correttamente\n");
    return 0;
}