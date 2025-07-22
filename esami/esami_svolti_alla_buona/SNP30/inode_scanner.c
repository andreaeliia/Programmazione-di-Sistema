/*
 * Scanner Inode Ricorsivo per Directory
 * Implementazione C90 compatibile con Linux e Mac
 * Usa libreria apue.h
 */

#include "apue.h"
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

/* Costanti del programma */
#define MAX_PATH_LENGTH 4096
#define INITIAL_INODE_CAPACITY 1000
#define CAPACITY_INCREMENT 500

/* Struttura per memorizzare gli inode */
struct inode_list {
    ino_t *inodes;          /* Array dinamico di inode */
    size_t count;           /* Numero di inode attuali */
    size_t capacity;        /* Capacità corrente dell'array */
};

/* Funzioni per gestione lista inode */
int init_inode_list(struct inode_list *list);
int add_inode(struct inode_list *list, ino_t inode);
void free_inode_list(struct inode_list *list);
int compare_inodes(const void *a, const void *b);
void sort_inodes(struct inode_list *list);
void print_inodes(struct inode_list *list);

/* Funzioni per attraversamento directory */
int scan_directory_recursive(const char *path, struct inode_list *list);
int process_directory_entry(const char *base_path, const char *entry_name, 
                           struct inode_list *list);
int is_regular_file(const char *path);

/* Funzioni di utilità */
void print_usage(const char *program_name);
int validate_directory(const char *path);

/*
 * Inizializza la lista degli inode
 */
int init_inode_list(struct inode_list *list)
{
    list->inodes = (ino_t*)malloc(INITIAL_INODE_CAPACITY * sizeof(ino_t));
    if (list->inodes == NULL) {
        err_ret("malloc failed for inode list");
        return -1;
    }
    
    list->count = 0;
    list->capacity = INITIAL_INODE_CAPACITY;
    
    return 0;
}

/*
 * Aggiunge un inode alla lista, espandendo l'array se necessario
 */
int add_inode(struct inode_list *list, ino_t inode)
{
    ino_t *new_array;
    size_t new_capacity;
    
    /* Controlla se l'array è pieno */
    if (list->count >= list->capacity) {
        /* Espande l'array */
        new_capacity = list->capacity + CAPACITY_INCREMENT;
        new_array = (ino_t*)realloc(list->inodes, new_capacity * sizeof(ino_t));
        
        if (new_array == NULL) {
            err_ret("realloc failed for inode list expansion");
            return -1;
        }
        
        list->inodes = new_array;
        list->capacity = new_capacity;
    }
    
    /* Aggiunge l'inode */
    list->inodes[list->count] = inode;
    list->count++;
    
    return 0;
}

/*
 * Libera la memoria della lista inode
 */
void free_inode_list(struct inode_list *list)
{
    if (list->inodes != NULL) {
        free(list->inodes);
        list->inodes = NULL;
    }
    list->count = 0;
    list->capacity = 0;
}

/*
 * Funzione di confronto per qsort - ordine numerico crescente
 */
int compare_inodes(const void *a, const void *b)
{
    ino_t inode_a = *(const ino_t*)a;
    ino_t inode_b = *(const ino_t*)b;
    
    if (inode_a < inode_b) {
        return -1;
    } else if (inode_a > inode_b) {
        return 1;
    } else {
        return 0;
    }
}

/*
 * Ordina gli inode in ordine numerico crescente
 */
void sort_inodes(struct inode_list *list)
{
    if (list->count > 1) {
        qsort(list->inodes, list->count, sizeof(ino_t), compare_inodes);
    }
}

/*
 * Stampa tutti gli inode della lista su stdout
 */
void print_inodes(struct inode_list *list)
{
    size_t i;
    
    for (i = 0; i < list->count; i++) {
        printf("%lu\n", (unsigned long)list->inodes[i]);
    }
}

/*
 * Controlla se un path è un file regolare
 */
int is_regular_file(const char *path)
{
    struct stat statbuf;
    
    if (stat(path, &statbuf) == -1) {
        err_ret("stat failed for %s", path);
        return 0;
    }
    
    return S_ISREG(statbuf.st_mode);
}

/*
 * Processa una singola entry di directory
 */
int process_directory_entry(const char *base_path, const char *entry_name, 
                           struct inode_list *list)
{
    char full_path[MAX_PATH_LENGTH];
    struct stat statbuf;
    int path_len;
    
    /* Costruisce il path completo */
    path_len = snprintf(full_path, sizeof(full_path), "%s/%s", 
                       base_path, entry_name);
    
    if (path_len >= sizeof(full_path)) {
        err_ret("path too long: %s/%s", base_path, entry_name);
        return -1;
    }
    
    /* Ottiene informazioni sul file/directory */
    if (lstat(full_path, &statbuf) == -1) {
        err_ret("lstat failed for %s", full_path);
        return -1;
    }
    
    /* Se è un file regolare, aggiunge l'inode */
    if (S_ISREG(statbuf.st_mode)) {
        if (add_inode(list, statbuf.st_ino) == -1) {
            return -1;
        }
    }
    /* Se è una directory, scansiona ricorsivamente */
    else if (S_ISDIR(statbuf.st_mode)) {
        if (scan_directory_recursive(full_path, list) == -1) {
            return -1;
        }
    }
    /* Ignora link simbolici, device files, etc. */
    
    return 0;
}

/*
 * Scansiona ricorsivamente una directory e raccoglie gli inode di tutti i file
 */
int scan_directory_recursive(const char *path, struct inode_list *list)
{
    DIR *dirp;
    struct dirent *dp;
    
    /* Apre la directory */
    dirp = opendir(path);
    if (dirp == NULL) {
        err_ret("opendir failed for %s", path);
        return -1;
    }
    
    /* Legge tutte le entry della directory */
    while ((dp = readdir(dirp)) != NULL) {
        /* Salta le entry speciali "." e ".." */
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
            continue;
        }
        
        /* Processa l'entry */
        if (process_directory_entry(path, dp->d_name, list) == -1) {
            closedir(dirp);
            return -1;
        }
    }
    
    /* Chiude la directory */
    if (closedir(dirp) == -1) {
        err_ret("closedir failed for %s", path);
        return -1;
    }
    
    return 0;
}

/*
 * Valida che il path sia una directory esistente
 */
int validate_directory(const char *path)
{
    struct stat statbuf;
    
    /* Controlla che il path esista */
    if (stat(path, &statbuf) == -1) {
        err_ret("stat failed for %s", path);
        return -1;
    }
    
    /* Controlla che sia una directory */
    if (!S_ISDIR(statbuf.st_mode)) {
        err_quit("%s is not a directory", path);
        return -1;
    }
    
    return 0;
}

/*
 * Stampa le istruzioni d'uso del programma
 */
void print_usage(const char *program_name)
{
    printf("Uso: %s <directory_path>\n", program_name);
    printf("\n");
    printf("Scansiona ricorsivamente la directory specificata e stampa\n");
    printf("in ordine numerico crescente tutti i numeri di inode dei\n");
    printf("file regolari presenti nel sottoalbero.\n");
    printf("\n");
    printf("Argomenti:\n");
    printf("  directory_path    Path della directory da scansionare\n");
    printf("\n");
    printf("Esempi:\n");
    printf("  %s /home/user/documents\n", program_name);
    printf("  %s .\n", program_name);
    printf("  %s /tmp\n", program_name);
}

/*
 * Funzione principale
 */
int main(int argc, char *argv[])
{
    struct inode_list inode_list;
    char *directory_path;
    
    /* Controlla il numero di argomenti */
    if (argc != 2) {
        print_usage(argv[0]);
        exit(1);
    }
    
    directory_path = argv[1];
    
    /* Valida la directory */
    if (validate_directory(directory_path) == -1) {
        exit(1);
    }
    
    /* Inizializza la lista degli inode */
    if (init_inode_list(&inode_list) == -1) {
        err_sys("failed to initialize inode list");
    }
    
    printf("Scansione directory: %s\n", directory_path);
    
    /* Scansiona ricorsivamente la directory */
    if (scan_directory_recursive(directory_path, &inode_list) == -1) {
        free_inode_list(&inode_list);
        err_sys("failed to scan directory");
    }
    
    printf("Trovati %zu file regolari\n", inode_list.count);
    
    /* Ordina gli inode */
    sort_inodes(&inode_list);
    
    printf("Inode in ordine numerico crescente:\n");
    printf("=====================================\n");
    
    /* Stampa gli inode ordinati */
    print_inodes(&inode_list);
    
    /* Pulizia memoria */
    free_inode_list(&inode_list);
    
    printf("=====================================\n");
    printf("Scansione completata con successo\n");
    
    return 0;
}