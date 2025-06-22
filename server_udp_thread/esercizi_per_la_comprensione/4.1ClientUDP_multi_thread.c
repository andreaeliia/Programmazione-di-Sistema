#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERVER_IP "127.0.0.1"
#define PORT_A 8080
#define PORT_B 8081

//Struttura per passare i dati al thread
typedef struct 
{
    int thread_id;
    int port;
}thread_data_t;


//thread per client
void* client_thread(void* arg){
    thread_data_t* data = (thread_data_t*)arg;
    int client_fd;
    struct sockaddr_in server_addr;
    char dummy = 'X';
    char received_char;
    socklen_t server_len=  sizeof(server_addr);
    

    //Creazione socker UDP
    client_fd = socket(AF_INET,SOCK_DGRAM,0);
    if(client_fd<0){
        err_sys("socket error");
    }

    //Configurazione indirizzo
    server_addr.sin_family = AF_INET;
    server_addr.sin_port =  htons(data -> port); 
    inet_pton(AF_INET,SERVER_IP,&server_addr.sin_addr); 
    

    for(int i =0;i<3;i++){
        printf("Thread %d (porta %d): invio richiesta %d\n",data->thread_id,data->port,i+1);


        //INVIARE RICHIESTA 
        sendto(client_fd,&dummy,1,0,(struct sockaddr*)&server_addr,sizeof(server_addr));
        

        //RICEVERE RISPOSTA
        int bytes = recvfrom(client_fd,&received_char,1,0,(struct sockaddr*)&server_addr,&server_len);

          if (bytes > 0) {
            printf("Thread %d ricevuto: '%c'\n", data->thread_id, received_char);
        }
        
        sleep(1);
    
    }
    close(client_fd);
    return 0;
}


int main(void){
    //dati thread
    pthread_t thread1,thread2;
    thread_data_t data1 = {1,PORT_A};
    thread_data_t data2 = {2,PORT_B};

    printf("Client UDP multi-thread start");

    pthread_create(&thread1,NULL,client_thread,&data1);
    pthread_create(&thread2,NULL,client_thread,&data2);

    pthread_join(thread1,NULL);
    pthread_join(thread2,NULL);

    printf("Client terminated \n");

    return 0;
    

}

