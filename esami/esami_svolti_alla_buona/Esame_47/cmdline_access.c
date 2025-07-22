/*
 * cmdline_access.c
 * 
 * Programma che dimostra come una funzione può accedere agli argomenti
 * della linea di comando senza che argc e argv vengano passati come
 * parametri o copiati in variabili globali.
 * 
 * La soluzione utilizza il file system /proc sotto Linux e tecniche
 * di accesso alla memoria del processo per recuperare le informazioni
 * degli argomenti dalla command line originale.
 * 
 * Compilazione: utilizzare il Makefile fornito
 * Esecuzione: ./cmdline_access arg1 arg2 arg3 ...
 */

#include "apue.h"
#include <string.h>
#include <ctype.h>

#ifdef LINUX
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#endif

#ifdef MACOS
#include <sys/sysctl.h>
#include <libproc.h>
#endif

#define MAX_ARGS 256
#define MAX_ARG_LEN 1024
#define MAX_CMDLINE 4096

/* Struttura per contenere gli argomenti recuperati */
typedef struct {
    int argc;
    char argv[MAX_ARGS][MAX_ARG_LEN];
} cmdline_args_t;

/* Prototipi delle funzioni */
static int get_cmdline_args_linux(cmdline_args_t *args);
static int get_cmdline_args_macos(cmdline_args_t *args);
static int parse_cmdline_buffer(char *buffer, int len, cmdline_args_t *args);
static void print_cmdline_args(const cmdline_args_t *args);
static void demonstrate_access_without_params(void);
static void analyze_arguments(const cmdline_args_t *args);

/*
 * Funzione principale
 * Non passa argc e argv alla funzione di dimostrazione
 */
int main(int argc, char *argv[])
{
    printf("=== Dimostrazione accesso argomenti command line ===\n");
    printf("Programma avviato con %d argomenti:\n", argc);
    
    /* Mostra gli argomenti ricevuti normalmente */
    int i;
    for (i = 0; i < argc; i++) {
        printf("  argv[%d] = \"%s\"\n", i, argv[i]);
    }
    
    printf("\n--- Ora la funzione accederà agli argomenti SENZA riceverli ---\n\n");
    
    /* Chiama la funzione che deve recuperare gli argomenti da sola */
    demonstrate_access_without_params();
    
    return 0;
}

/*
 * Funzione che accede agli argomenti della command line
 * SENZA ricevere argc e argv come parametri
 */
static void demonstrate_access_without_params(void)
{
    cmdline_args_t args;
    int success = 0;
    
    printf("Funzione chiamata SENZA argc e argv...\n");
    printf("Tentativo di recupero argomenti tramite sistema operativo:\n\n");
    
    /* Inizializza la struttura */
    memset(&args, 0, sizeof(args));
    
#ifdef LINUX
    printf("Sistema: Linux - Usando /proc/self/cmdline\n");
    success = get_cmdline_args_linux(&args);
#endif

#ifdef MACOS
    printf("Sistema: macOS - Usando sysctl e libproc\n");
    success = get_cmdline_args_macos(&args);
#endif

    if (success) {
        printf("✓ Successo! Argomenti recuperati:\n\n");
        print_cmdline_args(&args);
        printf("\n");
        analyze_arguments(&args);
    } else {
        printf("✗ Errore nel recupero degli argomenti\n");
    }
}

#ifdef LINUX
/*
 * Recupera gli argomenti su Linux tramite /proc/self/cmdline
 * Ogni argomento è separato da un byte null
 */
static int get_cmdline_args_linux(cmdline_args_t *args)
{
    int fd;
    char buffer[MAX_CMDLINE];
    ssize_t bytes_read;
    
    printf("Apertura /proc/self/cmdline...\n");
    
    fd = open("/proc/self/cmdline", O_RDONLY);
    if (fd == -1) {
        err_msg("Errore apertura /proc/self/cmdline");
        return 0;
    }
    
    bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);
    
    if (bytes_read <= 0) {
        err_msg("Errore lettura /proc/self/cmdline");
        return 0;
    }
    
    printf("Letti %ld bytes da /proc/self/cmdline\n", (long)bytes_read);
    
    return parse_cmdline_buffer(buffer, (int)bytes_read, args);
}
#endif

#ifdef MACOS
/*
 * Recupera gli argomenti su macOS tramite sysctl
 * Utilizza KERN_PROCARGS2 per ottenere gli argomenti del processo
 */
static int get_cmdline_args_macos(cmdline_args_t *args)
{
    int mib[4];
    size_t size;
    char *buffer;
    int argc_from_kernel;
    char *cp;
    int i;
    
    printf("Utilizzando sysctl KERN_PROCARGS2...\n");
    
    /* Ottiene il PID corrente */
    pid_t pid = getpid();
    
    /* Imposta i parametri per sysctl */
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROCARGS2;
    mib[2] = pid;
    
    /* Prima chiamata per ottenere la dimensione necessaria */
    if (sysctl(mib, 3, NULL, &size, NULL, 0) == -1) {
        err_msg("Errore sysctl size");
        return 0;
    }
    
    /* Alloca buffer */
    buffer = malloc(size);
    if (buffer == NULL) {
        err_msg("Errore malloc");
        return 0;
    }
    
    /* Seconda chiamata per ottenere i dati */
    if (sysctl(mib, 3, buffer, &size, NULL, 0) == -1) {
        err_msg("Errore sysctl data");
        free(buffer);
        return 0;
    }
    
    printf("Ricevuti %zu bytes da sysctl\n", size);
    
    /* Estrae argc dai primi 4 bytes */
    memcpy(&argc_from_kernel, buffer, sizeof(int));
    printf("argc dal kernel: %d\n", argc_from_kernel);
    
    /* Salta argc e il nome dell'eseguibile */
    cp = buffer + sizeof(int);
    
    /* Salta il path dell'eseguibile */
    cp += strlen(cp) + 1;
    
    /* Salta eventuali byte null padding */
    while (cp < buffer + size && *cp == '\0') {
        cp++;
    }
    
    /* Estrae gli argomenti */
    args->argc = 0;
    for (i = 0; i < argc_from_kernel && cp < buffer + size && args->argc < MAX_ARGS; i++) {
        if (*cp != '\0') {
            strncpy(args->argv[args->argc], cp, MAX_ARG_LEN - 1);
            args->argv[args->argc][MAX_ARG_LEN - 1] = '\0';
            args->argc++;
            cp += strlen(cp);
        }
        cp++; /* Salta il terminatore null */
    }
    
    free(buffer);
    return 1;
}
#endif

/*
 * Analizza il buffer contenente la command line e estrae gli argomenti
 * Su Linux gli argomenti sono separati da byte null
 */
static int parse_cmdline_buffer(char *buffer, int len, cmdline_args_t *args)
{
    int i, arg_start;
    
    printf("Analisi buffer di %d bytes...\n", len);
    
    args->argc = 0;
    arg_start = 0;
    
    for (i = 0; i <= len && args->argc < MAX_ARGS; i++) {
        /* Trova la fine dell'argomento (byte null o fine buffer) */
        if (i == len || buffer[i] == '\0') {
            if (i > arg_start) {
                /* Copia l'argomento */
                int arg_len = i - arg_start;
                if (arg_len >= MAX_ARG_LEN) {
                    arg_len = MAX_ARG_LEN - 1;
                }
                memcpy(args->argv[args->argc], &buffer[arg_start], arg_len);
                args->argv[args->argc][arg_len] = '\0';
                args->argc++;
            }
            arg_start = i + 1;
        }
    }
    
    printf("Estratti %d argomenti\n", args->argc);
    return (args->argc > 0);
}

/*
 * Stampa gli argomenti recuperati
 */
static void print_cmdline_args(const cmdline_args_t *args)
{
    int i;
    
    printf("Argomenti recuperati dalla funzione (argc = %d):\n", args->argc);
    for (i = 0; i < args->argc; i++) {
        printf("  [%d] = \"%s\"\n", i, args->argv[i]);
    }
}

/*
 * Analizza gli argomenti recuperati e fornisce statistiche
 */
static void analyze_arguments(const cmdline_args_t *args)
{
    int i, total_chars = 0, numeric_args = 0;
    int max_len = 0, min_len = INT_MAX;
    
    printf("=== ANALISI ARGOMENTI ===\n");
    
    if (args->argc == 0) {
        printf("Nessun argomento da analizzare\n");
        return;
    }
    
    /* Calcola statistiche */
    for (i = 0; i < args->argc; i++) {
        int len = strlen(args->argv[i]);
        total_chars += len;
        
        if (len > max_len) max_len = len;
        if (len < min_len) min_len = len;
        
        /* Verifica se l'argomento è numerico */
        if (i > 0) { /* Salta argv[0] che è il nome del programma */
            char *p = args->argv[i];
            int is_numeric = 1;
            
            if (*p == '-') p++; /* Salta il segno negativo */
            
            while (*p) {
                if (!isdigit(*p) && *p != '.') {
                    is_numeric = 0;
                    break;
                }
                p++;
            }
            
            if (is_numeric && strlen(args->argv[i]) > 0) {
                numeric_args++;
            }
        }
    }
    
    printf("Numero totale di argomenti: %d\n", args->argc);
    printf("Nome programma: %s\n", args->argc > 0 ? args->argv[0] : "N/A");
    printf("Argomenti utente: %d\n", args->argc > 1 ? args->argc - 1 : 0);
    printf("Caratteri totali: %d\n", total_chars);
    printf("Lunghezza argomento più lungo: %d\n", max_len);
    printf("Lunghezza argomento più corto: %d\n", min_len > max_len ? 0 : min_len);
    printf("Argomenti numerici: %d\n", numeric_args);
    
    if (args->argc > 1) {
        printf("Media caratteri per argomento: %.2f\n", 
               (float)total_chars / args->argc);
    }
}