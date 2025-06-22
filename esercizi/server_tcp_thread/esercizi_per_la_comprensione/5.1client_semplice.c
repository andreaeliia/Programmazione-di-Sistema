#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"





int main(void){
    int sock;
    struct sockaddr_in serv_addr;
    char message[] = "SONO GAY";
    char buffer[1024] = {0};

    //TODO: Creare socket
    sock = socket(AF_INET,SOCK_STREAM,0);

    //Configurazione indirizzo server
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);


    //Conservsione IP Stringa in formato numerico
    inet_pton(AF_INET,SERVER_IP, &serv_addr.sin_addr);


    //TODO: Connettersi al server

    if(connect(sock,(struct sockaddr_in*)&serv_addr,sizeof(serv_addr))<0){
        err_sys("Connetion failed!");
    }

    printf("Connesso al server!\n");

    //TODO: Inviare il messaggio 
    send(sock,message,strlen(message),0);

    //TODO: ricevere risposta
    read(sock,buffer,1024);

    printf("Server ha risposto: %s\n", buffer);
    
    close(sock);

    return 0;
}