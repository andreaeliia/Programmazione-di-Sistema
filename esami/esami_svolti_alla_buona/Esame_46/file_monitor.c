/*
 * file_monitor.c
 * 
 * Programma C che utilizza due thread per leggere alternativamente
 * i dati da un file in crescita (dump.txt) e cercare sequenze di
 * tre caratteri alfanumerici uguali.
 * 
 * Compilazione: utilizzare il Makefile fornito
 * Esecuzione: ./file_monitor
 * 
 * Prerequisiti: 
 * - File dump.txt generato dallo script bash fornito
 * - Libreria APUE (apue.h)
 */

#include "apue.h"
#include <pthread.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024
#define PATTERN_LENGTH 3

/* Struttura per condividere dati tra thread */
typedef struct {
    FILE *fp;                    /* File pointer condiviso */
    pthread_mutex_t file_mutex;  /* Mutex per accesso esclusivo al file */
    pthread_mutex_t print_mutex; /* Mutex per output sincronizzato */
    int thread_id;               /* ID del thread (1 o 2) */
    long total_processed;        /* Totale byte processati */
} thread_data_t;

/* Variabili globali */
static thread_data_t shared_data;
static int active_thread = 1;    /* Thread attualmente attivo (1 o 2) */
static pthread_cond_t turn_cond; /* Condition variable per alternanza */
static pthread_mutex_t turn_mutex; /* Mutex per controllo alternanza */

/* Prototipi delle funzioni */
static void* thread_worker(void* arg);
static int is_alphanumeric(char c);
static void process_buffer(char* buffer, int size, int thread_id);
static void print_progress(int thread_id, int bytes_read, long total_bytes);
static void print_match_found(int thread_id, char pattern, long position);
static void wait_for_turn(int thread_id);
static void signal_next_thread(int thread_id);
static void init_shared_data(void);
static void cleanup_shared_data(void);

/*
 * Funzione principale
 */
int main(void)
{
    pthread_t thread1, thread2;
    thread_data_t data1, data2;
    int ret;
    
    printf("Avvio monitoraggio file dump.txt...\n");
    printf("Premere Ctrl+C per terminare\n\n");
    
    /* Inizializza strutture dati condivise */
    init_shared_data();
    
    /* Apre il file in modalità lettura */
    shared_data.fp = fopen("dump.txt", "r");
    if (shared_data.fp == NULL) {
        err_sys("Errore apertura file dump.txt");
    }
    
    /* Prepara dati per i thread */
    data1.fp = shared_data.fp;
    data1.thread_id = 1;
    data1.total_processed = 0;
    
    data2.fp = shared_data.fp;
    data2.thread_id = 2;
    data2.total_processed = 0;
    
    /* Crea i thread */
    ret = pthread_create(&thread1, NULL, thread_worker, &data1);
    if (ret != 0) {
        err_quit("Errore creazione thread 1: %s", strerror(ret));
    }
    
    ret = pthread_create(&thread2, NULL, thread_worker, &data2);
    if (ret != 0) {
        err_quit("Errore creazione thread 2: %s", strerror(ret));
    }
    
    /* Attende terminazione dei thread */
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    /* Pulizia risorse */
    fclose(shared_data.fp);
    cleanup_shared_data();
    
    return 0;
}

/*
 * Funzione eseguita dai thread worker
 * Ogni thread legge alternativamente dal file e processa i dati
 */
static void* thread_worker(void* arg)
{
    thread_data_t* data = (thread_data_t*)arg;
    char buffer[BUFFER_SIZE];
    int bytes_read;
    long file_pos;
    
    while (1) {
        /* Attende il proprio turno */
        wait_for_turn(data->thread_id);
        
        /* Accesso esclusivo al file */
        pthread_mutex_lock(&shared_data.file_mutex);
        
        /* Legge dal file */
        bytes_read = fread(buffer, 1, BUFFER_SIZE, data->fp);
        file_pos = ftell(data->fp);
        
        pthread_mutex_unlock(&shared_data.file_mutex);
        
        if (bytes_read > 0) {
            /* Processa i dati letti */
            shared_data.total_processed += bytes_read;
            print_progress(data->thread_id, bytes_read, shared_data.total_processed);
            process_buffer(buffer, bytes_read, data->thread_id);
            
            /* Azzera i byte elaborati nel file */
            pthread_mutex_lock(&shared_data.file_mutex);
            fseek(data->fp, file_pos, SEEK_SET);
            pthread_mutex_unlock(&shared_data.file_mutex);
        } else {
            /* Non ci sono nuovi dati, attende */
            usleep(100000); /* 100ms */
        }
        
        /* Cede il turno all'altro thread */
        signal_next_thread(data->thread_id);
    }
    
    return NULL;
}

/*
 * Verifica se un carattere è alfanumerico
 */
static int is_alphanumeric(char c)
{
    return (isalnum((unsigned char)c));
}

/*
 * Processa il buffer cercando sequenze di tre caratteri alfanumerici uguali
 */
static void process_buffer(char* buffer, int size, int thread_id)
{
    int i;
    static char prev_chars[2] = {0, 0}; /* Caratteri precedenti per continuità */
    static int prev_count = 0;
    
    for (i = 0; i < size; i++) {
        char current = buffer[i];
        
        if (is_alphanumeric(current)) {
            /* Controlla se forma una sequenza di 3 caratteri uguali */
            if (prev_count >= 2 && 
                prev_chars[0] == current && 
                prev_chars[1] == current) {
                
                print_match_found(thread_id, current, 
                                shared_data.total_processed - size + i - 2);
            }
            
            /* Aggiorna i caratteri precedenti */
            prev_chars[0] = prev_chars[1];
            prev_chars[1] = current;
            if (prev_count < 2) prev_count++;
        } else {
            /* Reset se incontriamo un carattere non alfanumerico */
            prev_count = 0;
            prev_chars[0] = prev_chars[1] = 0;
        }
    }
}

/*
 * Stampa il progresso dell'elaborazione
 */
static void print_progress(int thread_id, int bytes_read, long total_bytes)
{
    pthread_mutex_lock(&shared_data.print_mutex);
    printf("[Thread %d] Letti %d bytes - Totale processati: %ld bytes\n", 
           thread_id, bytes_read, total_bytes);
    pthread_mutex_unlock(&shared_data.print_mutex);
}

/*
 * Stampa un match trovato
 */
static void print_match_found(int thread_id, char pattern, long position)
{
    pthread_mutex_lock(&shared_data.print_mutex);
    printf("*** [Thread %d] TROVATA SEQUENZA: '%c%c%c' alla posizione %ld ***\n", 
           thread_id, pattern, pattern, pattern, position);
    pthread_mutex_unlock(&shared_data.print_mutex);
}

/*
 * Attende il proprio turno per processare
 */
static void wait_for_turn(int thread_id)
{
    pthread_mutex_lock(&turn_mutex);
    while (active_thread != thread_id) {
        pthread_cond_wait(&turn_cond, &turn_mutex);
    }
    pthread_mutex_unlock(&turn_mutex);
}

/*
 * Segnala all'altro thread che è il suo turno
 */
static void signal_next_thread(int thread_id)
{
    pthread_mutex_lock(&turn_mutex);
    active_thread = (thread_id == 1) ? 2 : 1;
    pthread_cond_broadcast(&turn_cond);
    pthread_mutex_unlock(&turn_mutex);
}

/*
 * Inizializza le strutture dati condivise
 */
static void init_shared_data(void)
{
    int ret;
    
    /* Inizializza mutex */
    ret = pthread_mutex_init(&shared_data.file_mutex, NULL);
    if (ret != 0) {
        err_quit("Errore inizializzazione file_mutex: %s", strerror(ret));
    }
    
    ret = pthread_mutex_init(&shared_data.print_mutex, NULL);
    if (ret != 0) {
        err_quit("Errore inizializzazione print_mutex: %s", strerror(ret));
    }
    
    ret = pthread_mutex_init(&turn_mutex, NULL);
    if (ret != 0) {
        err_quit("Errore inizializzazione turn_mutex: %s", strerror(ret));
    }
    
    /* Inizializza condition variable */
    ret = pthread_cond_init(&turn_cond, NULL);
    if (ret != 0) {
        err_quit("Errore inizializzazione turn_cond: %s", strerror(ret));
    }
    
    shared_data.total_processed = 0;
}

/*
 * Pulisce le risorse allocate
 */
static void cleanup_shared_data(void)
{
    pthread_mutex_destroy(&shared_data.file_mutex);
    pthread_mutex_destroy(&shared_data.print_mutex);
    pthread_mutex_destroy(&turn_mutex);
    pthread_cond_destroy(&turn_cond);
}