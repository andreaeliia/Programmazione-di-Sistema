/*
 * mirror_daemon.c
 * 
 * Daemon per il mirroring periodico di directory con supporto threading
 * Confronta le performance tra operazioni single-thread e multi-thread
 * 
 * Compilazione: make mirror_daemon
 * Uso: ./mirror_daemon <src_dir> <dest_dir> [thread_mode]
 *      thread_mode: 0 = single-thread, 1 = multi-thread (default: 0)
 */

#include "apue.h"
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>

/* Costanti */
#define MIRROR_INTERVAL 30     /* Intervallo in secondi tra i backup */
#define MAX_PATH_LEN 1024      /* Lunghezza massima del path */
#define BUFFER_SIZE 8192       /* Dimensione buffer per copia file */
#define MAX_THREADS 10         /* Numero massimo di thread worker */

/* Strutture dati */
typedef struct file_info {
    char src_path[MAX_PATH_LEN];
    char dest_path[MAX_PATH_LEN];
    time_t mtime;
    off_t size;
    struct file_info *next;
} file_info_t;

typedef struct thread_data {
    file_info_t *file;
    int thread_id;
} thread_data_t;

/* Variabili globali */
static char g_src_dir[MAX_PATH_LEN];
static char g_dest_dir[MAX_PATH_LEN];
static int g_thread_mode = 0;
static int g_daemon_running = 1;
static pthread_mutex_t g_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static file_info_t *g_files_to_copy = NULL;
static int g_files_copied = 0;
static int g_total_files = 0;

/* Prototipi delle funzioni */
static void daemonize(void);
static void signal_handler(int sig);
static void setup_signal_handlers(void);
static int scan_directory(const char *dir_path, file_info_t **file_list);
static int need_update(const char *src_file, const char *dest_file);
static int copy_file(const char *src, const char *dest);
static void *worker_thread(void *arg);
static void perform_mirror_threaded(file_info_t *file_list);
static void perform_mirror_single(file_info_t *file_list);
static void free_file_list(file_info_t *file_list);
static void log_message(const char *msg);
static double get_time_diff(struct timeval *start, struct timeval *end);

/*
 * Funzione principale
 */
int main(int argc, char *argv[])
{
    file_info_t *file_list;
    struct timeval start_time, end_time;
    double elapsed_time;
    
    /* Verifica argomenti */
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Uso: %s <src_dir> <dest_dir> [thread_mode]\n", argv[0]);
        fprintf(stderr, "thread_mode: 0=single-thread, 1=multi-thread\n");
        exit(1);
    }
    
    /* Inizializza directory */
    strncpy(g_src_dir, argv[1], MAX_PATH_LEN - 1);
    strncpy(g_dest_dir, argv[2], MAX_PATH_LEN - 1);
    g_src_dir[MAX_PATH_LEN - 1] = '\0';
    g_dest_dir[MAX_PATH_LEN - 1] = '\0';
    
    /* Modalità threading */
    if (argc == 4) {
        g_thread_mode = atoi(argv[3]);
        if (g_thread_mode != 0 && g_thread_mode != 1) {
            fprintf(stderr, "Errore: thread_mode deve essere 0 o 1\n");
            exit(1);
        }
    }
    
    /* Verifica esistenza directory sorgente */
    if (access(g_src_dir, R_OK) != 0) {
        fprintf(stderr, "Errore: impossibile accedere alla directory %s\n", g_src_dir);
        exit(1);
    }
    
    /* Crea directory destinazione se non exists */
    if (mkdir(g_dest_dir, 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "Errore nella creazione della directory %s\n", g_dest_dir);
        exit(1);
    }
    
    printf("Avvio daemon mirror - Modalità: %s\n", 
           g_thread_mode ? "multi-thread" : "single-thread");
    printf("Sorgente: %s\n", g_src_dir);
    printf("Destinazione: %s\n", g_dest_dir);
    
    /* Daemonizza il processo */
    daemonize();
    
    /* Setup gestori segnali */
    setup_signal_handlers();
    
    /* Loop principale del daemon */
    while (g_daemon_running) {
        log_message("Inizio scansione directory");
        
        /* Misura tempo di esecuzione */
        gettimeofday(&start_time, NULL);
        
        /* Scansiona directory e trova file da aggiornare */
        file_list = NULL;
        g_total_files = scan_directory(g_src_dir, &file_list);
        
        if (g_total_files > 0) {
            char log_buf[256];
            sprintf(log_buf, "Trovati %d file da verificare", g_total_files);
            log_message(log_buf);
            
            /* Reset contatore file copiati */
            g_files_copied = 0;
            
            /* Esegui mirroring basato sulla modalità */
            if (g_thread_mode) {
                perform_mirror_threaded(file_list);
            } else {
                perform_mirror_single(file_list);
            }
            
            gettimeofday(&end_time, NULL);
            elapsed_time = get_time_diff(&start_time, &end_time);
            
            sprintf(log_buf, "Mirroring completato: %d file copiati in %.2f secondi", 
                    g_files_copied, elapsed_time);
            log_message(log_buf);
            
            /* Libera memoria */
            free_file_list(file_list);
        } else {
            log_message("Nessun file da aggiornare");
        }
        
        /* Attendi prossimo ciclo */
        sleep(MIRROR_INTERVAL);
    }
    
    log_message("Daemon terminato");
    return 0;
}

/*
 * Converte il processo in daemon
 */
static void daemonize(void)
{
    pid_t pid;
    int i;
    
    /* Primo fork */
    if ((pid = fork()) < 0) {
        err_quit("Errore prima fork");
    } else if (pid != 0) {
        exit(0); /* Termina processo padre */
    }
    
    /* Diventa session leader */
    if (setsid() < 0) {
        err_quit("Errore setsid");
    }
    
    /* Secondo fork */
    if ((pid = fork()) < 0) {
        err_quit("Errore seconda fork");
    } else if (pid != 0) {
        exit(0); /* Termina primo figlio */
    }
    
    /* Cambia directory di lavoro */
    if (chdir("/") < 0) {
        err_quit("Errore chdir");
    }
    
    /* Chiudi tutti i file descriptor */
    for (i = 0; i < 64; i++) {
        close(i);
    }
    
    /* Reindirizza stdin, stdout, stderr a /dev/null */
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_WRONLY);
}

/*
 * Gestore dei segnali
 */
static void signal_handler(int sig)
{
    switch (sig) {
        case SIGTERM:
        case SIGINT:
            g_daemon_running = 0;
            log_message("Ricevuto segnale di terminazione");
            break;
        case SIGHUP:
            log_message("Ricevuto SIGHUP - ricarico configurazione");
            break;
    }
}

/*
 * Configura i gestori dei segnali
 */
static void setup_signal_handlers(void)
{
    struct sigaction sa;
    
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
    
    /* Ignora SIGPIPE */
    signal(SIGPIPE, SIG_IGN);
}

/*
 * Scansiona ricorsivamente una directory e costruisce lista file
 */
static int scan_directory(const char *dir_path, file_info_t **file_list)
{
    DIR *dp;
    struct dirent *entry;
    struct stat src_stat;
    char src_full_path[MAX_PATH_LEN];
    char dest_full_path[MAX_PATH_LEN];
    file_info_t *new_file;
    int file_count = 0;
    
    if ((dp = opendir(dir_path)) == NULL) {
        return 0;
    }
    
    while ((entry = readdir(dp)) != NULL) {
        /* Salta . e .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        /* Costruisci path completo */
        snprintf(src_full_path, MAX_PATH_LEN, "%s/%s", dir_path, entry->d_name);
        snprintf(dest_full_path, MAX_PATH_LEN, "%s/%s", 
                 g_dest_dir, src_full_path + strlen(g_src_dir));
        
        if (stat(src_full_path, &src_stat) < 0) {
            continue;
        }
        
        if (S_ISDIR(src_stat.st_mode)) {
            /* Directory: scansiona ricorsivamente */
            char dest_dir_path[MAX_PATH_LEN];
            snprintf(dest_dir_path, MAX_PATH_LEN, "%s/%s", 
                     g_dest_dir, src_full_path + strlen(g_src_dir));
            
            /* Crea directory destinazione se non esiste */
            if (mkdir(dest_dir_path, src_stat.st_mode & 0777) != 0 && errno != EEXIST) {
                continue;
            }
            
            /* Ricorsione */
            file_count += scan_directory(src_full_path, file_list);
            
        } else if (S_ISREG(src_stat.st_mode)) {
            /* File regolare: controlla se serve aggiornamento */
            if (need_update(src_full_path, dest_full_path)) {
                /* Aggiungi alla lista */
                new_file = malloc(sizeof(file_info_t));
                if (new_file != NULL) {
                    strncpy(new_file->src_path, src_full_path, MAX_PATH_LEN - 1);
                    strncpy(new_file->dest_path, dest_full_path, MAX_PATH_LEN - 1);
                    new_file->src_path[MAX_PATH_LEN - 1] = '\0';
                    new_file->dest_path[MAX_PATH_LEN - 1] = '\0';
                    new_file->mtime = src_stat.st_mtime;
                    new_file->size = src_stat.st_size;
                    new_file->next = *file_list;
                    *file_list = new_file;
                    file_count++;
                }
            }
        }
    }
    
    closedir(dp);
    return file_count;
}

/*
 * Verifica se un file necessita di aggiornamento
 */
static int need_update(const char *src_file, const char *dest_file)
{
    struct stat src_stat, dest_stat;
    
    /* Se file destinazione non esiste, serve aggiornamento */
    if (stat(dest_file, &dest_stat) < 0) {
        return 1;
    }
    
    /* Ottieni stat del file sorgente */
    if (stat(src_file, &src_stat) < 0) {
        return 0;
    }
    
    /* Confronta tempo di modifica e dimensione */
    if (src_stat.st_mtime > dest_stat.st_mtime || 
        src_stat.st_size != dest_stat.st_size) {
        return 1;
    }
    
    return 0;
}

/*
 * Copia un file dalla sorgente alla destinazione
 */
static int copy_file(const char *src, const char *dest)
{
    int src_fd, dest_fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_written;
    struct stat src_stat;
    
    /* Apri file sorgente */
    if ((src_fd = open(src, O_RDONLY)) < 0) {
        return -1;
    }
    
    /* Ottieni permessi file sorgente */
    if (fstat(src_fd, &src_stat) < 0) {
        close(src_fd);
        return -1;
    }
    
    /* Apri/crea file destinazione */
    if ((dest_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 
                        src_stat.st_mode & 0777)) < 0) {
        close(src_fd);
        return -1;
    }
    
    /* Copia contenuto */
    while ((bytes_read = read(src_fd, buffer, BUFFER_SIZE)) > 0) {
        bytes_written = write(dest_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            close(src_fd);
            close(dest_fd);
            return -1;
        }
    }
    
    close(src_fd);
    close(dest_fd);
    
    /* Preserva timestamp */
    struct timeval times[2];
    times[0].tv_sec = src_stat.st_atime;
    times[0].tv_usec = 0;
    times[1].tv_sec = src_stat.st_mtime;
    times[1].tv_usec = 0;
    utimes(dest, times);
    
    return 0;
}

/*
 * Thread worker per copia file
 */
static void *worker_thread(void *arg)
{
    thread_data_t *data = (thread_data_t *)arg;
    
    if (copy_file(data->file->src_path, data->file->dest_path) == 0) {
        pthread_mutex_lock(&g_list_mutex);
        g_files_copied++;
        pthread_mutex_unlock(&g_list_mutex);
    }
    
    free(data);
    return NULL;
}

/*
 * Esegui mirroring con thread multipli
 */
static void perform_mirror_threaded(file_info_t *file_list)
{
    pthread_t threads[MAX_THREADS];
    thread_data_t *thread_data;
    file_info_t *current_file;
    int active_threads = 0;
    int thread_index = 0;
    
    current_file = file_list;
    
    while (current_file != NULL || active_threads > 0) {
        /* Avvia nuovi thread se ci sono file da processare e slot disponibili */
        while (current_file != NULL && active_threads < MAX_THREADS) {
            thread_data = malloc(sizeof(thread_data_t));
            if (thread_data != NULL) {
                thread_data->file = current_file;
                thread_data->thread_id = thread_index;
                
                if (pthread_create(&threads[thread_index], NULL, 
                                   worker_thread, thread_data) == 0) {
                    active_threads++;
                    thread_index = (thread_index + 1) % MAX_THREADS;
                    current_file = current_file->next;
                } else {
                    free(thread_data);
                    break;
                }
            } else {
                break;
            }
        }
        
        /* Attendi completamento di almeno un thread */
        if (active_threads > 0) {
            for (int i = 0; i < MAX_THREADS; i++) {
                if (pthread_tryjoin_np(threads[i], NULL) == 0) {
                    active_threads--;
                    break;
                }
            }
            
            /* Se tutti i thread sono occupati, attendi */
            if (active_threads == MAX_THREADS) {
                pthread_join(threads[thread_index], NULL);
                active_threads--;
            }
        }
    }
    
    /* Attendi completamento di tutti i thread rimanenti */
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
}

/*
 * Esegui mirroring single-thread
 */
static void perform_mirror_single(file_info_t *file_list)
{
    file_info_t *current_file = file_list;
    
    while (current_file != NULL) {
        if (copy_file(current_file->src_path, current_file->dest_path) == 0) {
            g_files_copied++;
        }
        current_file = current_file->next;
    }
}

/*
 * Libera la lista dei file
 */
static void free_file_list(file_info_t *file_list)
{
    file_info_t *current, *next;
    
    current = file_list;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
}

/*
 * Scrive messaggio nel log di sistema
 */
static void log_message(const char *msg)
{
    time_t now;
    char time_buf[64];
    FILE *log_file;
    
    time(&now);
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    /* Scrivi su file di log */
    if ((log_file = fopen("/tmp/mirror_daemon.log", "a")) != NULL) {
        fprintf(log_file, "[%s] %s\n", time_buf, msg);
        fclose(log_file);
    }
    
    /* Scrivi anche su syslog */
    openlog("mirror_daemon", LOG_PID, LOG_DAEMON);
    syslog(LOG_INFO, "%s", msg);
    closelog();
}

/*
 * Calcola differenza di tempo in secondi
 */
static double get_time_diff(struct timeval *start, struct timeval *end)
{
    return (end->tv_sec - start->tv_sec) + 
           (end->tv_usec - start->tv_usec) / 1000000.0;
}