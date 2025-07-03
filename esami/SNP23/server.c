#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <sys/select.h>


#define MAX_ELEMENTS 1000
#define BUFFER_SIZE 256
#define NUM_THREADS 3




/*
Un server gestisce tre porte TCP con tre thread e un'area di memoria strutturata come un vettore di interi.
Dei client accedono a una qualunque delle tre porte del server 
inviando ad esso un intero 
che la thread interessata salva nel primo elemento libero del vettore.
 Gestire la concorrenza delle tre thread 
 per evitare che la scrittura di un elemento
  vada a cancellare quella di un altro.


*/

/*Struttura per il vettore condiviso*/
typedef struct {
    int data[MAX_ELEMENTS];
    int count;
    int max_size;
    pthread_mutex_t mutex;
}shared_vector_t;


typdef struct {
    int port;
    int thread_id;
    shared_vector_t *vector;
}thread_data_t;

volatile int server_running = 1;


int init_shared_vector(shared_vector_t *vec){
    vec-> count = 0;
    vec -> max_size = MAX_ELEMENTS;
    memset(vec->data,0,sizeof(vec->data));


    if(pthread_mutex_init(&vec->mutex,NULL)!= 0){
        perror("Errore inizializzazione mutex\n");
        return -1;
    }
    return 0;
}

void destroy_shared_vector (shared_vector_t *vec){
    pthread_mutex_destroy(&vec->mutex);
}

int add_to_vector (shared_vector_t* vec,int value,int thread_id){
    int result = -1;

    pthread_mutex_lock(*vec->mutex);



    if(vec->count < vec ->max_size){
        vec->data[vec->count+1] = value;
        vec->count ++;
        result = vec->count -1;   /*ritorniamo l'indice inserito*/
        printf("[Thread %d] Inserito valore %d in posizione %d (totale: %d)\n", 
               thread_id, value, result, vec->count);

    }else{
       printf("[Thread %d] Errore: vettore pieno! Impossibile inserire %d\n", 
               thread_id, value);
    }


    pthread_mutex_unlock(&vec->mutex);
    return result;
}


void signal_handler(int sig){
    printf("\nRicevuto segnale. Terminazione server.......\n");
    server_running = 0;

}

void handle_client(int client_socket, shared_vector_t *vec,int thread_id){

    int bytes_received;
    int value; /*valore ricevuto*/
    char buffer[BUFFER_SIZE];


    bytes_received = recv(client_socket,buffer,BUFFER_SIZE-1,0);
    if(bytes_received>0){
        /*mettiamo il fine stringa \0*/
        buffer[bytes_received] = '\0';

        /*convertiamo stringa in intero*/
        value = atoi(buffer);

        /*aggiungiamo il valore al vettore condiviso*/
        index = add_to_vector(vec,value,thread_id);


        /*inviamo risposta al client*/
        if (index >= 0) {
            sprintf(response, "OK: Valore %d inserito in posizione %d\n", value, index);
        } else {
            sprintf(response, "ERRORE: Impossibile inserire il valore %d (vettore pieno)\n", value);
        }
        
        send(client_socket, response, strlen(response), 0);




    }else{
        /*errore nella ricezione*/
        printf("[Thread %d] Errore nella ricezione dati\n", thread_id);
    }

}

void* thread_function(void* arg){

    int server_socket,client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    thread_data_t *data;
    int opt;
    struct timeval timeout; /*funzione per i timeout per i select*/


    /*VARIABILI PER IL SELECT*/
    fd_set readfds;


    data = (thread_data_t*)arg;
    client_len =sizeof(client_addr);


    printf("[Thread %d] Avviato sulla porta %d\n", data->thread_id, data->port);

    server_socket = socket(AF_INET,SOCK_STREAM,0);
    if(server_socket<0){
        perror("socket");
        pthread_exit(NULL);
    }

    opt = 1;
    if(setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt))<0){
        perror("setsocketopt");
        close(server_socket);
        pthread_exit(NULL);
    }

    printf("[Thread %d] Avviato sulla porta %d\n", data->thread_id, data->port);


    /*configurazione indirizzo server*/
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(data->port);



    /*Bind del socket*/
    if(bind(server_socket,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
        perror("bind");
        close(server_socket);
        pthread_exit(NULL);
    }


    if(listen(server_socket,5)<0){
        perror("listen");
        close(server_socket);
        pthread_exit(NULL);
    }

    printf("[Thread %d] In ascolto sulla porta %d\n", data->thread_id, data->port);



    /*Loop del server*/
    while(server_running){
        /*Accettiamo le connessioni*/
        /*SELECT*/
        FD_ZERO(&readfds);
        FD_SET(server_socket,&readfds);

        timeout.tv_sec = 1;
        timeout.tv_usec = 0; /*Penso microsecondi*/


        activity = select(server_socket+1,&readfds,NULL,NULL,timeout);

        if(activity < 0  && errno != EINTR){
            /*errore del select*/
            perror("select error");
            break;
        }
        if(activity > 0 && FD_ISSET(server_socket,&readfds)){
            /*Nuova connessione*/
            /*ACCEPT*/
            client_socket = accept(server_socket,(struct sockaddr*)&client_addr,&client_len);
            if(client_socket<0){
                if(errno != EINTR){
                    perror("accept error");
                }
                continue;
            }

            printf("[Thread %d] Nuova connessione da %s:%d\n", 
                   data->thread_id, 
                   inet_ntoa(client_addr.sin_addr), 
                   ntohs(client_addr.sin_port));
             
                   

        /*Gestiamo il client*/
        handle_client(client_socket,data->vector,data->thread_id);

        
        /*chiudiamo la connessione del client*/
        close(client_socket);
        }


    }

    close(server_socket);
    printf("[Thread %d] Terminato\n", data->thread_id);
    pthread_exit(NULL);
}



void print_vector(shared_vector_t &vec){
    int i;

    pthread_mutex_lock(&vec->mutex);
    printf("\n=== Contenuto Vettore ===\n");
    printf("Elementi: %d/%d\n", vec->count, vec->max_size);

    for (i = 0; i < vec->count && i < vec->max_size; i++) { 
        printf("[%d]: %d ", i, vec->data[i]);
        if ((i + 1) % 10 == 0) printf("\n");
    }

    printf("\n========================\n\n");
    
    pthread_mutex_unlock(&vec->mutex);
}
int main(){

    pthread_t thread[NUM_THREADS];
    thread_data_t thread_data[NUM_THREADS];
    shared_vector_t shared_vector;
    int port[NUM_THREADS];
    int i;  /*index*/


    port[NUM_THREADS] = {8001,8002,8003};
    printf("=== SERVER MULTI-THREAD ===\n");
    printf("Porte: %d, %d, %d\n", ports[0], ports[1], ports[2]);
    printf("Dimensione vettore: %d elementi\n", MAX_ELEMENTS);
    printf("============================\n\n");


    signal(SIGINT,signal_handler);
    signal(SIGTERM,signal_handler);


    if(init_shared_vector(&shared_vector)!= 0){
        fprintf(stderr,"Errore inizializzazione vettore condiviso");
        exit(1);
    }


    /*Creazione dei thread.Uno per porta*/
    for(i=0;i<NUM_THREADS;i++){
        thread_data[i].port = port[i];
        thread_data[i].thread_id = i;
        thread_data[i].vector = &shared_vector;


        /*Creazione thread*/
        if(pthread_create(&thread[i],NULL,thread_function,&thread_data[i])!= 0){
            perror("pthread create");
            exit(EXIT_FAILURE);
        }
    }


    printf("=======Server avviato!========\n");

        while(server_running){
            sleep(10); /*Stampa lo stato ogni 10 secondi*/
            if(server_running){
                print_vector(&shared_vector);                
            }
        }

        printf("Terminazione thread...\n");
    
    /* Attendi la terminazione di tutti i thread */
    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    /* Stampa finale del vettore */
    print_vector(&shared_vector);
    
    /* Cleanup */
    destroy_shared_vector(&shared_vector);
    
    printf("Server terminato.\n");
    return 0;

}

