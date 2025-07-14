/*
 * Server TCP semplice con una porta e un client alla volta
 * Uso: ./server [porta]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#define BUFFER_SIZE 1024
#define BACKLOG 5
#define DEFAULT_PORT 8080

/*=========STRUCT===============*/
struct {
    int connessioni_totali;
    int messaggi_elaborati;
    time_t avvio_server;
}stats;

/*==============VARIABILI GLOBALI=============*/
int porta = DEFAULT_PORT;
int listening_socket = -1;
int continua_esecuzione =1;
int client_socket = -1;
int A;



/*==========SERVER==================*/
int init_listening_socket(){
    int reuse = 1;
    struct sockaddr_in addr;

    /*Creazione del socket*/
    listening_socket = socket(AF_INET,SOCK_STREAM,0);
    if (listening_socket < 0) {
        printf("Errore socket: %s\n", strerror(errno));
        return -1;
    }

    /*configurazione indirizzo*/
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons (porta);


    /*Bind*/
    if(bind(listening_socket,(struct sockaddr*)&addr,sizeof(addr))<0){
        printf("Errore bind porta %d: %s\n", porta, strerror(errno));
        close(listening_socket);
        return -1;
    }

    /*Listen*/
    if(listen(listening_socket,BACKLOG)<0){
        printf("Errore listen: %s\n",strerror(errno));
        close(listening_socket);
        return -1;
    }

    printf("Server in ascolto sulla porta %d\n",porta);
    return 0;
}

int accetta_connessione(){
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    char wlc_msg[64];

    printf("In attesa di connessioni...\n");

    client_socket = accept(listening_socket,(struct sockaddr*)&client_addr,&client_len);
    if (client_socket < 0) {
        if (continua_esecuzione) { /* Solo se non stiamo terminando */
            printf("Errore accept: %s\n", strerror(errno));
        }
        return -1;
    }

    /*aggiorna statistiche*/
    stats.connessioni_totali++;

    /*Welcome messagge*/
    snprintf(wlc_msg,sizeof(wlc_msg),"Connesso al server. Porta %d\n",porta);
    send(client_socket,wlc_msg,strlen(wlc_msg),0);

    
    printf("Nuova connessione da %s:%d\n",
            inet_ntoa(client_addr.sin_addr),
            ntohs(client_addr.sin_port));

    return 0;
}

int gestisci_client(){
    int bytes_read;
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE + 50];
    char name[] = "A";
    char number[BUFFER_SIZE];

    printf("SONO nella funzione gestisci_client\n");

    while (continua_esecuzione)
    {
        bytes_read = recv(client_socket,buffer,BUFFER_SIZE-1,0);
        
        if (bytes_read <= 0) {
            if (bytes_read == 0) {
                printf("Client disconnesso\n");
                /*Il server si interrompe non appena il client chiude la connessione*/
                continua_esecuzione = 0;
                close(listening_socket);
                break;

            } else if (continua_esecuzione) {
                printf("Errore recv: %s\n", strerror(errno));
            }
            break;
        }

        buffer[bytes_read] = '\0'; /*'\0' terminatore di stringa*/

        /*Rimozione new line finale*/
        if(buffer[bytes_read -1] == '\n'){
            buffer[bytes_read - 1] = '\0';
           

        }

        /*LAVORO SERVER*/
        strcpy(number,buffer);
        

        /*RIPARTIRE DA QUI DOMANIIIIIIIIIIIIIIIIIIIIII*/
        if(setenv(name,number,1) == -1){
            perror("setenv error");
        }else {
            printf("Variabile A del server aggiornata a: %s\n", getenv("A"));  // ✅ DEBUG
        }


        /*risposta echo*/
        snprintf(response, sizeof(response),"Echo : %s\n",buffer);

        /*Inviamo risposta*/
        /* Invia risposta */
        if (send(client_socket, response, strlen(response), 0) < 0) {
            printf("Errore send: %s\n", strerror(errno));
            break;
        }

        /* Aggiorna statistiche */
        stats.messaggi_elaborati++;
        printf("Messaggio elaborato: %s\n", buffer);
    }
    /* Chiudi connessione client */
    if (client_socket != -1) {
        close(client_socket);
        client_socket = -1;
        printf("Connessione client chiusa\n");
    }

    return 0;
}
/*=====================SEGNALI==================*/
void signal_handler(int sig) {
    printf("\nRicevuto segnale %d. Iniziando terminazione...\n", sig);
    continua_esecuzione = 0;
    
    /* Chiudi i socket per sbloccare accept() e recv() */
    if (client_socket != -1) {
        close(client_socket);
        client_socket = -1;
    }
    if (listening_socket != -1) {
        close(listening_socket);
        listening_socket = -1;
    }
}

int main(int argc, char* argv[]){
    /*Parsing degli argomenti*/
    if(argc>1){
        porta = atoi(argv[1]);
        if(porta <1024 || porta  > 65535){
            fprintf(stderr, "Porta deve essere tra 1024 e 65535");
            exit(1);
        }
    }

    /*Iniziallizzazione statistiche*/
    memset(&stats,0,sizeof(stats));
    stats.avvio_server = time(NULL);

    /*Segnali*/
    signal(SIGTERM,signal_handler);
    signal(SIGINT,signal_handler);
    

    printf("Avvio server sulla porta %d\n", porta);

    if(init_listening_socket()<0){
        printf("Errore inizializzazione socket\n");
        exit(EXIT_FAILURE);
    }


    while (continua_esecuzione)
    {
        if(accetta_connessione() == 0){
            gestisci_client();
        }
        
        if (continua_esecuzione) {
            printf("Pronto per nuova connessione...\n");
            sleep(1);
        }
    }
    

}