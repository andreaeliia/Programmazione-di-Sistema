/*
Un processo client ha due thread che cambiano di continuo a intervalli
di tempo casuali il valore di una variabile d'ambiente A, assegnandole
un numero intero casuale. 

Gestire la concorrenza delle operazioni svolte dalle thread e fare in
modo che un'altra thread comunichi via TCP a un server ogni valore
modificato della variabile d'ambiente A. Il server, a sua volta, dovrà
provedere ad aggiornare ad ogni ricezione il valore della sua variabile
d'ambiente A.

I due processi devono procedere fino a quando il client non sia
interrotto. Non appena ciò accade, anche il server deve essere
interrotto.

Per la sperimentazione usare server e client sulla stessa macchina.*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <time.h>  
#include <signal.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

/*==========VARIABILI GLOBALI==============*/
pthread_t A_thread1;
pthread_t A_thread2;
pthread_t connection_thread;
int A; /*Variabile d'ambiente*/
int continua_esecuzione = 1;
pthread_mutex_t A_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t A_changed_cond = PTHREAD_COND_INITIALIZER; // Condizione per notificare il cambiamento
volatile int A_changed = 0;

/*========UTILS========*/
int random_value(){    
    int random_number;
    random_number = rand() % 10000;
    return random_number;
}

/*==========THREAD==============*/
void* change_A(void* arg){
    char name[32];
    char number[32];
    int random_time;
    int thread_id;

    thread_id = *(int*)arg;
    sprintf(name,"A");

    while (continua_esecuzione) {
        random_time = (rand() % 2)+1;

        pthread_mutex_lock(&A_mutex);

        /*Cambiamo il valore di A*/
        A = random_value();
        sprintf(number, "%d", A);
        
        /*Settarla come variabile d'ambiente*/
        if(setenv(name,number,1) == -1){
            perror("setenv error");
        } else {
            printf("%d | A | %s\n",thread_id, getenv("A"));
        }
        
        // Notifica che A è cambiata
        A_changed = 1;
        pthread_cond_signal(&A_changed_cond); // Risveglia il thread di connessione
        
        pthread_mutex_unlock(&A_mutex);

        sleep(random_time);
    }

    return NULL;
}

/*==============SERVER================*/
int connessione_server(const char* indirizzo, int porta){
    int sock;
    struct sockaddr_in server_addr;

    sock = socket(AF_INET,SOCK_STREAM,0);
    if(sock<0){
        printf("Errore creazione socket\n");
        return -1;
    }

    /*Configurazione indirizzo server*/
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(porta);

    if (inet_aton(indirizzo, &server_addr.sin_addr) == 0) {
        printf("Indirizzo IP non valido: %s\n", indirizzo);
        close(sock);
        return -1;
    }

    /*Connessione*/
    printf("Connessione al %s:%d...\n",indirizzo,porta);
    if(connect(sock, (struct sockaddr*)&server_addr,sizeof(server_addr))<0){
        printf("Errore connessione al server\n");
        close(sock);
        return -1;
    }    

    printf("Connesso con successo...!\n");
    return sock;
}

int send_message(int sock){
    char messaggio[50];
    char buffer[BUFFER_SIZE];
    int bytes;
    
    pthread_mutex_lock(&A_mutex);
    
    // Aspetta che A cambi
    while (!A_changed && continua_esecuzione) {
        pthread_cond_wait(&A_changed_cond, &A_mutex);
    }
    
    if (!continua_esecuzione) {
        pthread_mutex_unlock(&A_mutex);
        return -1;
    }
    
    // Invia il valore corrente di A
    sprintf(messaggio,"%d\n",A);   
    A_changed = 0; // Reset della flag
    
    pthread_mutex_unlock(&A_mutex);
    
    if(send(sock,messaggio,strlen(messaggio),0) >= 0){
        printf("Inviato: %s", messaggio);
    } else {
        printf("Errore invio messaggio\n");
        return -1;
    }

    /*Riceviamo l'echo del server*/
    bytes = recv(sock, buffer, BUFFER_SIZE-1, 0);
    if(bytes > 0){
        buffer[bytes] = '\0';
        printf("Echo ricevuto: %s", buffer);
    } else if (bytes == 0) {
        printf("Server ha chiuso la connessione\n");
        return -1;
    } else {
        printf("Errore ricezione echo\n");
        return -1;
    }
    
    return 0;
}

/*=============SERVER THREAD===========*/
void* connection_thread_func(void* arg){
    int sock;

    sock = connessione_server(SERVER_IP,SERVER_PORT);
    if (sock < 0) {
        printf("Impossibile connettersi al server\n");
        continua_esecuzione = 0;
        return NULL;
    }
    
    while(continua_esecuzione){
        if (send_message(sock) < 0) {
            break;
        }
    }

    close(sock);
    return NULL;
}

/* ========== GESTIONE SEGNALI ========== */
void signal_handler(int sig) {
    printf("\n[Processo A] Ricevuto segnale %d. Terminazione...\n", sig);
    continua_esecuzione = 0;
    
    // Risveglia tutti i thread in attesa
    pthread_cond_broadcast(&A_changed_cond);
}

int main(){
    int thread_id1 = 1;
    int thread_id2 = 2;

    /*Seed random con PID*/
    srand(getpid());
    signal(SIGINT,signal_handler);
    signal(SIGTERM,signal_handler);

    pthread_create(&connection_thread,NULL,connection_thread_func,NULL);
    pthread_create(&A_thread1,NULL,change_A,&thread_id1);
    pthread_create(&A_thread2,NULL,change_A,&thread_id2);

    /*aspetta tutti i thread*/
    pthread_join(A_thread1,NULL);
    pthread_join(A_thread2,NULL); 
    pthread_join(connection_thread,NULL);
    
    pthread_mutex_destroy(&A_mutex);
    pthread_cond_destroy(&A_changed_cond);

    return 0;
}