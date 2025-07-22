/*
 * processo_a.c - Processo A con 3 thread che inviano file al processo B
 * 
 * Ogni thread trasferisce un file di 100KB utilizzando un'area condivisa
 * di 20 byte, preceduto da una sigla di 2 byte per identificazione.
 */

#include "apue.h"
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>

#define NUM_THREADS 3
#define SHARED_SIZE 20
#define SIGLA_SIZE 2
#define DATA_SIZE (SHARED_SIZE - SIGLA_SIZE)
#define FILE_SIZE (100 * 1024)  /* 100 KB */

/* Struttura per passare parametri ai thread */
typedef struct {
    int thread_id;
    char sigla[SIGLA_SIZE];
    char filename[256];
} thread_data_t;

/* Variabili globali */
static char *shared_memory = NULL;    /* Area di memoria condivisa */
static int pipe_fd[2];               /* Pipe per comunicazione con processo B */
static pthread_mutex_t write_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t write_cond = PTHREAD_COND_INITIALIZER;
static int current_writer = 0;       /* Thread corrente autorizzato a scrivere */
static sem_t *shared_sem = NULL;     /* Semaforo per sincronizzazione memoria condivisa */

/*
 * Funzione per inizializzare la memoria condivisa
 */
static int init_shared_memory(void)
{
    int fd;
    
    /* Crea il file per la memoria condivisa */
    fd = shm_open("/shared_mem_ab", O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        err_sys("shm_open error");
    }
    
    /* Imposta la dimensione */
    if (ftruncate(fd, SHARED_SIZE) == -1) {
        err_sys("ftruncate error");
    }
    
    /* Mappa la memoria */
    shared_memory = mmap(NULL, SHARED_SIZE, PROT_READ | PROT_WRITE,
                        MAP_SHARED, fd, 0);
    if (shared_memory == MAP_FAILED) {
        err_sys("mmap error");
    }
    
    close(fd);
    
    /* Inizializza il semaforo per la memoria condivisa */
    shared_sem = sem_open("/shared_sem_ab", O_CREAT, 0666, 1);
    if (shared_sem == SEM_FAILED) {
        err_sys("sem_open error");
    }
    
    return 0;
}

/*
 * Funzione per attendere il proprio turno di scrittura
 */
static void wait_for_turn(int thread_id)
{
    pthread_mutex_lock(&write_mutex);
    while (current_writer != thread_id) {
        pthread_cond_wait(&write_cond, &write_mutex);
    }
    pthread_mutex_unlock(&write_mutex);
}

/*
 * Funzione per passare il turno al thread successivo
 */
static void pass_turn(void)
{
    pthread_mutex_lock(&write_mutex);
    current_writer = (current_writer + 1) % NUM_THREADS;
    pthread_cond_broadcast(&write_cond);
    pthread_mutex_unlock(&write_mutex);
}

/*
 * Funzione per attendere il segnale dal processo B
 */
static int wait_for_signal(void)
{
    char signal_byte;
    ssize_t n;
    
    n = read(pipe_fd[0], &signal_byte, 1);
    if (n <= 0) {
        return -1;
    }
    
    return 0;
}

/*
 * Funzione per scrivere dati nell'area condivisa
 */
static int write_to_shared(const char *sigla, const char *data, int data_len)
{
    /* Acquisisce il semaforo per accesso esclusivo alla memoria condivisa */
    if (sem_wait(shared_sem) == -1) {
        err_sys("sem_wait error");
    }
    
    /* Scrive la sigla */
    memcpy(shared_memory, sigla, SIGLA_SIZE);
    
    /* Scrive i dati */
    memcpy(shared_memory + SIGLA_SIZE, data, data_len);
    
    /* Rilascia il semaforo */
    if (sem_post(shared_sem) == -1) {
        err_sys("sem_post error");
    }
    
    return 0;
}

/*
 * Funzione thread per il trasferimento file
 */
static void *thread_function(void *arg)
{
    thread_data_t *data = (thread_data_t *)arg;
    int fd;
    char buffer[DATA_SIZE];
    ssize_t bytes_read, total_sent = 0;
    
    printf("Thread %d: Inizio trasferimento file %s con sigla %.2s\n",
           data->thread_id, data->filename, data->sigla);
    
    /* Apre il file da trasferire */
    fd = open(data->filename, O_RDONLY);
    if (fd == -1) {
        err_sys("open error per %s", data->filename);
    }
    
    /* Ciclo di trasferimento */
    while (total_sent < FILE_SIZE) {
        /* Attende il proprio turno */
        wait_for_turn(data->thread_id);
        
        /* Legge dati dal file */
        bytes_read = read(fd, buffer, DATA_SIZE);
        if (bytes_read <= 0) {
            if (bytes_read == 0) {
                printf("Thread %d: Fine file raggiunta\n", data->thread_id);
            } else {
                err_sys("read error");
            }
            break;
        }
        
        /* Se i dati letti sono meno di DATA_SIZE, riempi con zeri */
        if (bytes_read < DATA_SIZE) {
            memset(buffer + bytes_read, 0, DATA_SIZE - bytes_read);
        }
        
        /* Scrive nell'area condivisa */
        write_to_shared(data->sigla, buffer, DATA_SIZE);
        
        total_sent += bytes_read;
        printf("Thread %d: Inviati %ld byte (totale: %ld/%d)\n",
               data->thread_id, (long)bytes_read, (long)total_sent, FILE_SIZE);
        
        /* Passa il turno */
        pass_turn();
        
        /* Attende segnale dal processo B */
        if (wait_for_signal() == -1) {
            printf("Thread %d: Errore nella ricezione del segnale\n", data->thread_id);
            break;
        }
    }
    
    close(fd);
    printf("Thread %d: Trasferimento completato (%ld byte)\n",
           data->thread_id, (long)total_sent);
    
    return NULL;
}

/*
 * Funzione per creare i file di test
 */
static void create_test_files(void)
{
    int i, fd;
    char filename[256];
    char data_pattern[1024];
    int j;
    
    /* Crea un pattern di dati */
    for (j = 0; j < 1024; j++) {
        data_pattern[j] = (char)('A' + (j % 26));
    }
    
    for (i = 0; i < NUM_THREADS; i++) {
        snprintf(filename, sizeof(filename), "file_%d.dat", i);
        
        fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd == -1) {
            err_sys("create test file error");
        }
        
        /* Scrive 100KB di dati */
        for (j = 0; j < (FILE_SIZE / 1024); j++) {
            /* Modifica il pattern per ogni thread */
            data_pattern[0] = '0' + i;
            if (write(fd, data_pattern, 1024) != 1024) {
                err_sys("write test file error");
            }
        }
        
        close(fd);
        printf("Creato file di test: %s\n", filename);
    }
}

/*
 * Funzione principale
 */
int main(void)
{
    pthread_t threads[NUM_THREADS];
    thread_data_t thread_data[NUM_THREADS];
    char siglas[NUM_THREADS][SIGLA_SIZE] = {"T1", "T2", "T3"};
    int i;
    
    printf("=== PROCESSO A - Avvio ===\n");
    
    /* Crea i file di test */
    create_test_files();
    
    /* Inizializza la memoria condivisa */
    init_shared_memory();
    
    /* Crea la pipe per comunicazione con processo B */
    if (pipe(pipe_fd) == -1) {
        err_sys("pipe error");
    }
    
    printf("Pipe creata: lettura fd=%d, scrittura fd=%d\n", pipe_fd[0], pipe_fd[1]);
    printf("Memoria condivisa inizializzata (%d byte)\n", SHARED_SIZE);
    
    /* Prepara i dati per i thread */
    for (i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i;
        memcpy(thread_data[i].sigla, siglas[i], SIGLA_SIZE);
        snprintf(thread_data[i].filename, sizeof(thread_data[i].filename),
                "file_%d.dat", i);
    }
    
    /* Crea i thread */
    for (i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, thread_function,
                          &thread_data[i]) != 0) {
            err_sys("pthread_create error");
        }
    }
    
    printf("Creati %d thread\n", NUM_THREADS);
    printf("Avviare il processo B per iniziare il trasferimento\n");
    printf("File descriptor pipe scrittura per processo B: %d\n", pipe_fd[1]);
    
    /* Attende la terminazione dei thread */
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    /* Cleanup */
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    munmap(shared_memory, SHARED_SIZE);
    shm_unlink("/shared_mem_ab");
    sem_close(shared_sem);
    sem_unlink("/shared_sem_ab");
    
    printf("=== PROCESSO A - Terminato ===\n");
    return 0;
}