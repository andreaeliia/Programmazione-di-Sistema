/*
 * Esercizi APUE - Gestione I/O e Segnali Unix
 * 
 * Esercizio 1: Analisi comportamento ls con pipe
 * Esercizio 2: Spiegazione meccanismo nohup
 * 
 * Compilazione: make esercizi_apue
 */

#include "apue.h"
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>

/* Prototipi delle funzioni */
void esercizio1_demo_ls_pipe(void);
void test_output_type(void);
void esercizio2_demo_nohup(void);
void sighup_handler(int sig);
void print_menu(void);

/* Variabile globale per il gestore del segnale */
static volatile sig_atomic_t sighup_ricevuto = 0;

int main(void)
{
    int scelta;
    
    printf("=== ESERCIZI APUE - I/O E SEGNALI ===\n\n");
    
    while (1) {
        print_menu();
        
        if (scanf("%d", &scelta) != 1) {
            /* Pulizia buffer in caso di input non valido */
            while (getchar() != '\n');
            printf("Input non valido. Riprovare.\n\n");
            continue;
        }
        
        switch (scelta) {
            case 1:
                esercizio1_demo_ls_pipe();
                break;
            case 2:
                esercizio2_demo_nohup();
                break;
            case 0:
                printf("Programma terminato.\n");
                exit(0);
            default:
                printf("Scelta non valida. Riprovare.\n\n");
        }
    }
    
    return 0;
}

/*
 * Stampa il menu principale
 */
void print_menu(void)
{
    printf("Seleziona un esercizio:\n");
    printf("1. Dimostrazione comportamento ls con pipe\n");
    printf("2. Dimostrazione meccanismo nohup\n");
    printf("0. Esci\n");
    printf("Scelta: ");
}

/*
 * ESERCIZIO 1: Dimostrazione del comportamento di ls con pipe
 * 
 * ls usa implicitamente l'opzione -1 quando rileva che il suo output
 * non va verso un terminale ma verso una pipe o file.
 * Questo viene fatto controllando se stdout e' un terminale con isatty().
 */
void esercizio1_demo_ls_pipe(void)
{
    printf("\n=== ESERCIZIO 1: COMPORTAMENTO LS CON PIPE ===\n\n");
    
    printf("SPIEGAZIONE:\n");
    printf("Il comando 'ls' controlla automaticamente se il suo output\n");
    printf("e' diretto verso un terminale o verso una pipe/file.\n");
    printf("Usa la funzione isatty() per questo controllo.\n\n");
    
    printf("- Se output va al terminale: formato multi-colonna\n");
    printf("- Se output va a pipe/file: formato una-per-riga (-1)\n\n");
    
    /* Dimostrazione pratica del controllo del tipo di output */
    test_output_type();
    
    printf("\nDIMOSTRAZIONE PRATICA:\n");
    printf("Eseguendo 'ls' normale vs 'ls | cat':\n\n");
    
    /* Esecuzione di ls normale */
    printf("1. Output di 'ls' diretto al terminale:\n");
    fflush(stdout);
    system("ls");
    
    printf("\n2. Output di 'ls | cat' (attraverso pipe):\n");
    fflush(stdout);
    system("ls | cat");
    
    printf("\nCome si vede, nel secondo caso ogni file e' su una riga separata.\n");
    printf("Questo perche' ls rileva la pipe e usa automaticamente -1.\n\n");
}

/*
 * Funzione che dimostra come controllare il tipo di output
 * usando isatty() come fa ls internamente
 */
void test_output_type(void)
{
    printf("CONTROLLO TIPO OUTPUT (come fa ls internamente):\n");
    
    if (isatty(STDOUT_FILENO)) {
        printf("- stdout e' connesso a un terminale\n");
        printf("- ls userebbe formato multi-colonna\n");
    } else {
        printf("- stdout NON e' un terminale (pipe/file)\n");
        printf("- ls userebbe formato una-per-riga (-1)\n");
    }
    
    printf("- File descriptor stdout: %d\n", STDOUT_FILENO);
    printf("- isatty(stdout) restituisce: %s\n", 
           isatty(STDOUT_FILENO) ? "TRUE" : "FALSE");
}

/*
 * ESERCIZIO 2: Dimostrazione del meccanismo nohup
 * 
 * nohup previene che un processo riceva SIGHUP quando il terminale
 * che lo ha lanciato viene chiuso. Inoltre redirge stdout/stderr.
 */
void esercizio2_demo_nohup(void)
{
    pid_t pid;
    int status;
    
    printf("\n=== ESERCIZIO 2: MECCANISMO NOHUP ===\n\n");
    
    printf("SPIEGAZIONE DEL MECCANISMO NOHUP:\n\n");
    
    printf("1. SENZA nohup:\n");
    printf("   - Processo figlio riceve SIGHUP quando bash termina\n");
    printf("   - Default: processo termina\n");
    printf("   - stdout/stderr restano collegati al terminale\n\n");
    
    printf("2. CON nohup:\n");
    printf("   - nohup ignora SIGHUP (signal(SIGHUP, SIG_IGN))\n");
    printf("   - Redirge stdout verso nohup.out se e' un terminale\n");
    printf("   - Redirge stderr verso stdout\n");
    printf("   - Processo continua anche se terminale chiude\n\n");
    
    /* Installazione gestore per SIGHUP */
    if (signal(SIGHUP, sighup_handler) == SIG_ERR) {
        err_sys("errore nell'installare il gestore SIGHUP");
    }
    
    printf("DIMOSTRAZIONE:\n");
    printf("Creazione processo figlio che simula comportamento con/senza nohup...\n\n");
    
    if ((pid = fork()) < 0) {
        err_sys("errore fork");
    } else if (pid == 0) {
        /* Processo figlio */
        printf("[FIGLIO] PID: %ld\n", (long)getpid());
        printf("[FIGLIO] Attendo segnali...\n");
        
        /* Simula un processo che lavora */
        int i;
        for (i = 0; i < 10; i++) {
            printf("[FIGLIO] Iterazione %d - ancora in esecuzione\n", i + 1);
            sleep(1);
            
            if (sighup_ricevuto) {
                printf("[FIGLIO] SIGHUP ricevuto! Terminazione...\n");
                exit(1);
            }
        }
        
        printf("[FIGLIO] Completato senza interruzioni\n");
        exit(0);
    } else {
        /* Processo padre */
        printf("[PADRE] Processo figlio creato con PID: %ld\n", (long)pid);
        printf("[PADRE] Attendo 3 secondi poi invio SIGHUP...\n");
        
        sleep(3);
        
        printf("[PADRE] Invio SIGHUP al figlio...\n");
        if (kill(pid, SIGHUP) < 0) {
            err_sys("errore kill");
        }
        
        /* Attende terminazione figlio */
        if (waitpid(pid, &status, 0) < 0) {
            err_sys("errore waitpid");
        }
        
        if (WIFEXITED(status)) {
            printf("[PADRE] Figlio terminato con status: %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("[PADRE] Figlio terminato da segnale: %d\n", WTERMSIG(status));
        }
    }
    
    printf("\nCOME FUNZIONA NOHUP NEL DETTAGLIO:\n");
    printf("1. nohup esegue: signal(SIGHUP, SIG_IGN)\n");
    printf("2. Se stdout e' un terminale: redirge a nohup.out\n");
    printf("3. Se stderr e' un terminale: redirge a stdout\n");
    printf("4. Esegue il comando richiesto\n");
    printf("5. Il processo ignora SIGHUP e continua\n\n");
}

/*
 * Gestore del segnale SIGHUP
 */
void sighup_handler(int sig)
{
    /* Gestore semplice e signal-safe */
    sighup_ricevuto = 1;
    
    /* 
     * Nota: printf() non e' signal-safe, ma per scopi dimostrativi
     * In codice reale si dovrebbe usare write() o funzioni signal-safe
     */
    write(STDOUT_FILENO, "[SEGNALE] SIGHUP ricevuto!\n", 27);
}