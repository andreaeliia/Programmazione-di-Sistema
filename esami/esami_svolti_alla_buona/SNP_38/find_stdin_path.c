#include "apue.h"
#include <sys/stat.h>
#include <fcntl.h>

#ifdef LINUX
    #include <unistd.h>
#elif MACOS
    #include <sys/syslimits.h>
    #include <libproc.h>
#endif

#define BUFFER_SIZE 1024
#define PATH_MAX_SIZE 4096

/*
 * Funzione per determinare il path del file associato allo stdin
 * Utilizza approcci diversi per Linux e macOS
 * Ritorna 0 in caso di successo, -1 in caso di errore
 */
int get_stdin_path(char *path_buffer, size_t buffer_size) {
    struct stat stdin_stat, file_stat;
    
    /* Verifica se stdin e' associato a un file regolare */
    if (fstat(STDIN_FILENO, &stdin_stat) < 0) {
        err_sys("fstat error on stdin");
        return -1;
    }
    
    /* Se stdin non e' un file regolare, non possiamo determinare il path */
    if (!S_ISREG(stdin_stat.st_mode)) {
        printf("Standard input non e' associato a un file regolare\n");
        return -1;
    }
    
#ifdef LINUX
    /* Su Linux, utilizziamo il link simbolico in /proc/self/fd/0 */
    ssize_t len = readlink("/proc/self/fd/0", path_buffer, buffer_size - 1);
    if (len == -1) {
        err_sys("readlink error");
        return -1;
    }
    path_buffer[len] = '\0';
    
#elif MACOS
    /* Su macOS, utilizziamo proc_pidpath per ottenere il path */
    pid_t pid = getpid();
    int ret = proc_pidpath(pid, path_buffer, buffer_size);
    if (ret <= 0) {
        /* Se proc_pidpath fallisce, proviamo con fcntl */
        if (fcntl(STDIN_FILENO, F_GETPATH, path_buffer) == -1) {
            err_sys("Cannot determine stdin path");
            return -1;
        }
    }
#endif
    
    return 0;
}

/*
 * Funzione per leggere e processare i dati dallo standard input
 * Conta il numero di righe, caratteri e parole lette
 */
void process_stdin_data(void) {
    char buffer[BUFFER_SIZE];
    int lines = 0, chars = 0, words = 0;
    int in_word = 0;
    ssize_t n;
    
    printf("\nLettura dati dallo standard input...\n");
    printf("(Premere Ctrl+D per terminare l'input)\n\n");
    
    /* Legge i dati dallo stdin e li processa */
    while ((n = read(STDIN_FILENO, buffer, BUFFER_SIZE - 1)) > 0) {
        int i;
        buffer[n] = '\0';
        
        /* Conta caratteri, parole e righe */
        for (i = 0; i < n; i++) {
            chars++;
            
            if (buffer[i] == '\n') {
                lines++;
            }
            
            if (buffer[i] == ' ' || buffer[i] == '\t' || buffer[i] == '\n') {
                in_word = 0;
            } else if (!in_word) {
                in_word = 1;
                words++;
            }
        }
        
        /* Stampa il contenuto letto */
        printf("%s", buffer);
    }
    
    if (n < 0) {
        err_sys("read error");
    }
    
    /* Stampa le statistiche */
    printf("\n\n--- Statistiche input ---\n");
    printf("Righe: %d\n", lines);
    printf("Parole: %d\n", words);
    printf("Caratteri: %d\n", chars);
}

/*
 * Funzione principale che coordina le operazioni
 */
int main(void) {
    char stdin_path[PATH_MAX_SIZE];
    
    printf("=== Determinazione automatica del path dello stdin ===\n\n");
    
    /* Tenta di determinare il path del file associato allo stdin */
    if (get_stdin_path(stdin_path, sizeof(stdin_path)) == 0) {
        printf("Path del file associato allo standard input:\n");
        printf("%s\n", stdin_path);
    } else {
        printf("Impossibile determinare il path dello standard input\n");
        printf("Lo standard input potrebbe essere associato a:\n");
        printf("- Un terminale\n");
        printf("- Una pipe\n");
        printf("- Un socket\n");
        printf("- Un dispositivo speciale\n");
    }
    
    /* Processa i dati dallo stdin indipendentemente dal risultato precedente */
    process_stdin_data();
    
    printf("\nProgramma terminato.\n");
    
    return 0;
}