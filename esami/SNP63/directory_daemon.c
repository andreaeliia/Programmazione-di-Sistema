/*
Creare un daemon che periodicamente effettui una verifica di tutti i file
annidati all'interno di una directory passata come argomento al suo lancio
e verifichi se qualcuno dei file è stato modificato rispetto al momento del
suo lancio, avvisando l'utente se ciò è accaduto.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>  // ← MANCAVA QUESTO INCLUDE

#define DAEMON_NAME "mydaemon"
#define PID_FILE "/var/run/mydaemon.pid"
#define LOG_FILE "/var/log/mydaemon.log"
#define CHECK_TIME 5   // Controlliamo ogni 5 secondi

// Flag globale per terminazione pulita
volatile sig_atomic_t daemon_running = 1;

typedef struct FileSnapshot {  // ← CORRETTO: era struct FileSnapShot
    char* filepath;
    time_t mtime;  // ultima modifica del file
    off_t size;    // Dimensione del file
    struct FileSnapshot *next;  // ← CORRETTO: era FileSnapShot
} FileSnapshot;

// Struttura per raccogliere informazioni sui file
typedef struct {
    FileSnapshot *files;
    int file_count;
    long total_size;
} FileCollection;

// Variabile globale per la collezione iniziale
FileCollection* initial_collection = NULL;

// Inizializza collezione file
FileCollection* init_file_collection() {
    FileCollection* fc = malloc(sizeof(FileCollection));
    if (!fc) return NULL;
    
    fc->files = NULL;       // Lista vuota inizialmente
    fc->file_count = 0;
    fc->total_size = 0;
    return fc;
}

// Crea nuovo nodo FileSnapshot
FileSnapshot* create_file_snapshot(const char *filepath, time_t mtime, off_t size) {
    FileSnapshot *snapshot = malloc(sizeof(FileSnapshot));
    if (!snapshot) return NULL;
    
    // Alloca e copia il filepath
    snapshot->filepath = malloc(strlen(filepath) + 1);
    if (!snapshot->filepath) {
        free(snapshot);
        return NULL;
    }
    strcpy(snapshot->filepath, filepath);
    
    snapshot->mtime = mtime;
    snapshot->size = size;
    snapshot->next = NULL;
    
    return snapshot;
}

// Aggiungi file alla collezione
void add_file_to_collection(FileCollection* fc, const char *filepath, time_t mtime, off_t size) {
    if (!fc) return;
    
    FileSnapshot *new_snapshot = create_file_snapshot(filepath, mtime, size);
    if (!new_snapshot) return;
    
    // Inserisci in testa alla lista
    new_snapshot->next = fc->files;
    fc->files = new_snapshot;
    fc->file_count++;
    fc->total_size += size;
}

// Trova file nella collezione
FileSnapshot* find_file_in_collection(FileCollection* fc, const char *filepath) {
    if (!fc) return NULL;
    
    FileSnapshot *current = fc->files;
    while (current) {
        if (strcmp(current->filepath, filepath) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Libera un singolo FileSnapshot
void free_file_snapshot(FileSnapshot *snapshot) {
    if (snapshot) {
        free(snapshot->filepath);
        free(snapshot);
    }
}

// Libera tutta la collezione
void free_file_collection(FileCollection* fc) {
    if (!fc) return;
    
    FileSnapshot *current = fc->files;
    while (current) {
        FileSnapshot *next = current->next;
        free_file_snapshot(current);
        current = next;
    }
    
    free(fc);
}

// CORRETTO: Controllo se file è modificato
int is_modified(FileSnapshot* old_file, FileSnapshot* new_file) {
    if (old_file->mtime != new_file->mtime || old_file->size != new_file->size) {
        return 1;  // Modificato
    } else {
        return 0;  // Non modificato
    }
}

// Raccoglie tutti i file ricorsivamente
int collect_all_files_recursive(const char* directory_path, FileCollection* fc) {
    DIR* dir;
    struct dirent* entry;
    struct stat entry_stats;
    char full_path[1024];
    
    syslog(LOG_DEBUG, "Esplorando directory: %s", directory_path);
    
    dir = opendir(directory_path);
    if (dir == NULL) {
        syslog(LOG_ERR, "Errore apertura directory %s: %s", directory_path, strerror(errno));
        return -1;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        // Salta . e ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Costruisci path completo
        snprintf(full_path, sizeof(full_path), "%s/%s", directory_path, entry->d_name);
        
        // Ottieni informazioni sul file/directory
        if (stat(full_path, &entry_stats) != 0) {
            syslog(LOG_WARNING, "Errore stat su %s: %s", full_path, strerror(errno));
            continue;
        }
        
        if (S_ISREG(entry_stats.st_mode)) {
            // È un file regolare - aggiungilo alla collezione
            // CORRETTO: Passa mtime e size corretti
            add_file_to_collection(fc, full_path, entry_stats.st_mtime, entry_stats.st_size);
            syslog(LOG_DEBUG, "File trovato: %s (size: %ld, mtime: %ld)", 
                   full_path, entry_stats.st_size, entry_stats.st_mtime);
            
        } else if (S_ISDIR(entry_stats.st_mode)) {
            // È una directory - ricorsione
            syslog(LOG_DEBUG, "Directory trovata: %s (esplorando...)", full_path);
            collect_all_files_recursive(full_path, fc);
        }
    }
    
    closedir(dir);
    return 0;
}

// CORRETTO: Controllo modifiche nella directory
int check_directory_modifications(const char* directory_path, FileCollection* original_fc) {
    FileCollection* current_fc = init_file_collection();
    if (!current_fc) return -1;
    
    // Raccoglie stato attuale
    if (collect_all_files_recursive(directory_path, current_fc) != 0) {
        free_file_collection(current_fc);
        return -1;
    }
    
    int modifications_found = 0;
    
    // 1. Controlla file modificati o cancellati
    FileSnapshot *original_file = original_fc->files;
    while (original_file) {
        FileSnapshot *current_file = find_file_in_collection(current_fc, original_file->filepath);
        
        if (!current_file) {
            // FILE CANCELLATO
            syslog(LOG_NOTICE, "FILE CANCELLATO: %s", original_file->filepath);
            modifications_found = 1;
        } else {
            // FILE ESISTE - controlla modifiche
            if (is_modified(original_file, current_file)) {
                syslog(LOG_NOTICE, "FILE MODIFICATO: %s", original_file->filepath);
                syslog(LOG_INFO, "  Vecchio: size=%ld, mtime=%ld", 
                       original_file->size, original_file->mtime);
                syslog(LOG_INFO, "  Nuovo: size=%ld, mtime=%ld", 
                       current_file->size, current_file->mtime);
                modifications_found = 1;
                
                // Aggiorna i valori nella collezione originale
                original_file->mtime = current_file->mtime;
                original_file->size = current_file->size;
            }
        }
        original_file = original_file->next;
    }
    
    // 2. Controlla file nuovi
    FileSnapshot *current_file = current_fc->files;
    while (current_file) {
        if (!find_file_in_collection(original_fc, current_file->filepath)) {
            // FILE NUOVO
            syslog(LOG_NOTICE, "FILE NUOVO: %s", current_file->filepath);
            modifications_found = 1;
            
            // Aggiungi alla collezione originale
            add_file_to_collection(original_fc, current_file->filepath, 
                                 current_file->mtime, current_file->size);
        }
        current_file = current_file->next;
    }
    
    // Cleanup
    free_file_collection(current_fc);
    
    return modifications_found;
}

// Handler per segnali di terminazione
void signal_handler(int sig) {
    switch (sig) {
        case SIGTERM:
            syslog(LOG_INFO, "Ricevuto SIGTERM - terminazione daemon");
            daemon_running = 0;
            break;
        case SIGINT:
            syslog(LOG_INFO, "Ricevuto SIGINT - terminazione daemon");
            daemon_running = 0;
            break;
        case SIGHUP:
            syslog(LOG_INFO, "Ricevuto SIGHUP - ricarico configurazione");
            break;
        default:
            syslog(LOG_WARNING, "Ricevuto segnale inaspettato: %d", sig);
            break;
    }
}

// Crea file PID per prevenire istanze multiple
int create_pid_file() {
    int pid_fd;
    char pid_str[16];
    
    pid_fd = open(PID_FILE, O_CREAT | O_WRONLY | O_EXCL, 0644);
    if (pid_fd < 0) {
        if (errno == EEXIST) {
            syslog(LOG_ERR, "Daemon già in esecuzione (PID file exists)");
            return -1;
        } else {
            syslog(LOG_ERR, "Impossibile creare PID file: %s", strerror(errno));
            return -1;
        }
    }
    
    snprintf(pid_str, sizeof(pid_str), "%d\n", getpid());
    if (write(pid_fd, pid_str, strlen(pid_str)) != strlen(pid_str)) {
        syslog(LOG_ERR, "Errore scrittura PID file");
        close(pid_fd);
        unlink(PID_FILE);
        return -1;
    }
    
    close(pid_fd);
    syslog(LOG_INFO, "PID file creato: %s (PID: %d)", PID_FILE, getpid());
    return 0;
}

// Rimuovi file PID al termine
void remove_pid_file() {
    if (unlink(PID_FILE) < 0) {
        syslog(LOG_WARNING, "Errore rimozione PID file: %s", strerror(errno));
    } else {
        syslog(LOG_INFO, "PID file rimosso");
    }
}

// Processo di daemonizzazione completo
int daemonize() {
    pid_t pid, sid;
    
    // 1. PRIMO FORK
    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Errore primo fork: %s\n", strerror(errno));
        return -1;
    }
    
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    
    // 2. DIVENTA SESSION LEADER
    sid = setsid();
    if (sid < 0) {
        syslog(LOG_ERR, "Errore setsid: %s", strerror(errno));
        return -1;
    }
    
    // 3. SECONDO FORK
    pid = fork();
    if (pid < 0) {
        syslog(LOG_ERR, "Errore secondo fork: %s", strerror(errno));
        return -1;
    }
    
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }
    
    // 4. CAMBIA WORKING DIRECTORY
    if (chdir("/") < 0) {
        syslog(LOG_ERR, "Errore chdir: %s", strerror(errno));
        return -1;
    }
    
    // 5. IMPOSTA UMASK
    umask(0);
    
    // 6. CHIUDI FILE DESCRIPTOR STANDARD
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    // 7. REINDIRIZZA STANDARD FD a /dev/null
    int null_fd = open("/dev/null", O_RDWR);
    if (null_fd >= 0) {
        dup2(null_fd, STDIN_FILENO);
        dup2(null_fd, STDOUT_FILENO);
        dup2(null_fd, STDERR_FILENO);
        
        if (null_fd > STDERR_FILENO) {
            close(null_fd);
        }
    }
    
    syslog(LOG_INFO, "Daemonizzazione completata - PID: %d", getpid());
    return 0;
}

// Inizializza logging di sistema
void init_logging() {
    openlog(DAEMON_NAME, LOG_PID | LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "=== %s daemon avviato ===", DAEMON_NAME);
    syslog(LOG_INFO, "PID: %d", getpid());
}

// Cleanup risorse al termine
void cleanup_daemon() {
    syslog(LOG_INFO, "Cleanup daemon in corso...");
    
    // Libera collezione file
    if (initial_collection) {
        free_file_collection(initial_collection);
        initial_collection = NULL;
    }
    
    // Rimuovi file PID
    remove_pid_file();
    
    // Chiudi logging
    syslog(LOG_INFO, "=== %s daemon terminato ===", DAEMON_NAME);
    closelog();
}

// Logica principale del daemon
void daemon_main_loop(const char* watch_path) {
    time_t last_heartbeat = time(NULL);
    time_t last_check = time(NULL);
    int heartbeat_interval = 60;
    
    syslog(LOG_INFO, "Daemon main loop avviato per directory: %s", watch_path);
    
    while (daemon_running) {
        time_t current_time = time(NULL);
        
        // HEARTBEAT
        if (current_time - last_heartbeat >= heartbeat_interval) {
            syslog(LOG_INFO, "Daemon heartbeat - monitorando: %s", watch_path);
            last_heartbeat = current_time;
        }
        
        // CONTROLLO MODIFICHE
        if (current_time - last_check >= CHECK_TIME) {
            int modifications = check_directory_modifications(watch_path, initial_collection);
            
            if (modifications > 0) {
                syslog(LOG_ALERT, "ATTENZIONE: Rilevate modifiche in %s", watch_path);
            } else if (modifications == 0) {
                syslog(LOG_DEBUG, "Nessuna modifica in %s", watch_path);
            } else {
                syslog(LOG_ERR, "Errore durante controllo modifiche");
            }
            
            last_check = current_time;
        }
        
        sleep(1);  // Sleep breve per non consumare troppa CPU
    }
    
    syslog(LOG_INFO, "Daemon main loop terminato");
}

int main(int argc, char* argv[]) {
    // Controllo argomenti
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <directory_da_monitorare>\n", argv[0]);
        return 1;
    }
    
    const char* watch_path = argv[1];  // ← CORRETTO: era argv[0]
    
    // Verifica che la directory esista
    struct stat path_stat;
    if (stat(watch_path, &path_stat) != 0) {
        fprintf(stderr, "Errore: directory '%s' non trovata: %s\n", 
                watch_path, strerror(errno));
        return 1;
    }
    
    if (!S_ISDIR(path_stat.st_mode)) {
        fprintf(stderr, "Errore: '%s' non è una directory\n", watch_path);
        return 1;
    }
    
    // Inizializza logging
    init_logging();
    
    // Crea collezione iniziale PRIMA della daemonizzazione
    initial_collection = init_file_collection();
    if (!initial_collection) {
        syslog(LOG_ERR, "Errore inizializzazione collezione file");
        return EXIT_FAILURE;
    }
    
    // Scansione iniziale
    syslog(LOG_INFO, "Scansione iniziale directory: %s", watch_path);
    if (collect_all_files_recursive(watch_path, initial_collection) != 0) {
        syslog(LOG_ERR, "Errore scansione iniziale");
        free_file_collection(initial_collection);
        return EXIT_FAILURE;
    }
    
    syslog(LOG_INFO, "Scansione iniziale completata: %d file trovati", 
           initial_collection->file_count);
    
    // Daemonizza processo
    printf("Avvio daemon %s per monitorare: %s\n", DAEMON_NAME, watch_path);
    
    if (daemonize() < 0) {
        syslog(LOG_ERR, "Errore daemonizzazione");
        return EXIT_FAILURE;
    }
    
    // Crea file PID dopo daemonizzazione
    if (create_pid_file() < 0) {
        return EXIT_FAILURE;
    }
    
    // Installa handler per segnali
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGHUP, signal_handler);
    
    // Registra funzione cleanup
    atexit(cleanup_daemon);
    
    // Avvia logica principale
    daemon_main_loop(watch_path);
    
    return EXIT_SUCCESS;
}