#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8081
#define SERVER_IP "127.0.0.1"


int main(void){
    int client_fd;
    struct sockaddr_in server_addr;
    char message[] = "i'm the client";
    char buffer[1024];
    socklen_t server_len = sizeof(server_addr);



    //1.CREAZIONE SOCKET UDP
    client_fd = socket(AF_INET,SOCK_DGRAM,0);

    //2.CONFIGURAZIONE INDIRIZZO SERVER
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET,SERVER_IP,&server_addr.sin_addr);

    //3.INVIARE MESSAGGIO 
    sendto(client_fd,message,strlen(message),0,(struct sockaddr *)&server_addr,sizeof(server_addr));


    //4.RICEVERE RISPOSTA
    int bytes_received = recvfrom(client_fd,buffer,sizeof(buffer),0,(struct sockaddr*)&server_addr,&server_len);
    
    if(bytes_received > 0){
        buffer[bytes_received] = '\0';
        printf("Server ha risposto %s\n",buffer);
    }

    close(client_fd);
    return 0;



}