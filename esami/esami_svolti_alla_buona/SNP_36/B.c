/*
 * B.c - Programma Server per conteggio file/directory
 * 
 * Descrizione: Server UDP che riceve richieste per contare file o directory
 * del sistema e restituisce il risultato al client.
 * 
 * Utilizzo: ./B
 * 
 * Argomenti traccia:
 * - Server UDP che ascolta richieste
 * - Conteggio file o directory del sistema
 * - Invio risposta numerica al client
 * - Utilizzo di ftw() per attraversamento filesystem
 */

#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ftw.h>

#define SERVER_PORT 12345
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

/* Variabili globali per il conteggio */
static long file_count = 0;
static long dir_count = 0;
static char count_type = 'f';  /* 'f' per file, 'd' per directory */

/* Prototipi delle funzioni */
static int create_server_socket(void);
static void bind_server_socket(int sockfd);
static void server_loop(int sockfd);
static void handle_client_request(int sockfd, const struct sockaddr_in *client_addr, 
                                 socklen_t client_len, char request);
static long count_filesystem_items(char type);
static int count_callback(const char *pathname, const struct stat *statbuf, int typeflag);
static void send_response(int sockfd, const struct sockaddr_in *client_addr, 
                         socklen_t client_len, long count);

/*
 * Funzione principale del server
 */
int main(void)
{
    int sockfd;
    
    printf("=== SERVER CONTATORE FILE/DIRECTORY ===\n");
    printf("Porta: %d\n\n", SERVER_PORT);
    
    /* Crea socket server */
    sockfd = create_server_socket();
    
    /* Associa socket alla porta */
    bind_server_socket(sockfd);
    
    /* Avvia loop principale del server */
    server_loop(sockfd);
    
    close(sockfd);
    return 0;
}

/*
 * Crea il socket server UDP
 * Ritorna: file descriptor del socket
 */
static int create_server_socket(void)
{
    int sockfd;
    int opt = 1;
    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        err_sys("errore socket()");
    
    /* Permetti riuso indirizzo */
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        err_sys("errore setsockopt()");
    
    printf("Socket server creato.\n");
    return sockfd;
}

/*
 * Associa il socket alla porta del server
 * sockfd: file descriptor del socket
 */
static void bind_server_socket(int sockfd)
{
    struct sockaddr_in server_addr;
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);
    
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        err_sys("errore bind()");
    
    printf("Server in ascolto sulla porta %d...\n\n", SERVER_PORT);
}

/*
 * Loop principale del server - gestisce le richieste dei client
 * sockfd: socket del server
 */
static void server_loop(int sockfd)
{
    struct sockaddr_in client_addr;
    socklen_t client_len;
    char request;
    ssize_t n;
    
    while (1) {
        client_len = sizeof(client_addr);
        
        /* Ricevi richiesta dal client */
        printf("Attendo richieste...\n");
        
        if ((n = recvfrom(sockfd, &request, 1, 0,
                         (struct sockaddr *)&client_addr, &client_len)) < 0) {
            err_ret("errore recvfrom()");
            continue;
        }
        
        if (n == 0) {
            printf("Connessione chiusa dal client.\n");
            continue;
        }
        
        printf("Richiesta '%c' ricevuta da %s:%d\n", 
               request, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        /* Gestisci richiesta */
        handle_client_request(sockfd, &client_addr, client_len, request);
    }
}

/*
 * Gestisce una singola richiesta del client
 * sockfd: socket del server
 * client_addr: indirizzo del client
 * client_len: lunghezza indirizzo client
 * request: tipo di richiesta ('f' o 'd')
 */
static void handle_client_request(int sockfd, const struct sockaddr_in *client_addr,
                                 socklen_t client_len, char request)
{
    long count;
    
    /* Valida richiesta */
    if (request != 'f' && request != 'd') {
        printf("Richiesta non valida: '%c'\n", request);
        send_response(sockfd, client_addr, client_len, -1);
        return;
    }
    
    if (request == 'f') {
        printf("Conteggio FILE in corso...\n");
    } else {
        printf("Conteggio DIRECTORY in corso...\n");
    }
    
    /* Esegui conteggio */
    count = count_filesystem_items(request);
    
    printf("Conteggio completato: %ld elementi\n", count);
    
    /* Invia risposta al client */
    send_response(sockfd, client_addr, client_len, count);
    
    printf("Risposta inviata al client.\n\n");
}

/*
 * Conta file o directory nel filesystem
 * type: 'f' per file, 'd' per directory
 * Ritorna: numero di elementi contati
 */
static long count_filesystem_items(char type)
{
    /* Inizializza contatori e tipo globale */
    file_count = 0;
    dir_count = 0;
    count_type = type;
    
    /* Attraversa filesystem partendo dalla root */
    /* Nota: su alcuni sistemi potrebbe essere necessario usare un path diverso */
    if (ftw("/", count_callback, 20) < 0) {
        err_ret("errore ftw()");
        return -1;
    }
    
    return (type == 'f') ? file_count : dir_count;
}

/*
 * Callback per ftw() - conta file e directory
 * pathname: percorso dell'elemento corrente  
 * statbuf: informazioni stat dell'elemento
 * typeflag: tipo di elemento (FTW_F, FTW_D, etc.)
 * Ritorna: 0 per continuare, non-zero per fermarsi
 */
static int count_callback(const char *pathname, const struct stat *statbuf, int typeflag)
{
    /* Evita pathname per evitare warning unused parameter */
    (void)pathname;
    (void)statbuf;
    
    switch (typeflag) {
        case FTW_F:   /* File regolare */
            file_count++;
            break;
            
        case FTW_D:   /* Directory */
            dir_count++;
            break;
            
        case FTW_DNR: /* Directory non leggibile */
            if (count_type == 'd')
                dir_count++;
            break;
            
        case FTW_NS:  /* Stat fallito */
            /* Ignora questi elementi */
            break;
    }
    
    return 0;  /* Continua attraversamento */
}

/*
 * Invia risposta numerica al client
 * sockfd: socket del server
 * client_addr: indirizzo del client
 * client_len: lunghezza indirizzo client  
 * count: numero da inviare
 */
static void send_response(int sockfd, const struct sockaddr_in *client_addr,
                         socklen_t client_len, long count)
{
    char buffer[BUFFER_SIZE];
    int len;
    
    /* Converti numero in stringa */
    len = snprintf(buffer, sizeof(buffer), "%ld", count);
    
    if (len >= sizeof(buffer)) {
        err_ret("buffer troppo piccolo per risposta");
        return;
    }
    
    /* Invia risposta */
    if (sendto(sockfd, buffer, len, 0,
               (struct sockaddr *)client_addr, client_len) < 0) {
        err_ret("errore sendto()");
        return;
    }
}