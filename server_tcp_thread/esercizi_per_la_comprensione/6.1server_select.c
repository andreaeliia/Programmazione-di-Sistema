#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <time.h>

#define PORT 8080
#define MAX_CLIENTS 2

int main(void){
    int server_fd;
    int client_socket [MAX_CLIENTS];
    struct sockaddr_in address;
    fd_set readfds;
    int max_fd;
    int activity;
    int i;
    srand(time(NULL));

    //Inizializzazione dell'array
    for(i=0;i<MAX_CLIENTS; i++){
        client_socket[i] = 0;
    }

    //setup server
    server_fd = socket(AF_INET,SOCK_STREAM,0);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd,3);


    printf("Server in ascolto sulla porta %d\n",PORT);

    while(1){
        //TODO: Preparare set per select
        FD_ZERO(&readfds);
        FD_SET(server_fd,&readfds);
        max_fd = server_fd;

        //TODO : Aggiungere client socket al set
        for(i=0;i<MAX_CLIENTS;i++){
            if(client_socket[i]>0){
                FD_SET(client_socket[i],&readfds);
            }
            if(client_socket[i]> max_fd){
                max_fd= client_socket[i];
            }
        }
            
            //TODO: chiamare select
            activity = select(max_fd+1,&readfds,NULL,NULL,NULL);

            if(activity < 0 ){
                err_sys("select error");
            }

            //TODO: Controllare se server socket ha nuove connessioni

            if(FD_ISSET(server_fd,&readfds)){
                int new_socket = accept(server_fd,NULL,NULL);
                printf("Nuova connessione: socket %d\n",new_socket);


                for(i=0;i<MAX_CLIENTS;i++){
                    if(client_socket[i]== 0 ){
                        client_socket[i] =  new_socket;
                        break;
                    }
                }

            }


            //TODO: Controllare client socket per dati
            for(i=0;i<MAX_CLIENTS;i++){
                int sd =  client_socket[i];
                if(FD_ISSET(sd,&readfds)){
                    char buffer[1];
                    int valread = read(sd, buffer,1);


                    if(valread == 0){
                        //client disconesso
                        printf("Client %d disconesso\n",sd);
                        close(sd);
                        client_socket[i] = 0;
                    }else {
                        char random_char = 'A' + (rand() % 26);
                        send(sd, &random_char, 1, 0);
                        printf("Inviato '%c' al client %d\n", random_char, sd);
                    }
                }
            }

        }





return 0;

}