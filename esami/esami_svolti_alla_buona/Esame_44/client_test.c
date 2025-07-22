/*
 * Client di test per i server TCP
 * Utilizzato per test manuali e benchmark
 */

#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>

#define MAXLINE 4096

/* Prototipi delle funzioni */
static int connect_to_server(const char *host, int port);
static double get_time_diff(struct timeval start, struct timeval end);
static void send_request(int sockfd);

/*
 * Funzione principale del client
 */
int main(int argc, char **argv)
{
    int sockfd, port;
    char *host;
    struct timeval start, end;
    double elapsed;
    
    /* Controlla argomenti */
    if (argc != 3) {
        err_quit("usage: %s <host> <port>", argv[0]);
    }
    
    host = argv[1];
    port = atoi(argv[2]);
    
    if (port <= 0 || port > 65535) {
        err_quit("invalid port number");
    }
    
    printf("Connecting to %s:%d\n", host, port);
    
    /* Misura tempo di connessione e comunicazione */
    gettimeofday(&start, NULL);
    
    /* Connetti al server */
    sockfd = connect_to_server(host, port);
    
    /* Invia richiesta */
    send_request(sockfd);
    
    /* Chiudi connessione */
    close(sockfd);
    
    gettimeofday(&end, NULL);
    elapsed = get_time_diff(start, end);
    
    printf("Request completed in %.3f ms\n", elapsed * 1000);
    
    return 0;
}

/*
 * Connette al server specificato
 */
static int connect_to_server(const char *host, int port)
{
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *he;
    
    /* Crea socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        err_sys("socket error");
    }
    
    /* Risolvi hostname */
    if ((he = gethostbyname(host)) == NULL) {
        err_quit("gethostbyname error for %s", host);
    }
    
    /* Configura indirizzo server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr, he->h_addr, he->h_length);
    
    /* Connetti */
    if (connect(sockfd, (SA *) &server_addr, sizeof(server_addr)) < 0) {
        err_sys("connect error");
    }
    
    return sockfd;
}

/*
 * Invia richiesta al server e riceve risposta
 */
static void send_request(int sockfd)
{
    char send_buf[MAXLINE], recv_buf[MAXLINE];
    ssize_t n;
    
    /* Prepara messaggio */
    snprintf(send_buf, MAXLINE, "Test request from client");
    
    /* Invia richiesta */
    if (write(sockfd, send_buf, strlen(send_buf)) < 0) {
        err_sys("write error");
    }
    
    /* Ricevi risposta */
    if ((n = read(sockfd, recv_buf, MAXLINE-1)) > 0) {
        recv_buf[n] = '\0';
        printf("Server response: %s", recv_buf);
    }
}

/*
 * Calcola differenza di tempo in secondi
 */
static double get_time_diff(struct timeval start, struct timeval end)
{
    return (end.tv_sec - start.tv_sec) + 
           (end.tv_usec - start.tv_usec) / 1000000.0;
}