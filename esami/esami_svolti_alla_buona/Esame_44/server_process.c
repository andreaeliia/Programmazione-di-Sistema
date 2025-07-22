/*
 * Server TCP concorrente basato su processi figli
 * Confronto prestazioni con architettura a thread
 */

#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#define MAXLINE 4096
#define LISTENQ 1024

/* Variabile globale per il socket di ascolto */
static int listenfd;

/* Prototipi delle funzioni */
static void handle_client(int connfd, struct sockaddr_in client_addr);
static void setup_server(int port);
static void cleanup_and_exit(int sig);
static void reap_children(int sig);
static void process_request(int connfd);

/*
 * Funzione principale del server
 */
int main(int argc, char **argv)
{
    int connfd, port;
    pid_t pid;
    socklen_t clilen;
    struct sockaddr_in client_addr;
    
    /* Controlla argomenti */
    if (argc != 2) {
        err_quit("usage: %s <port>", argv[0]);
    }
    
    port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        err_quit("invalid port number");
    }
    
    /* Configura gestione segnali */
    signal(SIGINT, cleanup_and_exit);
    signal(SIGTERM, cleanup_and_exit);
    signal(SIGCHLD, reap_children); /* Raccoglie processi figli terminati */
    signal(SIGPIPE, SIG_IGN); /* Ignora SIGPIPE */
    
    /* Inizializza il server */
    setup_server(port);
    
    printf("Server (process-based) listening on port %d\n", port);
    
    /* Loop principale del server */
    for (;;) {
        clilen = sizeof(client_addr);
        
        /* Accetta connessione */
        if ((connfd = accept(listenfd, (SA *) &client_addr, &clilen)) < 0) {
            if (errno == EINTR) {
                continue; /* Segnale ricevuto, riprova */
            }
            err_sys("accept error");
        }
        
        /* Crea processo figlio per gestire il client */
        if ((pid = fork()) == 0) {
            /* Processo figlio */
            close(listenfd); /* Figlio non ha bisogno del socket di ascolto */
            handle_client(connfd, client_addr);
            exit(0);
        } else if (pid > 0) {
            /* Processo padre */
            close(connfd); /* Padre non ha bisogno della connessione */
        } else {
            err_sys("fork error");
        }
    }
    
    return 0;
}

/*
 * Funzione eseguita dal processo figlio per gestire un client
 */
static void handle_client(int connfd, struct sockaddr_in client_addr)
{
    /* Log connessione client */
    printf("Process handling client %s\n", 
           inet_ntoa(client_addr.sin_addr));
    
    /* Processa la richiesta */
    process_request(connfd);
    
    /* Chiudi connessione */
    close(connfd);
}

/*
 * Configura il socket server
 */
static void setup_server(int port)
{
    struct sockaddr_in server_addr;
    int opt = 1;
    
    /* Crea socket */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        err_sys("socket error");
    }
    
    /* Abilita riuso dell'indirizzo */
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, 
                   &opt, sizeof(opt)) < 0) {
        err_sys("setsockopt error");
    }
    
    /* Configura indirizzo server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);
    
    /* Bind del socket */
    if (bind(listenfd, (SA *) &server_addr, sizeof(server_addr)) < 0) {
        err_sys("bind error");
    }
    
    /* Metti in ascolto */
    if (listen(listenfd, LISTENQ) < 0) {
        err_sys("listen error");
    }
}

/*
 * Processa una richiesta client (simulazione)
 */
static void process_request(int connfd)
{
    char buffer[MAXLINE];
    ssize_t n;
    
    /* Leggi richiesta */
    if ((n = read(connfd, buffer, MAXLINE-1)) > 0) {
        buffer[n] = '\0';
        
        /* Simula elaborazione (breve delay) */
        usleep(1000); /* 1ms */
        
        /* Invia risposta */
        snprintf(buffer, MAXLINE, "Response from process-based server\n");
        write(connfd, buffer, strlen(buffer));
    }
}

/*
 * Raccoglie processi figli terminati (evita zombie)
 */
static void reap_children(int sig)
{
    pid_t pid;
    int status;
    
    /* Raccoglie tutti i figli terminati */
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        /* Processo figlio terminato */
    }
}

/*
 * Gestisce terminazione pulita
 */
static void cleanup_and_exit(int sig)
{
    printf("\nShutting down process-based server...\n");
    close(listenfd);
    
    /* Termina tutti i processi figli */
    signal(SIGCHLD, SIG_DFL);
    kill(0, SIGTERM);
    
    exit(0);
}