
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <time.h>  
#include <signal.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

/*==========VARIABILI GLOBALI==============*/

int A; /*Variabile d'ambiente*/
int continua_esecuzione = 1;





/*==============SERVER================*/
int connessione_server(const char* indirizzo, int porta){
    int sock;
    struct sockaddr_in server_addr;

    sock = socket(AF_INET,SOCK_STREAM,0);
    if(sock<0){
        printf("Errore creazione socket\n");
        return -1;
    }

    /*Configurazione indirizzo server*/
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(porta);

    if (inet_aton(indirizzo, &server_addr.sin_addr) == 0) {
        printf("Indirizzo IP non valido: %s\n", indirizzo);
        close(sock);
        return -1;
    }

    /*Connessione*/
    printf("Connessione al %s:%d...\n",indirizzo,porta);
    if(connect(sock, (struct sockaddr*)&server_addr,sizeof(server_addr))<0){
        printf("Errore connessione al server\n");
        close(sock);
        return -1;
    }    

    printf("Connesso con successo...!\n");
    return sock;
}

int send_message(int sock){
    char messaggio[50];
    char buffer[BUFFER_SIZE];
    int bytes;
    

    
    
    /* Invia il valore corrente di A*/
   
    
    
    

    while (1)
    {
        
    
    /*Riceviamo l'echo del server*/
    bytes = recv(sock, buffer, BUFFER_SIZE-1, 0);
    if(bytes > 0){
        buffer[bytes] = '\0';
        printf("Echo ricevuto: %s", buffer);
    } else if (bytes == 0) {
        printf("Server ha chiuso la connessione\n");
        break;
    } else {
        printf("Errore ricezione echo\n");
        return -1;
    }
    sleep(1);
}

    
    return 0;
}


/* ========== GESTIONE SEGNALI ========== */
void signal_handler(int sig) {
    printf("\n[Processo A] Ricevuto segnale %d. Terminazione...\n", sig);
    continua_esecuzione = 0;
    
}

int main(){
    int sock;

    
    signal(SIGINT,signal_handler);
    signal(SIGTERM,signal_handler);


    sock = connessione_server(SERVER_IP,SERVER_PORT);
    if (sock < 0) {
        printf("Impossibile connettersi al server\n");
        continua_esecuzione = 0;
        return -1;
    }
    
    if (send_message(sock) < 0) {
        printf("Messaggio non inviato");
    }
        


    close(sock);
    
    return 0;
}