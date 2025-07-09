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
#include <sys/select.h>

#define BUFFER_SIZE 1024
#define MAX_MESSAGE_LEN 512


/*=======VARIABILI GLOBALI=============*/
int continua_esecuzione = 1;
int client_socket  =-1;
/*==============SEGNALI==============*/

void signal_handler(int sig){
    printf("\nRicevuto segnale %d. Disconnessione...\n", sig);
    continua_esecuzione = 0;
    if (client_socket >= 0) {
        close(client_socket);
    }
}
/*===========SERVER============*/

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


    /*Connesione*/
    printf("Connessione al %s:%d...,\n",indirizzo,porta);
    if(connect(sock, (struct sockaddr*)&server_addr,sizeof(server_addr))<0){
        printf("Errore connesione al server");
        close(sock);
        return -1;
    }    

    printf("Connesso con successo...!\n");

    return sock;
}
void welcome_message(int sock){
    fd_set read_fds;
    struct timeval time;
    int bytes;
    char buffer[BUFFER_SIZE];
    int result;

    FD_ZERO(&read_fds);
    FD_SET(sock,&read_fds);
    

    time.tv_sec =1;
    time.tv_usec =0;

    result = select(sock+1,&read_fds,NULL,NULL,&time);
    if(result > 0 && FD_ISSET(sock,&read_fds)){
        bytes = recv(sock,buffer,BUFFER_SIZE-1,0);
        if(bytes > 0){
            buffer[bytes] = '\0';
            printf("Server: %s\n",buffer);
        }
    }

}

int send_message(int sock){
    time_t inizio, fine;
    int i;
    char messaggio [50];
    char buffer[BUFFER_SIZE];
    int bytes;

    printf("Invio 50 messaggi \n");
    
    for(i=0;i<50 && continua_esecuzione;i++){
        sprintf(messaggio,"Messaggio %d \n",i);
        if(send(sock,messaggio,strlen(messaggio),0)>=0){
            printf("%s",messaggio);
        }

        /*Riceviamo gli echo del server*/
        bytes = recv(sock, buffer,BUFFER_SIZE -1,MSG_DONTWAIT);

        usleep(50000);
    }
}

int main(int argc, char* argv[]){
    char* indirizzo;
    int porta;
    


  /* parsing argomenti */
    if (argc < 2) {
        printf("Uso: %s <indirizzo> <porta>\n", argv[0]);
        exit(1);
    }
    indirizzo = argv[1];
    porta = atoi(argv[2]);


    
    if (porta < 1 || porta > 65535) {
        printf("Porta deve essere tra 1 e 65535\n");
        exit(1);
    }
    
    

    signal(SIGINT,signal_handler);
    signal(SIGTERM,signal_handler);
    signal(SIGPIPE,signal_handler);    

    /*connessione al server*/
    client_socket = connessione_server(indirizzo,porta);
    

    /*Ricezione messaggio benvenuto*/
    welcome_message(client_socket);

    send_message(client_socket);

    close(client_socket);
    return 0;


    
}
    