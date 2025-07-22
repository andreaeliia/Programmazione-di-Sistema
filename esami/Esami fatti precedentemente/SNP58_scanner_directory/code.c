/*
 * Scanner directory con due thread:
 * - Thread 1: Visita ricorsivamente directory e registra informazioni nodi
 * - Thread 2: Accumula byte totali evitando doppi conteggi per hard link
 * Standard C90 compatibile
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <pthread.h>
#include <errno.h>

#define MAX_PATH_LEN 4096
#define BUFFER_SIZE 1000
#define MAX_INODES 10000

/* Struttura per informazioni su un nodo del filesystem */
typedef struct {
    char path[MAX_PATH_LEN];
    off_t size;          /* Dimensione in byte */
    nlink_t hard_links;  /* Numero di hard link */
    ino_t inode;         /* Numero inode per evitare doppi conteggi */
    int valid;           /* Flag per indicare se l'entry è valida */
} NodeInfo;

/* Buffer circolare per comunicazione tra thread */
typedef struct {
    NodeInfo nodes[BUFFER_SIZE];
    int write_index;     /* Indice di scrittura */
    int read_index;      /* Indice di lettura */
    int count;           /* Numero elementi nel buffer */
    int finished;        /* Flag per indicare fine scansione */
    pthread_mutex_t mutex;
    pthread_cond_t not_full;   /* Condizione: buffer non pieno */
    pthread_cond_t not_empty;  /* Condizione: buffer non vuoto */
} SharedBuffer;

/* Struttura per tracking inode già visitati */
typedef struct {
    ino_t inodes[MAX_INODES];
    int count;
    pthread_mutex_t mutex;
} InodeTracker;

/* Strutture globali condivise */
SharedBuffer shared_buffer;
InodeTracker inode_tracker;
char *start_directory;
unsigned long long total_bytes;
int scanner_errors = 0;

/* Inizializza buffer condiviso */
void init_shared_buffer() {
    int i;
    
    shared_buffer.write_index = 0;
    shared_buffer.read_index = 0;
    shared_buffer.count = 0;
    shared_buffer.finished = 0;
    
    for (i = 0; i < BUFFER_SIZE; i++) {
        shared_buffer.nodes[i].valid = 0;
    }
    
    pthread_mutex_init(&shared_buffer.mutex, NULL);
    pthread_cond_init(&shared_buffer.not_full, NULL);
    pthread_cond_init(&shared_buffer.not_empty, NULL);
}

/* Inizializza tracker inode */
void init_inode_tracker() {
    inode_tracker.count = 0;
    pthread_mutex_init(&inode_tracker.mutex, NULL);
}

/* Verifica se inode è già stato visitato */
int is_inode_visited(ino_t inode) {
    int i;
    int found = 0;
    
    pthread_mutex_lock(&inode_tracker.mutex);
    
    for (i = 0; i < inode_tracker.count; i++) {
        if (inode_tracker.inodes[i] == inode) {
            found = 1;
            break;
        }
    }
    
    /* Se non trovato e c'è spazio, aggiungilo */
    if (!found && inode_tracker.count < MAX_INODES) {
        inode_tracker.inodes[inode_tracker.count] = inode;
        inode_tracker.count++;
    }
    
    pthread_mutex_unlock(&inode_tracker.mutex);
    
    return found;
}

/* Aggiunge un nodo al buffer condiviso */
void add_node_to_buffer(const NodeInfo *node) {
    pthread_mutex_lock(&shared_buffer.mutex);
    
    /* Attende che ci sia spazio nel buffer */
    while (shared_buffer.count >= BUFFER_SIZE) {
        pthread_cond_wait(&shared_buffer.not_full, &shared_buffer.mutex);
    }
    
    /* Aggiunge il nodo al buffer */
    shared_buffer.nodes[shared_buffer.write_index] = *node;
    shared_buffer.write_index = (shared_buffer.write_index + 1) % BUFFER_SIZE;
    shared_buffer.count++;
    
    /* Segnala che il buffer non è vuoto */
    pthread_cond_signal(&shared_buffer.not_empty);
    
    pthread_mutex_unlock(&shared_buffer.mutex);
}

/* Legge un nodo dal buffer condiviso */
int get_node_from_buffer(NodeInfo *node) {
    int result = 0;
    
    pthread_mutex_lock(&shared_buffer.mutex);
    
    /* Attende che ci sia qualcosa da leggere o che la scansione sia finita */
    while (shared_buffer.count == 0 && !shared_buffer.finished) {
        pthread_cond_wait(&shared_buffer.not_empty, &shared_buffer.mutex);
    }
    
    /* Se c'è qualcosa da leggere */
    if (shared_buffer.count > 0) {
        *node = shared_buffer.nodes[shared_buffer.read_index];
        shared_buffer.read_index = (shared_buffer.read_index + 1) % BUFFER_SIZE;
        shared_buffer.count--;
        result = 1;
        
        /* Segnala che il buffer non è pieno */
        pthread_cond_signal(&shared_buffer.not_full);
    }
    
    pthread_mutex_unlock(&shared_buffer.mutex);
    
    return result;
}

/* Segnala la fine della scansione */
void signal_scan_finished() {
    pthread_mutex_lock(&shared_buffer.mutex);
    shared_buffer.finished = 1;
    pthread_cond_broadcast(&shared_buffer.not_empty);
    pthread_mutex_unlock(&shared_buffer.mutex);
}

/* Scansione ricorsiva di una directory */
void scan_directory_recursive(const char *dir_path) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    char full_path[MAX_PATH_LEN];
    NodeInfo node_info;
    
    /* Apre la directory */
    dir = opendir(dir_path);
    if (dir == NULL) {
        printf("Errore apertura directory %s: %s\n", dir_path, strerror(errno));
        scanner_errors++;
        return;
    }
    
    /* Scorre tutti gli elementi della directory */
    while ((entry = readdir(dir)) != NULL) {
        /* Salta . e .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        /* Costruisce il percorso completo */
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        
        /* Ottiene informazioni sul file */
        if (lstat(full_path, &file_stat) == -1) {
            printf("Errore lstat su %s: %s\n", full_path, strerror(errno));
            scanner_errors++;
            continue;
        }
        
        /* Prepara informazioni del nodo */
        strncpy(node_info.path, full_path, MAX_PATH_LEN - 1);
        node_info.path[MAX_PATH_LEN - 1] = '\0';
        node_info.size = file_stat.st_size;
        node_info.hard_links = file_stat.st_nlink;
        node_info.inode = file_stat.st_ino;
        node_info.valid = 1;
        
        /* Aggiunge al buffer condiviso */
        add_node_to_buffer(&node_info);
        
        printf("Scanner: %s (inode=%lu, size=%ld, links=%d)\n", 
               full_path, (unsigned long)file_stat.st_ino, 
               (long)file_stat.st_size, (int)file_stat.st_nlink);
        
        /* Se è una directory, scansiona ricorsivamente */
        if (S_ISDIR(file_stat.st_mode)) {
            scan_directory_recursive(full_path);
        }
    }
    
    closedir(dir);
}

/* Thread 1: Scansione directory */
void* scanner_thread(void *arg) {
    printf("Thread Scanner: Inizio scansione di %s\n", start_directory);
    
    scan_directory_recursive(start_directory);
    
    /* Segnala la fine della scansione */
    signal_scan_finished();
    
    printf("Thread Scanner: Scansione completata\n");
    return NULL;
}

/* Thread 2: Conteggio byte */
void* counter_thread(void *arg) {
    NodeInfo node;
    unsigned long long nodes_processed = 0;
    unsigned long long nodes_skipped = 0;
    
    printf("Thread Counter: Inizio conteggio\n");
    
    total_bytes = 0;
    
    /* Processa tutti i nodi */
    while (get_node_from_buffer(&node)) {
        nodes_processed++;
        
        /* Verifica se l'inode è già stato conteggiato */
        if (is_inode_visited(node.inode)) {
            printf("Counter: Saltato %s (inode %lu già conteggiato)\n", 
                   node.path, (unsigned long)node.inode);
            nodes_skipped++;
        } else {
            /* Aggiunge i byte al totale */
            total_bytes += node.size;
            printf("Counter: +%ld byte da %s (totale: %llu)\n", 
                   (long)node.size, node.path, total_bytes);
        }
    }
    
    printf("Thread Counter: Completato\n");
    printf("  Nodi processati: %llu\n", nodes_processed);
    printf("  Nodi saltati (hard link): %llu\n", nodes_skipped);
    printf("  Inode unici visitati: %d\n", inode_tracker.count);
    
    return NULL;
}

/* Cleanup risorse */
void cleanup_resources() {
    pthread_mutex_destroy(&shared_buffer.mutex);
    pthread_cond_destroy(&shared_buffer.not_full);
    pthread_cond_destroy(&shared_buffer.not_empty);
    pthread_mutex_destroy(&inode_tracker.mutex);
}

/* Funzione principale */
int main(int argc, char *argv[]) {
    pthread_t scanner_tid, counter_tid;
    int ret1, ret2;
    struct stat dir_stat;
    
    printf("=== SCANNER DIRECTORY CON DUE THREAD ===\n");
    
    /* Verifica argomenti */
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <directory>\n", argv[0]);
        return 1;
    }
    
    start_directory = argv[1];
    
    /* Verifica che la directory esista */
    if (stat(start_directory, &dir_stat) == -1) {
        perror("Errore accesso directory");
        return 1;
    }
    
    if (!S_ISDIR(dir_stat.st_mode)) {
        fprintf(stderr, "Errore: %s non è una directory\n", start_directory);
        return 1;
    }
    
    printf("Directory da scansionare: %s\n\n", start_directory);
    
    /* Inizializza strutture condivise */
    init_shared_buffer();
    init_inode_tracker();
    
    /* Crea i thread */
    ret1 = pthread_create(&scanner_tid, NULL, scanner_thread, NULL);
    if (ret1 != 0) {
        fprintf(stderr, "Errore creazione thread scanner: %s\n", strerror(ret1));
        return 1;
    }
    
    ret2 = pthread_create(&counter_tid, NULL, counter_thread, NULL);
    if (ret2 != 0) {
        fprintf(stderr, "Errore creazione thread counter: %s\n", strerror(ret2));
        return 1;
    }
    
    /* Attende terminazione dei thread */
    pthread_join(scanner_tid, NULL);
    pthread_join(counter_tid, NULL);
    
    /* Stampa risultati finali */
    printf("\n=== RISULTATI FINALI ===\n");
    printf("Byte totali (senza doppi conteggi): %llu\n", total_bytes);
    printf("Errori durante scansione: %d\n", scanner_errors);
    
    /* Converte in unità più leggibili */
    if (total_bytes >= 1024 * 1024 * 1024) {
        printf("Dimensione totale: %.2f GB\n", (double)total_bytes / (1024 * 1024 * 1024));
    } else if (total_bytes >= 1024 * 1024) {
        printf("Dimensione totale: %.2f MB\n", (double)total_bytes / (1024 * 1024));
    } else if (total_bytes >= 1024) {
        printf("Dimensione totale: %.2f KB\n", (double)total_bytes / 1024);
    }
    
    /* Cleanup */
    cleanup_resources();
    
    printf("\nScansione completata.\n");
    return 0;
}