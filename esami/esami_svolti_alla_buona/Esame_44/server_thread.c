/*
 * Server TCP concorrente basato su thread
 * Confronto prestazioni con architettura a processi
 */

#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

#define MAXLINE 4096
#define LISTENQ 1024

/* Struttura per passare dati ai thread */
struct thread_data {
    int connfd;
    struct sockaddr_in client_addr;
};

/* Variabile globale per il socket di ascolto */
static int listenfd;

/* Prototipi delle funzioni */
static void* handle_client(void* arg);
static void setup_server(int port);
static void cleanup_and_exit(int sig);
static void process_request(int connfd);

/*
 * Funzione principale del server
 */
int main(int argc, char **argv)
{
    int connfd, port;
    socklen_t clilen;
    struct sockaddr_in client_addr;
    struct thread_data *data;
    pthread_t thread_id;
    pthread_attr_t attr;
    
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
    signal(SIGPIPE, SIG_IGN); /* Ignora SIGPIPE */
    
    /* Inizializza il server */
    setup_server(port);
    
    /* Configura attributi thread per essere detached */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
    printf("Server (thread-based) listening on port %d\n", port);
    
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
        
        /* Alloca memoria per i dati del thread */
        if ((data = malloc(sizeof(struct thread_data))) == NULL) {
            err_sys("malloc error");
        }
        
        data->connfd = connfd;
        data->client_addr = client_addr;
        
        /* Crea nuovo thread per gestire il client */
        if (pthread_create(&thread_id, &attr, handle_client, data) != 0) {
            err_sys("pthread_create error");
        }
    }
    
    /* Pulizia (mai raggiunto) */
    pthread_attr_destroy(&attr);
    return 0;
}

/*
 * Funzione eseguita da ogni thread per gestire un client
 */
static void* handle_client(void* arg)
{
    struct thread_data *data = (struct thread_data*)arg;
    int connfd = data->connfd;
    
    /* Log connessione client */
    printf("Thread handling client %s\n", 
           inet_ntoa(data->client_addr.sin_addr));
    
    /* Processa la richiesta */
    process_request(connfd);
    
    /* Chiudi connessione e libera memoria */
    close(connfd);
    free(data);
    
    return NULL;
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
        snprintf(buffer, MAXLINE, "Response from thread-based server\n");
        write(connfd, buffer, strlen(buffer));
    }
}

/*
 * Gestisce terminazione pulita
 */
static void cleanup_and_exit(int sig)
{
    printf("\nShutting down thread-based server...\n");
    close(listenfd);
    exit(0);
}