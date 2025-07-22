#include "apue.h"
#include <sys/wait.h>
#include <string.h>

/* Prototipi delle funzioni */
static void sig_int(int signo);
static void parse_command(char *buf, char **cmd1, char **cmd2, char **outfile);
static void execute_single_command(char *cmd);
static void execute_pipe_command(char *cmd1, char *cmd2);
static void execute_redirect_command(char *cmd, char *outfile);
static void print_prompt(void);

/* Gestore del segnale SIGINT */
static void
sig_int(int signo)
{
    printf("\ninterrupt\n");
    print_prompt();
}

/* Stampa il prompt */
static void
print_prompt(void)
{
    printf("%% ");
    fflush(stdout);
}

/* 
 * Analizza la linea di comando per identificare:
 * - Pipe (|)
 * - Redirezione (>)
 * - Comando singolo
 */
static void
parse_command(char *buf, char **cmd1, char **cmd2, char **outfile)
{
    char *pipe_pos, *redir_pos;
    
    /* Inizializza i puntatori */
    *cmd1 = NULL;
    *cmd2 = NULL;
    *outfile = NULL;
    
    /* Cerca la pipe */
    pipe_pos = strchr(buf, '|');
    
    /* Cerca la redirezione */
    redir_pos = strchr(buf, '>');
    
    if (pipe_pos != NULL) {
        /* Comando con pipe */
        *pipe_pos = '\0';
        *cmd1 = buf;
        *cmd2 = pipe_pos + 1;
        
        /* Rimuove spazi iniziali e finali */
        while (**cmd1 == ' ') (*cmd1)++;
        while (**cmd2 == ' ') (*cmd2)++;
        
        /* Rimuove spazi finali da cmd1 */
        pipe_pos--;
        while (pipe_pos > *cmd1 && *pipe_pos == ' ') {
            *pipe_pos = '\0';
            pipe_pos--;
        }
        
    } else if (redir_pos != NULL) {
        /* Comando con redirezione */
        *redir_pos = '\0';
        *cmd1 = buf;
        *outfile = redir_pos + 1;
        
        /* Rimuove spazi iniziali e finali */
        while (**cmd1 == ' ') (*cmd1)++;
        while (**outfile == ' ') (*outfile)++;
        
        /* Rimuove spazi finali da cmd1 */
        redir_pos--;
        while (redir_pos > *cmd1 && *redir_pos == ' ') {
            *redir_pos = '\0';
            redir_pos--;
        }
        
    } else {
        /* Comando singolo */
        *cmd1 = buf;
        while (**cmd1 == ' ') (*cmd1)++;
    }
}

/* Esegue un comando singolo */
static void
execute_single_command(char *cmd)
{
    pid_t pid;
    int status;
    
    if ((pid = fork()) < 0) {
        err_sys("fork error");
    } else if (pid == 0) {
        /* Processo figlio */
        execlp(cmd, cmd, (char *)0);
        err_ret("couldn't execute: %s", cmd);
        exit(127);
    }
    
    /* Processo padre - attende il figlio */
    if ((pid = waitpid(pid, &status, 0)) < 0)
        err_sys("waitpid error");
}

/* Esegue due comandi collegati da pipe */
static void
execute_pipe_command(char *cmd1, char *cmd2)
{
    int pipefd[2];
    pid_t pid1, pid2;
    int status;
    
    /* Crea la pipe */
    if (pipe(pipefd) < 0)
        err_sys("pipe error");
    
    /* Primo processo (producer) */
    if ((pid1 = fork()) < 0) {
        err_sys("fork error");
    } else if (pid1 == 0) {
        /* Chiude il lato di lettura della pipe */
        close(pipefd[0]);
        
        /* Redirige stdout verso la pipe */
        if (dup2(pipefd[1], STDOUT_FILENO) != STDOUT_FILENO)
            err_sys("dup2 error");
        close(pipefd[1]);
        
        /* Esegue il primo comando */
        execlp(cmd1, cmd1, (char *)0);
        err_ret("couldn't execute: %s", cmd1);
        exit(127);
    }
    
    /* Secondo processo (consumer) */
    if ((pid2 = fork()) < 0) {
        err_sys("fork error");
    } else if (pid2 == 0) {
        /* Chiude il lato di scrittura della pipe */
        close(pipefd[1]);
        
        /* Redirige stdin dalla pipe */
        if (dup2(pipefd[0], STDIN_FILENO) != STDIN_FILENO)
            err_sys("dup2 error");
        close(pipefd[0]);
        
        /* Esegue il secondo comando */
        execlp(cmd2, cmd2, (char *)0);
        err_ret("couldn't execute: %s", cmd2);
        exit(127);
    }
    
    /* Processo padre - chiude entrambi i lati della pipe */
    close(pipefd[0]);
    close(pipefd[1]);
    
    /* Attende entrambi i processi figli */
    if (waitpid(pid1, &status, 0) < 0)
        err_sys("waitpid error for first command");
    if (waitpid(pid2, &status, 0) < 0)
        err_sys("waitpid error for second command");
}

/* Esegue un comando con redirezione dell'output verso file */
static void
execute_redirect_command(char *cmd, char *outfile)
{
    pid_t pid;
    int status;
    int fd;
    
    if ((pid = fork()) < 0) {
        err_sys("fork error");
    } else if (pid == 0) {
        /* Processo figlio */
        /* Apre il file di output */
        if ((fd = creat(outfile, 0644)) < 0)
            err_sys("can't create %s", outfile);
        
        /* Redirige stdout verso il file */
        if (dup2(fd, STDOUT_FILENO) != STDOUT_FILENO)
            err_sys("dup2 error");
        close(fd);
        
        /* Esegue il comando */
        execlp(cmd, cmd, (char *)0);
        err_ret("couldn't execute: %s", cmd);
        exit(127);
    }
    
    /* Processo padre - attende il figlio */
    if ((pid = waitpid(pid, &status, 0)) < 0)
        err_sys("waitpid error");
}

int
main(void)
{
    char buf[MAXLINE];
    char *cmd1, *cmd2, *outfile;
    
    /* Installa il gestore del segnale SIGINT */
    if (signal(SIGINT, sig_int) == SIG_ERR)
        err_sys("signal error");
    
    print_prompt();
    
    /* Loop principale della shell */
    while (fgets(buf, MAXLINE, stdin) != NULL) {
        /* Rimuove il carattere di newline */
        if (buf[strlen(buf) - 1] == '\n')
            buf[strlen(buf) - 1] = '\0';
        
        /* Salta linee vuote */
        if (strlen(buf) == 0) {
            print_prompt();
            continue;
        }
        
        /* Analizza la linea di comando */
        parse_command(buf, &cmd1, &cmd2, &outfile);
        
        if (cmd2 != NULL) {
            /* Comando con pipe */
            execute_pipe_command(cmd1, cmd2);
        } else if (outfile != NULL) {
            /* Comando con redirezione */
            execute_redirect_command(cmd1, outfile);
        } else if (cmd1 != NULL && strlen(cmd1) > 0) {
            /* Comando singolo */
            execute_single_command(cmd1);
        }
        
        print_prompt();
    }
    
    exit(0);
}