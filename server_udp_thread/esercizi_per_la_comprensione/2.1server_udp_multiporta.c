#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <stdlib.h>
#include <time.h>

#define PORT_A 8080
#define PORT_B 8081


int main (void){
    int socket_A,socket_B;
    struct sockaddr_in addr_A,addr_B,client_addr;
    fd_set readfds;
    int max_fd;
    socklen_t client_len = sizeof(client_addr);
    char buffer [1024];

    srand(time(NULL));

    //1.CREAZIONE SOCKET A E B
    socket_A = socket(AF_INET,SOCK_DGRAM,0);
    if(socket_A < 0) err_sys("socket A failed");

    socket_B = socket(AF_INET,SOCK_DGRAM,0);
    if(socket_B< 0) err_sys("socket B failed");


    //2.CONFIGURAZIONE INDIRIZZI A E B
    addr_A.sin_family = AF_INET;
    addr_A.sin_addr.s_addr = INADDR_ANY;
    addr_A.sin_port = htons(PORT_A);

    addr_B.sin_family = AF_INET;
    addr_B.sin_addr.s_addr = INADDR_ANY;
    addr_B.sin_port = htons(PORT_B);


    //3.BIND SOCKET A E B
    if(bind(socket_A,(struct sockaddr *)&addr_A,sizeof(addr_A))<0){
        err_sys("bind A failed");
    }

    if(bind(socket_B,(struct sockaddr *)&addr_B,sizeof(addr_B))<0){
        err_sys("bind B failed");
    }


    printf("Server UDP in ascolto su:\n");
    printf("  Porta A: %d\n", PORT_A);
    printf("  Porta B: %d\n", PORT_B);

    max_fd = (socket_A > socket_B) ? socket_A : socket_B;
    

    //4.while loop
    while(1){
        FD_ZERO(&readfds);
        FD_SET(socket_A, &readfds);
        FD_SET(socket_B, &readfds);


        //5.Chiamata select()
        int activity = select(max_fd+1,&readfds,NULL,NULL,NULL);
        if(activity<0){
            err_sys("select error");
        }

        //6.Check porte A_B
         // TODO: Controllare porta A
        if (FD_ISSET(socket_A, &readfds)) {
            int bytes = recvfrom(socket_A, buffer, sizeof(buffer), 0,
                               (struct sockaddr *)&client_addr, &client_len);
            if (bytes > 0) {
                char random_char = 'A' + (rand() % 26);
                sendto(socket_A, &random_char, 1, 0,
                      (struct sockaddr *)&client_addr, client_len);
                printf("Porta A: inviato '%c'\n", random_char);
            }
        }

         // TODO: Controllare porta A
        if (FD_ISSET(socket_B, &readfds)) {
            int bytes = recvfrom(socket_B, buffer, sizeof(buffer), 0,
                               (struct sockaddr *)&client_addr, &client_len);
            if (bytes > 0) {
                char random_char = 'A' + (rand() % 26);
                sendto(socket_B, &random_char, 1, 0,
                      (struct sockaddr *)&client_addr, client_len);
                printf("Porta B: inviato '%c'\n", random_char);
            }
        }
    }
    close(socket_A);
    close(socket_B);
}