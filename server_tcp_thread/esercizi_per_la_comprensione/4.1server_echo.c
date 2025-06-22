#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>

#define port 8080

int main(void){
    int server_fd;
    int client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer [1024] = {0};


    //Setup del server

    server_fd = socket(AF_INET,SOCK_STREAM,0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);


    bind(server_fd,(struct sockaddr_in *)&address,addrlen);
    listen(server_fd,3);


    //TODO : accettettare nuova connessione
    while(1){
        client_fd = accept(server_fd,(struct sockaddr_in *)&address, (socklen_t*)&addrlen);
        if(client_fd<0){
            err_sys("accept failed");
        }

        printf("Client connesso \n");


        //TODO : Leggere messaggio dal client

        int bytes_read = read(client_fd,buffer,sizeof(buffer));
        printf("Ricevuto:  %s\n", buffer);


        //TODO : Rimandare lo stesso messaggio
        send(client_fd,buffer,bytes_read,0);
    }

    
    close(client_fd);
    close(server_fd);
    return 0;



}