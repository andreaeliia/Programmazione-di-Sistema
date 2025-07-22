#include "apue.h"
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_PATH 4096
#define MAX_FILES 1000

/* Struttura per condividere dati tra i thread */
struct shared_data {
    char files_with_links[MAX_FILES][MAX_PATH]; /* Array dei file con link count > 1 */
    ino_t inodes[MAX_FILES];                    /* Array degli inode corrispondenti */
    int file_count;                             /* Numero di file trovati */
    int thread1_done;                           /* Flag per indicare che thread1 ha finito */
    pthread_mutex_t mutex;                      /* Mutex per sincronizzazione */
    pthread_cond_t cond;                        /* Condition variable per sincronizzazione */
    char search_dir[MAX_PATH];                  /* Directory di ricerca */
};

static struct shared_data shared;

/* Funzione per verificare se un percorso e' un file regolare */
static int is_regular_file(const char *path)
{
    struct stat st;
    
    if (lstat(path, &st) == -1)
        return 0;
    
    return S_ISREG(st.st_mode);
}

/* Funzione per attraversare ricorsivamente una directory */
static void traverse_directory(const char *dir_path, void (*process_file)(const char *, const struct stat *))
{
    DIR *dir;
    struct dirent *entry;
    char full_path[MAX_PATH];
    struct stat st;
    
    if ((dir = opendir(dir_path)) == NULL) {
        err_msg("opendir failed for %s", dir_path);
        return;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        /* Salta le directory . e .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        
        /* Costruisce il percorso completo */
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        
        if (lstat(full_path, &st) == -1) {
            err_msg("lstat failed for %s", full_path);
            continue;
        }
        
        /* Se e' un file regolare, lo processa */
        if (S_ISREG(st.st_mode)) {
            process_file(full_path, &st);
        }
        /* Se e' una directory, la attraversa ricorsivamente */
        else if (S_ISDIR(st.st_mode)) {
            traverse_directory(full_path, process_file);
        }
    }
    
    if (closedir(dir) == -1)
        err_msg("closedir failed");
}

/* Funzione per processare un file nel thread1 */
static void process_file_thread1(const char *file_path, const struct stat *st)
{
    /* Verifica se il file ha link count > 1 */
    if (st->st_nlink > 1) {
        pthread_mutex_lock(&shared.mutex);
        
        /* Aggiunge il file all'array condiviso se c'e' spazio */
        if (shared.file_count < MAX_FILES) {
            strncpy(shared.files_with_links[shared.file_count], file_path, MAX_PATH - 1);
            shared.files_with_links[shared.file_count][MAX_PATH - 1] = '\0';
            shared.inodes[shared.file_count] = st->st_ino;
            shared.file_count++;
            
            printf("Thread1: Trovato file con link count %ld: %s (inode: %ld)\n", 
                   (long)st->st_nlink, file_path, (long)st->st_ino);
            
            /* Segnala al thread2 che c'e' un nuovo file */
            pthread_cond_signal(&shared.cond);
        }
        
        pthread_mutex_unlock(&shared.mutex);
    }
}

/* Funzione per processare un file nel thread2 */
static void process_file_thread2(const char *file_path, const struct stat *st, ino_t target_inode)
{
    /* Verifica se l'inode corrisponde a quello cercato */
    if (st->st_ino == target_inode) {
        printf("Thread2: Trovato hard link: %s (inode: %ld)\n", 
               file_path, (long)st->st_ino);
    }
}

/* Thread1: trova i file con link count > 1 */
static void *thread1_func(void *arg)
{
    printf("Thread1: Inizio ricerca file con link count > 1 in %s\n", shared.search_dir);
    
    /* Attraversa la directory e cerca i file con link count > 1 */
    traverse_directory(shared.search_dir, process_file_thread1);
    
    pthread_mutex_lock(&shared.mutex);
    shared.thread1_done = 1;
    pthread_cond_broadcast(&shared.cond); /* Sveglia il thread2 */
    pthread_mutex_unlock(&shared.mutex);
    
    printf("Thread1: Ricerca completata. Trovati %d file con link count > 1\n", shared.file_count);
    return NULL;
}

/* Funzione helper per il thread2 per cercare hard link di un inode specifico */
static void find_hardlinks_for_inode(const char *dir_path, ino_t target_inode)
{
    DIR *dir;
    struct dirent *entry;
    char full_path[MAX_PATH];
    struct stat st;
    
    if ((dir = opendir(dir_path)) == NULL) {
        err_msg("opendir failed for %s", dir_path);
        return;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        /* Salta le directory . e .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        
        /* Costruisce il percorso completo */
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        
        if (lstat(full_path, &st) == -1) {
            err_msg("lstat failed for %s", full_path);
            continue;
        }
        
        /* Se e' un file regolare, verifica l'inode */
        if (S_ISREG(st.st_mode)) {
            process_file_thread2(full_path, &st, target_inode);
        }
        /* Se e' una directory, la attraversa ricorsivamente */
        else if (S_ISDIR(st.st_mode)) {
            find_hardlinks_for_inode(full_path, target_inode);
        }
    }
    
    if (closedir(dir) == -1)
        err_msg("closedir failed");
}

/* Thread2: trova tutti gli hard link per ogni file trovato dal thread1 */
static void *thread2_func(void *arg)
{
    int processed_files = 0;
    
    printf("Thread2: In attesa dei file dal Thread1...\n");
    
    while (1) {
        pthread_mutex_lock(&shared.mutex);
        
        /* Aspetta che ci siano nuovi file da processare o che thread1 abbia finito */
        while (processed_files >= shared.file_count && !shared.thread1_done) {
            pthread_cond_wait(&shared.cond, &shared.mutex);
        }
        
        /* Se thread1 ha finito e non ci sono piu' file da processare, esce */
        if (shared.thread1_done && processed_files >= shared.file_count) {
            pthread_mutex_unlock(&shared.mutex);
            break;
        }
        
        /* Processa i nuovi file */
        while (processed_files < shared.file_count) {
            ino_t current_inode = shared.inodes[processed_files];
            char current_file[MAX_PATH];
            strncpy(current_file, shared.files_with_links[processed_files], MAX_PATH);
            processed_files++;
            
            pthread_mutex_unlock(&shared.mutex);
            
            printf("Thread2: Ricerca hard link per inode %ld\n", (long)current_inode);
            find_hardlinks_for_inode(shared.search_dir, current_inode);
            
            pthread_mutex_lock(&shared.mutex);
        }
        
        pthread_mutex_unlock(&shared.mutex);
    }
    
    printf("Thread2: Ricerca hard link completata\n");
    return NULL;
}

/* Funzione di inizializzazione delle strutture condivise */
static void init_shared_data(const char *directory)
{
    shared.file_count = 0;
    shared.thread1_done = 0;
    strncpy(shared.search_dir, directory, MAX_PATH - 1);
    shared.search_dir[MAX_PATH - 1] = '\0';
    
    if (pthread_mutex_init(&shared.mutex, NULL) != 0)
        err_sys("pthread_mutex_init failed");
    
    if (pthread_cond_init(&shared.cond, NULL) != 0)
        err_sys("pthread_cond_init failed");
}

/* Funzione di cleanup delle strutture condivise */
static void cleanup_shared_data(void)
{
    pthread_mutex_destroy(&shared.mutex);
    pthread_cond_destroy(&shared.cond);
}

/* Funzione principale */
int main(int argc, char *argv[])
{
    pthread_t thread1, thread2;
    struct stat st;
    
    /* Verifica gli argomenti */
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <directory_path>\n", argv[0]);
        exit(1);
    }
    
    /* Verifica che il percorso sia una directory esistente */
    if (stat(argv[1], &st) == -1)
        err_sys("stat failed for %s", argv[1]);
    
    if (!S_ISDIR(st.st_mode))
        err_quit("%s non e' una directory", argv[1]);
    
    printf("Avvio ricerca hard link nella directory: %s\n", argv[1]);
    
    /* Inizializza le strutture condivise */
    init_shared_data(argv[1]);
    
    /* Crea i thread */
    if (pthread_create(&thread1, NULL, thread1_func, NULL) != 0)
        err_sys("pthread_create failed for thread1");
    
    if (pthread_create(&thread2, NULL, thread2_func, NULL) != 0)
        err_sys("pthread_create failed for thread2");
    
    /* Aspetta che i thread terminino */
    if (pthread_join(thread1, NULL) != 0)
        err_sys("pthread_join failed for thread1");
    
    if (pthread_join(thread2, NULL) != 0)
        err_sys("pthread_join failed for thread2");
    
    /* Cleanup */
    cleanup_shared_data();
    
    printf("Programma terminato con successo\n");
    return 0;
}
