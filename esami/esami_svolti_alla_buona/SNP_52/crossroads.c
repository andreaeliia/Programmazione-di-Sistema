/* 
 * crossroads.c - Simulazione di un incrocio a 4 vie
 * 
 * Questo programma simula un incrocio stradale con 4 strade:
 * - 4 processi generano casualmente l'arrivo di veicoli
 * - 1 processo controlla il semaforo dell'incrocio
 * - Solo un veicolo alla volta può attraversare l'incrocio
 * - L'attraversamento richiede 3 secondi
 * 
 * Compilazione: fare riferimento al Makefile fornito
 * Uso: ./crossroads
 */

#include "apue.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

/* Costanti del programma */
#define NUM_ROADS 4                /* Numero di strade */
#define MAX_QUEUE 10              /* Dimensione massima coda per strada */
#define CROSSING_TIME 3           /* Tempo attraversamento in secondi */
#define MAX_VEHICLES 50           /* Massimo numero veicoli totali */
#define VEHICLE_ARRIVAL_PROB 30   /* Probabilità arrivo veicolo (su 100) */

/* Struttura per rappresentare un veicolo */
typedef struct {
    int id;                       /* ID univoco del veicolo */
    int road;                     /* Strada di provenienza (0-3) */
} Vehicle;

/* Struttura per la coda di veicoli di una strada */
typedef struct {
    Vehicle queue[MAX_QUEUE];     /* Array di veicoli in coda */
    int front;                    /* Indice primo elemento */
    int rear;                     /* Indice ultimo elemento */
    int count;                    /* Numero veicoli in coda */
} RoadQueue;

/* Struttura per la memoria condivisa */
typedef struct {
    RoadQueue roads[NUM_ROADS];   /* Code delle 4 strade */
    int crossroads_busy;          /* 1 se incrocio occupato, 0 altrimenti */
    int crossing_vehicle_id;      /* ID veicolo che sta attraversando */
    int crossing_road;            /* Strada del veicolo che attraversa */
    int total_vehicles_created;   /* Contatore veicoli creati */
    int total_vehicles_crossed;   /* Contatore veicoli attraversati */
    int simulation_running;       /* Flag per terminare simulazione */
} SharedData;

/* ID globali per cleanup */
static int shmid = -1;
static int semid = -1;
static SharedData *shared_data = NULL;

/* Prototipi delle funzioni */
void cleanup_resources(void);
void signal_handler(int sig);
void road_process(int road_id);
void controller_process(void);
int add_vehicle_to_queue(int road_id, int vehicle_id);
Vehicle remove_vehicle_from_queue(int road_id);
int is_queue_empty(int road_id);
void print_status(void);
void init_semaphores(void);
void sem_wait_wrapper(int sem_num);
void sem_signal_wrapper(int sem_num);

/* Numeri dei semafori */
#define SEM_MUTEX 0               /* Semaforo per accesso esclusivo ai dati */
#define SEM_CROSSROADS 1          /* Semaforo per controllo incrocio */
#define NUM_SEMS 2                /* Numero totale semafori */

/*
 * Funzione principale
 * Inizializza la memoria condivisa, i semafori e crea i processi
 */
int main(void)
{
    key_t key;
    pid_t pids[NUM_ROADS + 1];    /* PID dei processi figli */
    int i, status;
    
    printf("=== SIMULAZIONE INCROCIO A 4 VIE ===\n");
    printf("Avvio simulazione...\n\n");
    
    /* Registra il gestore di segnali per cleanup */
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        err_sys("signal error");
    }
    
    /* Genera chiave per IPC */
    if ((key = ftok(".", 'C')) == -1) {
        err_sys("ftok error");
    }
    
    /* Crea memoria condivisa */
    if ((shmid = shmget(key, sizeof(SharedData), IPC_CREAT | 0666)) == -1) {
        err_sys("shmget error");
    }
    
    /* Attacca memoria condivisa */
    if ((shared_data = shmat(shmid, NULL, 0)) == (void *)-1) {
        err_sys("shmat error");
    }
    
    /* Inizializza struttura dati condivisa */
    memset(shared_data, 0, sizeof(SharedData));
    shared_data->simulation_running = 1;
    
    for (i = 0; i < NUM_ROADS; i++) {
        shared_data->roads[i].front = 0;
        shared_data->roads[i].rear = -1;
        shared_data->roads[i].count = 0;
    }
    
    /* Crea set di semafori */
    if ((semid = semget(key, NUM_SEMS, IPC_CREAT | 0666)) == -1) {
        err_sys("semget error");
    }
    
    /* Inizializza semafori */
    init_semaphores();
    
    /* Crea processi per le 4 strade */
    for (i = 0; i < NUM_ROADS; i++) {
        if ((pids[i] = fork()) == 0) {
            /* Processo figlio - gestisce una strada */
            road_process(i);
            exit(0);
        } else if (pids[i] < 0) {
            err_sys("fork error");
        }
    }
    
    /* Crea processo controller */
    if ((pids[NUM_ROADS] = fork()) == 0) {
        /* Processo controller */
        controller_process();
        exit(0);
    } else if (pids[NUM_ROADS] < 0) {
        err_sys("fork error");
    }
    
    /* Processo principale - monitora la simulazione */
    printf("Simulazione avviata. Premi Ctrl+C per terminare.\n");
    printf("Road 0: Nord, Road 1: Est, Road 2: Sud, Road 3: Ovest\n\n");
    
    /* Loop di monitoraggio */
    while (shared_data->simulation_running && 
           shared_data->total_vehicles_crossed < MAX_VEHICLES) {
        
        sleep(2);
        print_status();
        
        /* Termina se tutti i veicoli sono passati */
        if (shared_data->total_vehicles_crossed >= MAX_VEHICLES) {
            shared_data->simulation_running = 0;
            break;
        }
    }
    
    /* Termina tutti i processi figli */
    for (i = 0; i <= NUM_ROADS; i++) {
        kill(pids[i], SIGTERM);
        waitpid(pids[i], &status, 0);
    }
    
    printf("\n=== SIMULAZIONE TERMINATA ===\n");
    printf("Veicoli totali creati: %d\n", shared_data->total_vehicles_created);
    printf("Veicoli totali attraversati: %d\n", shared_data->total_vehicles_crossed);
    
    cleanup_resources();
    return 0;
}

/*
 * Processo che gestisce una strada
 * Genera casualmente veicoli e li aggiunge alla coda
 */
void road_process(int road_id)
{
    int vehicle_counter = 0;
    
    srand(time(NULL) + road_id);  /* Seed diverso per ogni processo */
    
    while (shared_data->simulation_running) {
        /* Genera veicolo con una certa probabilità */
        if (rand() % 100 < VEHICLE_ARRIVAL_PROB) {
            sem_wait_wrapper(SEM_MUTEX);
            
            if (shared_data->total_vehicles_created < MAX_VEHICLES) {
                int vehicle_id = ++shared_data->total_vehicles_created;
                
                if (add_vehicle_to_queue(road_id, vehicle_id)) {
                    printf("Veicolo %d arrivato sulla strada %d\n", 
                           vehicle_id, road_id);
                }
            }
            
            sem_signal_wrapper(SEM_MUTEX);
        }
        
        sleep(1 + rand() % 3);  /* Attesa casuale 1-3 secondi */
    }
}

/*
 * Processo controller dell'incrocio
 * Gestisce il passaggio dei veicoli attraverso l'incrocio
 */
void controller_process(void)
{
    int current_road = 0;         /* Strada corrente da controllare */
    int roads_checked = 0;        /* Contatore strade controllate */
    Vehicle crossing_vehicle;
    
    while (shared_data->simulation_running) {
        sem_wait_wrapper(SEM_MUTEX);
        
        /* Se l'incrocio è libero, cerca un veicolo da far passare */
        if (!shared_data->crossroads_busy) {
            roads_checked = 0;
            
            /* Controlla le strade in sequenza circolare */
            while (roads_checked < NUM_ROADS) {
                if (!is_queue_empty(current_road)) {
                    /* Trovato un veicolo, inizia attraversamento */
                    crossing_vehicle = remove_vehicle_from_queue(current_road);
                    shared_data->crossroads_busy = 1;
                    shared_data->crossing_vehicle_id = crossing_vehicle.id;
                    shared_data->crossing_road = current_road;
                    
                    printf(">>> Veicolo %d dalla strada %d sta attraversando l'incrocio\n",
                           crossing_vehicle.id, current_road);
                    
                    sem_signal_wrapper(SEM_MUTEX);
                    
                    /* Simula attraversamento */
                    sleep(CROSSING_TIME);
                    
                    sem_wait_wrapper(SEM_MUTEX);
                    
                    /* Attraversamento completato */
                    shared_data->crossroads_busy = 0;
                    shared_data->total_vehicles_crossed++;
                    
                    printf("<<< Veicolo %d ha completato l'attraversamento\n",
                           crossing_vehicle.id);
                    
                    break;
                }
                
                current_road = (current_road + 1) % NUM_ROADS;
                roads_checked++;
            }
        }
        
        sem_signal_wrapper(SEM_MUTEX);
        sleep(1);
    }
}

/*
 * Aggiunge un veicolo alla coda di una strada
 * Ritorna 1 se successful, 0 se coda piena
 */
int add_vehicle_to_queue(int road_id, int vehicle_id)
{
    RoadQueue *road = &shared_data->roads[road_id];
    
    if (road->count >= MAX_QUEUE) {
        return 0;  /* Coda piena */
    }
    
    road->rear = (road->rear + 1) % MAX_QUEUE;
    road->queue[road->rear].id = vehicle_id;
    road->queue[road->rear].road = road_id;
    road->count++;
    
    return 1;
}

/*
 * Rimuove un veicolo dalla testa della coda di una strada
 * Assume che la coda non sia vuota
 */
Vehicle remove_vehicle_from_queue(int road_id)
{
    RoadQueue *road = &shared_data->roads[road_id];
    Vehicle vehicle = road->queue[road->front];
    
    road->front = (road->front + 1) % MAX_QUEUE;
    road->count--;
    
    return vehicle;
}

/*
 * Controlla se la coda di una strada è vuota
 */
int is_queue_empty(int road_id)
{
    return shared_data->roads[road_id].count == 0;
}

/*
 * Stampa lo stato corrente della simulazione
 */
void print_status(void)
{
    int i;
    
    printf("--- STATUS ---\n");
    printf("Incrocio: %s", shared_data->crossroads_busy ? "OCCUPATO" : "LIBERO");
    
    if (shared_data->crossroads_busy) {
        printf(" (Veicolo %d dalla strada %d)\n", 
               shared_data->crossing_vehicle_id, 
               shared_data->crossing_road);
    } else {
        printf("\n");
    }
    
    for (i = 0; i < NUM_ROADS; i++) {
        printf("Strada %d: %d veicoli in coda\n", 
               i, shared_data->roads[i].count);
    }
    
    printf("Creati: %d, Attraversati: %d\n", 
           shared_data->total_vehicles_created,
           shared_data->total_vehicles_crossed);
    printf("--------------\n\n");
}

/*
 * Inizializza i semafori
 */
void init_semaphores(void)
{
    union semun {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
    } arg;
    
    /* Semaforo mutex inizializzato a 1 */
    arg.val = 1;
    if (semctl(semid, SEM_MUTEX, SETVAL, arg) == -1) {
        err_sys("semctl SEM_MUTEX error");
    }
    
    /* Semaforo crossroads inizializzato a 1 */
    arg.val = 1;
    if (semctl(semid, SEM_CROSSROADS, SETVAL, arg) == -1) {
        err_sys("semctl SEM_CROSSROADS error");
    }
}

/*
 * Wrapper per operazione wait su semaforo
 */
void sem_wait_wrapper(int sem_num)
{
    struct sembuf sb;
    sb.sem_num = sem_num;
    sb.sem_op = -1;
    sb.sem_flg = 0;
    
    if (semop(semid, &sb, 1) == -1) {
        if (errno != EINTR) {
            err_sys("semop wait error");
        }
    }
}

/*
 * Wrapper per operazione signal su semaforo
 */
void sem_signal_wrapper(int sem_num)
{
    struct sembuf sb;
    sb.sem_num = sem_num;
    sb.sem_op = 1;
    sb.sem_flg = 0;
    
    if (semop(semid, &sb, 1) == -1) {
        err_sys("semop signal error");
    }
}

/*
 * Gestore di segnali per cleanup
 */
void signal_handler(int sig)
{
    printf("\nRicevuto segnale %d. Terminando simulazione...\n", sig);
    if (shared_data != NULL) {
        shared_data->simulation_running = 0;
    }
    cleanup_resources();
    exit(0);
}

/*
 * Cleanup delle risorse IPC
 */
void cleanup_resources(void)
{
    if (shared_data != NULL && shmdt(shared_data) == -1) {
        err_sys("shmdt error");
    }
    
    if (shmid != -1 && shmctl(shmid, IPC_RMID, NULL) == -1) {
        err_sys("shmctl IPC_RMID error");
    }
    
    if (semid != -1 && semctl(semid, 0, IPC_RMID) == -1) {
        err_sys("semctl IPC_RMID error");
    }
}