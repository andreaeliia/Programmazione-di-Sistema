#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080

int main(void){
    //1.Variabili 
    int server_fd;
    struct sockaddr_in server_addr,client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[1024];


    //2.Creare socket UDP
    server_fd = socket(AF_INET,SOCK_DGRAM,0);
    if(server_fd<0){
        err_sys("Socket failed");
    }

    //3.Configurazione indirizzo
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr =  INADDR_ANY;
    server_addr.sin_port = htons(PORT);


    //4. Bind
    if(bind(server_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
        err_sys("bind failed");
    }

    printf("Server UDP listen on port %d\n",PORT);

    while(1){
        //5.RICEZIONE DEL MESSAGGIO 

        int bytes_received = recvfrom(server_fd,buffer,sizeof(buffer),0,(struct sockaddr*)&client_addr,&client_len);

         if (bytes_received > 0) {
            buffer[bytes_received] = '\0';  // Termina stringa
            printf("Ricevuto: %s\n", buffer);
            
            // TODO: Rispondere (NON write()!)
            char response[] = "ACK";
            sendto(server_fd, response, strlen(response), 0,
                   (struct sockaddr *)&client_addr, client_len);
        }
        close (server_fd);
        return 0 ;
    }


}