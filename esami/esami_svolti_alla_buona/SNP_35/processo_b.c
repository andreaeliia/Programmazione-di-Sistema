/*
 * processo_b.c - Processo B che riceve file dai thread del processo A
 * 
 * Legge sequenze di dati dall'area di memoria condivisa e invia segnali
 * di conferma tramite pipe.
 */

#include "apue.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>

#define SHARED_SIZE 20
#define SIGLA_SIZE 2
#define DATA_SIZE (SHARED_SIZE - SIGLA_SIZE)
#define SIGNAL_BYTE 0xAA

/* Variabili globali */
static char *shared_memory = NULL;
static int pipe_fd = -1;
static sem_t *shared_sem = NULL;
static int output_fds[3] = {-1, -1, -1};  /* File di output per ogni thread */

/*
 * Funzione per inizializzare l'accesso alla memoria condivisa
 */
static int init_shared_memory(void)
{
    int fd;
    
    /* Apre l'area di memoria condivisa creata dal processo A */
    fd = shm_open("/shared_mem_ab", O_RDWR, 0666);
    if (fd == -1) {
        err_sys("shm_open error - assicurarsi che processo A sia attivo");
    }
    
    /* Mappa la memoria */
    shared_memory = mmap(NULL, SHARED_SIZE, PROT_READ | PROT_WRITE,
                        MAP_SHARED, fd, 0);
    if (shared_memory == MAP_FAILED) {
        err_sys("mmap error");
    }
    
    close(fd);
    
    /* Apre il semaforo */
    shared_sem = sem_open("/shared_sem_ab", 0);
    if (shared_sem == SEM_FAILED) {
        err_sys("sem_open error");
    }
    
    return 0;
}

/*
 * Funzione per aprire la pipe di comunicazione
 */
static int init_pipe(void)
{
    printf("Inserire il file descriptor della pipe (processo A lo mostra all'avvio): ");
    if (scanf("%d", &pipe_fd) != 1 || pipe_fd <= 0) {
        err_quit("File descriptor pipe non valido");
    }
    
    /* Verifica che il file descriptor sia valido */
    if (fcntl(pipe_fd, F_GETFL) == -1) {
        err_sys("File descriptor pipe non valido");
    }
    
    return 0;
}

/*
 * Funzione per identificare il thread dalla sigla
 */
static int get_thread_id_from_sigla(const char *sigla)
{
    if (memcmp(sigla, "T1", 2) == 0) return 0;
    if (memcmp(sigla, "T2", 2) == 0) return 1;
    if (memcmp(sigla, "T3", 2) == 0) return 2;
    return -1;
}

/*
 * Funzione per aprire i file di output
 */
static void open_output_files(void)
{
    char filename[256];
    int i;
    
    for (i = 0; i < 3; i++) {
        snprintf(filename, sizeof(filename), "received_from_thread_%d.dat", i);
        output_fds[i] = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (output_fds[i] == -1) {
            err_sys("open output file error for %s", filename);
        }
        printf("File di output creato: %s\n", filename);
    }
}

/*
 * Funzione per inviare segnale di conferma
 */
static int send_acknowledgment(void)
{
    char signal = SIGNAL_BYTE;
    
    if (write(pipe_fd, &signal, 1) != 1) {
        err_sys("write acknowledgment error");
    }
    
    return 0;
}

/*
 * Funzione per leggere dati dalla memoria condivisa
 */
static int read_from_shared(char *sigla, char *data)
{
    /* Acquisisce il semaforo */
    if (sem_wait(shared_sem) == -1) {
        err_sys("sem_wait error");
    }
    
    /* Legge la sigla */
    memcpy(sigla, shared_memory, SIGLA_SIZE);
    
    /* Legge i dati */
    memcpy(data, shared_memory + SIGLA_SIZE, DATA_SIZE);
    
    /* Rilascia il semaforo */
    if (sem_post(shared_sem) == -1) {
        err_sys("sem_post error");
    }
    
    return 0;
}

/*
 * Funzione principale del processo B
 */
int main(void)
{
    char sigla[SIGLA_SIZE];
    char data[DATA_SIZE];
    int thread_id;
    int sequence_count = 0;
    long total_bytes[3] = {0, 0, 0};
    
    printf("=== PROCESSO B - Avvio ===\n");
    
    /* Inizializza la memoria condivisa */
    init_shared_memory();
    printf("Memoria condivisa connessa\n");
    
    /* Inizializza la pipe */
    init_pipe();
    printf("Pipe connessa (fd=%d)\n", pipe_fd);
    
    /* Apre i file di output */
    open_output_files();
    
    printf("Processo B pronto per ricevere dati...\n");
    printf("Premere Ctrl+C per terminare\n\n");
    
    /* Ciclo principale di ricezione */
    while (1) {
        /* Legge dalla memoria condivisa */
        read_from_shared(sigla, data);
        
        /* Identifica il thread mittente */
        thread_id = get_thread_id_from_sigla(sigla);
        if (thread_id == -1) {
            printf("Sigla non riconosciuta: %.2s\n", sigla);
            continue;
        }
        
        /* Scrive i dati nel file di output corrispondente */
        if (write(output_fds[thread_id], data, DATA_SIZE) != DATA_SIZE) {
            err_sys("write to output file error");
        }
        
        total_bytes[thread_id] += DATA_SIZE;
        sequence_count++;
        
        printf("Sequenza %d: Ricevuta da thread %d (%.2s) - %d byte "
               "(totale thread: %ld byte)\n",
               sequence_count, thread_id, sigla, DATA_SIZE,
               total_bytes[thread_id]);
        
        /* Invia conferma */
        send_acknowledgment();
        
        /* Statistiche ogni 100 sequenze */
        if (sequence_count % 100 == 0) {
            printf("\n--- Statistiche (sequenza %d) ---\n", sequence_count);
            printf("Thread 0: %ld byte ricevuti\n", total_bytes[0]);
            printf("Thread 1: %ld byte ricevuti\n", total_bytes[1]);
            printf("Thread 2: %ld byte ricevuti\n", total_bytes[2]);
            printf("Totale: %ld byte\n\n",
                   total_bytes[0] + total_bytes[1] + total_bytes[2]);
        }
    }
    
    /* Cleanup (mai raggiunto in questo esempio) */
    munmap(shared_memory, SHARED_SIZE);
    sem_close(shared_sem);
    close(pipe_fd);
    
    int i;
    for (i = 0; i < 3; i++) {
        if (output_fds[i] != -1) {
            close(output_fds[i]);
        }
    }
    
    return 0;
}