
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <pthread.h>
#include <signal.h>

#define NUM_SOCKETS 10
#define BASE_PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS_PER_SOCKET 5
#define MAX_TOTAL_CLIENTS (NUM_SOCKETS * MAX_CLIENTS_PER_SOCKET)

//==========================================//
//Creare un daemon che apra 10 listening sockets TCP e una cui thread utilizzi l'I/O multiplexing per gestire i dati in arrivo ai socket. Testare con un client il funzionamento del server.//
//==========================================//


typedef struct {
    int server_socket[NUM_SOCKETS];
    int client_sockets[MAX_TOTAL_CLIENTS];
    int client_to_server[MAX_TOTAL_CLIENTS];
    pthread_mutex_t mutex;
} server_data_t;


server_data_t server_data;
volatile int running =1;


void signal_handler(int sign){
    printf("Ricevuto segnale %d, arresto il server....\n",sign);
    running = 0;
}


int create_listening_socket (int port){
    //FUNZIONE CHE 
    //1.CREA SOCKET
    //2.RIUTILIZZA INDIRIZZO IP
    //3. CONFIGURA INDIRIZZZO IP
    //4.BINDA
    //5.LISTEN
    int server_fd;
    struct sockaddr_in server_addr;
    int opt =1;
    

    //=========1.CREAZIONE SOCKET=====//
    server_fd = socket(AF_INET,SOCK_STREAM,0);
    if(server_fd<0) {
        perror("socket error");
        return -1;
    }

    //=======2.RIUTILIZZO SOCKET======//
    if(setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt))<0){
        perror("setsocketopt failed");
        close(server_fd);
        return -1;
    }


    //=======3.CONFIGURAZIONE INDIRIZZO IP=====//
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);



    //=====4.BIND=====//
    if(bind(server_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
        printf("bind failed on port %d\n",port);
        perror("");
        close(server_fd);
        return -1;
    }


    //====5.LISTEN =====//


    if(listen(server_fd,5)<0){
        perror("listen error");
        close(server_fd);
        return -1;
    }

    return server_fd;

}



void* multiplexing_thread(void *arg){
    fd_set read_fs;
    int max_fd,activity,i,j;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];


    printf("🔄 Thread I/O multiplexing avviato\n");


    while(running){
        //PREPARIAMO IL SET DI FILE DESCRIPTOR
        FD_ZERO(&read_fs);
        max_fd = 0;


        pthread_mutex_lock(&server_data.mutex);


        //aggiuniamo tutti i server socket
        for (int  i = 0; i < NUM_SOCKETS; i++)
        {
            if(server_data.server_socket[i] != -1){   //diverso da -1 = libero
                FD_SET(server_data.server_socket[i],&read_fs);
                if(server_data.server_socket[i] > max_fd){
                    max_fd = server_data.server_socket[i];
                }
            }
        }
       // Aggiungi tutti i client socket
        for (i = 0; i < MAX_TOTAL_CLIENTS; i++) {
            if (server_data.client_sockets[i] != -1) {
                FD_SET(server_data.client_sockets[i], &read_fs);
                if (server_data.client_sockets[i] > max_fd) {
                    max_fd = server_data.client_sockets[i];
                }
            }
        }
        
            pthread_mutex_unlock(&server_data.mutex);
        
        if (max_fd == 0) {
            usleep(100000);  // 100ms
            continue;
        }


        //select con timeout
        struct timeval timeout = {1,0};  //1 secondo

        activity = select(max_fd +1,&read_fs,NULL,NULL,&timeout);

        if(activity < 0 ){
            if (running) perror("select failed");
            continue;
        }

        if(activity == 0){
            //timeout - continua
            continue;
        }
        
        pthread_mutex_lock(&server_data.mutex);


        //controllo delle nuove connessioni
        for(i=0;i<NUM_SOCKETS;i++){
            int server_fd = server_data.server_socket[i];
            if(server_fd !=-1 && FD_ISSET(server_fd,&read_fs)){ //se server_fd e' diverso da 1)(cioe non e' libero e ce una nuova connessione)
                int new_client = accept(server_fd,(struct sockaddr*)&client_addr,&client_len);
                if(new_client >= 0){
                    //Troviamo uno slot libero per il client
                    int slot_found = 0;
                    for (int j = 0; j < MAX_TOTAL_CLIENTS; j++)
                    {
                        if(server_data.client_sockets[j] == -1){
                            server_data.client_sockets[j] = new_client;
                            server_data.client_to_server[j] = i;  //RICORDIAMO DA QUALE SERVER
                            slot_found = 1;

                             printf("✅ Nuova connessione sulla porta %d: %s:%d → slot %d\n",
                                   BASE_PORT + i,
                                   inet_ntoa(client_addr.sin_addr),
                                   ntohs(client_addr.sin_port),
                                   j);
                            break;
                        }
                    }
                    if(!slot_found ){
                        printf("Troppi client, connessione rifiutata\n");
                        close(new_client);
                    }
                    
                }
            }
        }

        //GESTIAMO L'attivita del client
        for (int i = 0; i < MAX_TOTAL_CLIENTS; i++)
        {
            int client_fd = server_data.client_sockets[i];

            if(client_fd !=-1 && FD_ISSET(client_fd,&read_fs)){
                ssize_t bytes_read = recv(client_fd,buffer,BUFFER_SIZE-1,0);

                if(bytes_read <= 0){  //Client disconnesso
                    int server_idx = server_data.client_sockets[i];
                    printf("❌ Client slot %d (porta %d) disconnesso\n", 
                           i, BASE_PORT + server_idx);
                    close(client_fd);
                    server_data.client_sockets[i] = -1;
                }else{  //dati ricevuti
                    buffer[bytes_read] = "\0"; //Cosa vuol dire?
                    int server_idx = server_data.client_to_server[i];
                    
                    printf("📨 Porta %d, Slot %d: %s", 
                           BASE_PORT + server_idx, i, buffer);


                    //risposta 
                    char response[BUFFER_SIZE];
                    snprintf(response, BUFFER_SIZE, 
                            "[Porta %d, Slot %d] %s", 
                            BASE_PORT + server_idx, i, buffer);
                    send(client_fd, response, strlen(response), 0);
                }
            }
        }
        pthread_mutex_unlock(&server_data.mutex); 
    }

    printf("🔄 Thread I/O multiplexing terminato\n");
    return NULL;

}

int main (void){
    pthread_t multiplexing_tid;
    int i;

    printf("======SERVER MULTI_SOCKET CON THREAD=======");


    //GESTIONE DEI SEGNALI : perche' questa cosa?
    signal (SIGINT,signal_handler);
    signal(SIGTERM,signal_handler);

    //INIZIALLIZZAZIONE DELLA STRUTTURA DATi
    pthread_mutex_init(&server_data.mutex,NULL);

    for(int i = 0;i<MAX_TOTAL_CLIENTS;i++){
        server_data.client_sockets[i] = -1;
        server_data.client_to_server[i] = -1;
    }

    //CREAZIONE DEI 10 SOCKET CON LISTENING

    printf("Creazione di %d socket listening  \n",NUM_SOCKETS);
    for(int i = 0; i< NUM_SOCKETS;i++){
        int port = BASE_PORT + 1;
        server_data.client_sockets[i] = create_listening_socket(port);


        if(server_data.client_sockets[i] == -1){  //cioe e' occupato
            printf("❌ Impossibile creare socket sulla porta %d\n", port);
        }
    }


    printf("\n🎯 Server pronto! Porte attive: ");
    for (i = 0; i < NUM_SOCKETS; i++) {
        if (server_data.server_socket[i] != -1) {
            printf("%d ", BASE_PORT + i);
        }
    }

    printf("\n");





}