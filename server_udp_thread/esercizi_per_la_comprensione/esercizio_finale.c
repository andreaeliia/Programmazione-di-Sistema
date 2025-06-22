#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>

#define SERVER_IP "127.0.0.1"
#define PORT_A 8080
#define PORT_B 8081


typedef struct 
{
    int thread_id;
    int port;
}thread_data_t;

sem_t *semaforo;


void* client_thread(void* arg){
    thread_data_t* data = (thread_data_t*)arg;
    int client_fd;
    struct sockaddr_in server_addr;
    char dummy = 'X';
    char recevied_char;
    socklen_t server_len =sizeof(server_addr);


    //Creazione socket UDP
    client_fd = socket(AF_INET,SOCK_DGRAM,0);
    if(client_fd<0){
        err_sys("socket failed");
    }

    //Configurazione indirizzo IP
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(data->port);
    inet_pton(AF_INET,SERVER_IP,&server_addr.sin_addr);


    for (int i = 0; i < 3; i++)
    {
        printf("Thread %d : richiedo accesso",data->thread_id);
        
        sem_wait(semaforo);

         printf("Thread %d: ACCESSO CONCESSO - comunico con porta %d\n", 
               data->thread_id, data->port);
        
        // Comunicazione con server
        sendto(client_fd, &dummy, 1, 0, 
               (struct sockaddr *)&server_addr, sizeof(server_addr));
        
        int bytes = recvfrom(client_fd, &recevied_char, 1, 0,
                           (struct sockaddr *)&server_addr, &server_len);
        
        if (bytes > 0) {
            printf("Thread %d ricevuto: '%c'\n", data->thread_id, recevied_char);
        }
        
        // Simula elaborazione
        sleep(1);
        
        printf("Thread %d: rilascio accesso\n", data->thread_id);
        
        // TODO: Rilascia il semaforo
        sem_post(semaforo);
        // === SEZIONE CRITICA - FINE ===
        
        // Pausa tra comunicazioni (fuori dalla sezione critica)
        sleep(1);





    }
    close(client_fd);
    return NULL;
    
}


int main (void){
    pthread_t thread1, thread2;
    thread_data_t data1 = {1, PORT_A};
    thread_data_t data2 = {2, PORT_B};
    
    // TODO: Crea semaforo con valore 1 (un solo thread alla volta)
    semaforo = sem_open("/semaforo", O_CREAT, 0644,1);
    if (semaforo == SEM_FAILED) {
        err_sys("sem_open failed");
    }
    
    printf("Avvio client UDP sincronizzato\n");
    
    pthread_create(&thread1, NULL, client_thread, &data1);
    pthread_create(&thread2, NULL, client_thread, &data2);
    
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    // TODO: Pulizia semaforo
    sem_close(semaforo);
    sem_unlink("/semaforo");
    
    printf("Client terminato\n");
    return 0;
}
