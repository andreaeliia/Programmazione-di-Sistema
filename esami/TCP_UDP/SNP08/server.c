/*
 * Daemon server con 10 socket TCP e I/O multiplexing
 * Uso: ./daemon [porta_base]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>

#define NUM_PORTS 10
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024
#define BACKLOG 5


/* Statistiche */
struct {
    int connessioni_totali;
    int messaggi_elaborati;
    time_t avvio_daemon;
    int connessioni_per_porta[NUM_PORTS];
} stats;

typedef struct 
{
    int socket;
    int porta;
    time_t connection_time;
    int attivo;
}client_info_t;

/* Forward declaration per evitare warning */
void stampa_statistiche();

/*=============Variabili globali==============*/
int porta_base = 8080;
int continua_esecuzione = 1;
int listening_socket[NUM_PORTS];
client_info_t clients[MAX_CLIENTS];
int num_clients = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

/* =======================DAEMON=====================*/
void become_daemon() {
    pid_t pid;
    
    pid = fork();
    if (pid < 0) {
        perror("fork fallita");
        exit(1);
    }
    
    if (pid > 0) {
        exit(0); /* termina processo padre */
    }
    
    /* processo figlio diventa session leader */
    if (setsid() < 0) {
        perror("setsid fallita");
        exit(1);
    }
    
    /* secondo fork per evitare che il daemon riacquisisca il terminale */
    pid = fork();
    if (pid < 0) {
        perror("secondo fork fallita");
        exit(1);
    }
    
    if (pid > 0) {
        exit(0);
    }

    printf("PID: %d\n",pid);
    
    /* cambia directory di lavoro */
    chdir("/");
    
    /* chiude file descriptor standard */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    /* reindirizza a /dev/null */
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_WRONLY);
    
    umask(0);
}

void log_messaggio(const char* messaggio) {
    time_t ora = time(NULL);
    char* time_str = ctime(&ora);
    FILE* log_file;
    
    time_str[strlen(time_str)-1] = '\0'; /* rimuove newline */
    
    printf("[%s] %s\n", time_str, messaggio);
    fflush(stdout);
    
    log_file = fopen("daemon.log", "a");
    if (log_file) {
        fprintf(log_file, "[%s] %s\n", time_str, messaggio);
        fclose(log_file);
    }
}

void cleanup() {
    int i;
    char msg[256];
    
    log_messaggio("Iniziando cleanup risorse...");
    
    /* chiudi tutti i client socket */
    pthread_mutex_lock(&clients_mutex);
    for (i = 0; i < num_clients; i++) {
        if (clients[i].attivo) {
            close(clients[i].socket);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    
    /* chiudi listening socket */
    for (i = 0; i < NUM_PORTS; i++) {
        close(listening_socket[i]);
        sprintf(msg, "Chiuso socket porta %d", porta_base + i);
        log_messaggio(msg);
    }
    
    stampa_statistiche();
    log_messaggio("Daemon terminato");
}

void stampa_statistiche() {
    time_t ora_corrente = time(NULL);
    int durata = (int)(ora_corrente - stats.avvio_daemon);
    int i;
    char msg[512];
    
    sprintf(msg, "=== STATISTICHE DAEMON ===");
    log_messaggio(msg);
    sprintf(msg, "Tempo attivo: %02d:%02d:%02d", 
            durata/3600, (durata%3600)/60, durata%60);
    log_messaggio(msg);
    sprintf(msg, "Connessioni totali: %d", stats.connessioni_totali);
    log_messaggio(msg);
    sprintf(msg, "Messaggi elaborati: %d", stats.messaggi_elaborati);
    log_messaggio(msg);
    sprintf(msg, "Client attualmente connessi: %d", num_clients);
    log_messaggio(msg);
    
    for (i = 0; i < NUM_PORTS; i++) {
        sprintf(msg, "Porta %d: %d connessioni", 
                porta_base + i, stats.connessioni_per_porta[i]);
        log_messaggio(msg);
    }
}

/*=====================SEGNALI==================*/
void signal_handler(int sig) {
    char msg[256];
    sprintf(msg, "Ricevuto segnale %d. Iniziando terminazione...", sig);
    log_messaggio(msg);
    continua_esecuzione = 0;
}

/*==================SERVER=======================*/

int init_listening_socket(){
    int i,sock,reuse =1;
    struct sockaddr_in addr;
    char msg[256];

    /*1.socket  2. setsockopt      3. memset   4.bind  5.listening*/

    for(i=0;i<NUM_PORTS;i++){
        sock = socket(AF_INET,SOCK_STREAM,0);
        if(sock<0){
            snprintf(msg, sizeof(msg), "Errore socket su porta %d", porta_base+i);
            log_messaggio(msg);
            close(sock);
            return -1;
        }
        /*Riutilizzo indirizzo*/
        if(setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse))<0){
            snprintf(msg, sizeof(msg), "Errore setsocket su porta %d\n",porta_base+i);
            log_messaggio(msg);
            close(sock);
            return -1;
        }

        /*Configurazione indirizzo*/
        memset(&addr,0,sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(porta_base+i);

        /*bind*/
        if(bind(sock,(struct sockaddr*)&addr,sizeof(addr))<0){
            snprintf(msg, sizeof(msg), "Errore bind su porta %d",porta_base+i);
            log_messaggio(msg);
            close(sock);
            return -1;
        }

        /*Listen*/
        if(listen(sock,BACKLOG)<0){
            snprintf(msg, sizeof(msg), "Errore listen su porta %d",porta_base+i);
            log_messaggio(msg);
            close(sock);
            return -1;
        }

        listening_socket[i] = sock;
        snprintf(msg, sizeof(msg), "Socket listening attivato sulla porta %d",porta_base +i);
        log_messaggio(msg);
    }
    
    return 0;
}

void accetta_connessione(int listening_socket, int porta){
    int client_socket;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    char welcome_msg[256];
    char log_msg[256];

    client_socket = accept(listening_socket,(struct sockaddr*)&client_addr,&client_len);
    if (client_socket < 0) {
        sprintf(log_msg, "Errore accept porta %d: %s", 
                porta_base + porta, strerror(errno));
        log_messaggio(log_msg);
        return;
    }

    pthread_mutex_lock(&clients_mutex);

    if(num_clients >= MAX_CLIENTS){
        snprintf(log_msg, sizeof(log_msg), "massimo numero client raggiunto, connessione rifiutata");
        log_messaggio(log_msg);
        close(client_socket);
        pthread_mutex_unlock(&clients_mutex);
        return;
    }

    /*Aggiungere il client*/
    clients[num_clients].socket = client_socket;
    clients[num_clients].porta = porta_base + porta;
    clients[num_clients].connection_time = time(NULL);
    clients[num_clients].attivo = 1;
    num_clients++;

    stats.connessioni_totali++;
    stats.connessioni_per_porta[porta]++;

    pthread_mutex_unlock(&clients_mutex);

    /*Invio messaggi di benvenuto*/
    snprintf(welcome_msg, sizeof(welcome_msg), "Connesso alla porta %d \n",porta_base+porta);
    send(client_socket,welcome_msg,strlen(welcome_msg),0);

    snprintf(log_msg, sizeof(log_msg), "Nuova connessione da %s:%d su porta %d",
            inet_ntoa(client_addr.sin_addr),
            ntohs(client_addr.sin_port),
            porta_base+porta);
    log_messaggio(log_msg);
}

void gestisci_client_data(int client_idx){
    int bytes_read;
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE+50];
    char log_msg[256];

    bytes_read = recv(clients[client_idx].socket,buffer,BUFFER_SIZE-1,0);
    if(bytes_read<=0){
        /*Clienti disconnesso*/
        sprintf(log_msg,"Client disconnesso dalla porta %d\n",clients[client_idx].porta);
        log_messaggio(log_msg);
        close(clients[client_idx].socket);
        clients[client_idx].attivo = 0;
        return;
    }

    buffer[bytes_read] = '\0';

    /*Rimuoviamo new line finale*/
    if(buffer[bytes_read -1] == '\n'){
        buffer[bytes_read-1] = '\0';
    }

    /*Creiamo risposta echo*/
    snprintf(response, sizeof(response), "Echo[%d]: %s\n",clients[client_idx].porta,buffer);

    /*Inviamo risposta*/
    if(send(clients[client_idx].socket,response,strlen(response),0)<0){
        /*errore di invio*/
        sprintf(log_msg,"Errore invio risposta a client su porta %d\n",clients[client_idx].porta);
        log_messaggio(log_msg);
    }else{
        /*invio andato a buon fine*/
        stats.messaggi_elaborati++;
        sprintf(log_msg,"messaggio elaborato su porta %d\n",clients[client_idx].porta);
        log_messaggio(log_msg);
    }
}

void* io_multiplexing_thread(void* arg){
    fd_set master_fds,read_fds;
    int max_fd, activity;
    int i;
    struct timeval time;
    char msg[256];

    (void)arg; /*si fa per evitare warning di unused parameter*/

    /*Iniziallizzazione set di file descriptor*/
    FD_ZERO(&master_fds);
    max_fd = 0;

    /*Aggiungiamo listerning socket al set*/
    for(i=0;i<NUM_PORTS;i++){
        FD_SET(listening_socket[i],&master_fds);
        if(listening_socket[i]>max_fd){
            /* Teniamo traccia del file descriptor più alto per select() */
            max_fd = listening_socket[i];
        }
    }

    while (continua_esecuzione)
    {
        /* Copiamo master_fds perché select() modifica il set passato */
        read_fds = master_fds;

        /*aggiungiamo client socket al thread*/
        pthread_mutex_lock(&clients_mutex);
        for (i = 0; i < num_clients; i++)
        {   
            if(clients[i].attivo){
                FD_SET(clients[i].socket,&read_fds);
                if(clients[i].socket>max_fd){
                    max_fd = clients[i].socket;
                }
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        /*Timeout per mettere il controllo periodico di 1 second*/
        time.tv_sec = 1;
        time.tv_usec = 0;

        activity = select(max_fd +1,&read_fds,NULL,NULL,&time);

         if (activity < 0 && errno != EINTR) {
            sprintf(msg, "Errore select: %s", strerror(errno));
            log_messaggio(msg);
            break;
        }

        if(activity > 0){
            /*Controllo listening socket*/
            for ( i = 0; i < NUM_PORTS; i++)
            {
                if(FD_ISSET(listening_socket[i],&read_fds)){
                    accetta_connessione(listening_socket[i],i);
                }
            }
            /*Controlliamo client socket*/
            pthread_mutex_lock(&clients_mutex);
            for ( i = 0; i < num_clients; i++)
            {
                if(clients[i].attivo && FD_ISSET(clients[i].socket,&read_fds)){
                    gestisci_client_data(i);
                }
            }
            pthread_mutex_unlock(&clients_mutex);
        }
    }
    
    return NULL;
}

int main(int argc,char* argv[]){
    pthread_t io_thread;
    char msg[256];

     /* parsing argomenti */
    if (argc > 1) {
        porta_base = atoi(argv[1]);
        if (porta_base < 1024 || porta_base > 65525) {
            fprintf(stderr, "Porta base deve essere tra 1024 e 65525\n");
            exit(1);
        }
    }

    /*iniziallizazione statistiche*/
    memset(&stats,0,sizeof(stats));
    stats.avvio_daemon = time(NULL);

    /*Segnali*/
    signal(SIGTERM,signal_handler);
    signal(SIGINT,signal_handler);
    /* Ignora SIGPIPE che si genera quando si scrive su socket chiuso */
    signal(SIGPIPE,SIG_IGN);

    sprintf(msg,"Avvio daemon su porte %d-%d",porta_base,porta_base + NUM_PORTS - 1);
    log_messaggio(msg);

    /* Commentato per debug - decommenta per modalità daemon */
    /* become_daemon(); */

    /*inizializzazione dei socket*/
    if(init_listening_socket()<0){
        snprintf(msg, sizeof(msg), "Errore init_listing_socket");
        log_messaggio(msg);
        exit(EXIT_FAILURE);
    }

    /*creazione del thread io multiplexing*/
    if(pthread_create(&io_thread,NULL,io_multiplexing_thread,NULL)!= 0){
        log_messaggio("Errore creazione thread i/o\n");
        cleanup();
        exit(1);
    }

    log_messaggio("Daemon avviato con successo");

    /*Ogni 30 secondi stampiamo le statistiche*/
    while (continua_esecuzione)
    {
        sleep(30);
        stampa_statistiche();
    }

    /*attendiamo terminazione thread*/
    log_messaggio("attendiamo terminazione thread...");
    pthread_join(io_thread,NULL);

    cleanup();

    pthread_mutex_destroy(&clients_mutex);

    return 0 ;
}