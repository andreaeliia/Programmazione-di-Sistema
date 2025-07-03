#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#define NIST_SERVER "time.nist.gov"
#define NIST_PORT 13
#define BUFFER_SIZE 256
#define SLEEP_INTERVAL 30
/*
1) La prima thread visita, alla porta TCP 13 con periodo di 30 secondi,
uno dei server elencati nell'allegato documento "NIST Internet Time
Service.pdf", usando il nome DNS del server, e registra, aggiornandola
ogni volta, la data restituita dall'Internet Time Service in un area di
memoria.

2) L'altra thread legge con la stessa periodicità la data contenuta
nell'area di memoria e se il divario tra la data presente in memoria e
quella restituita dal sistema è maggiore di 30 secondi, stampa un avviso
al terminale.
*/ 


/*Struttura memoria condivisa*/
typedef struct {
    time_t nist_time;
    int valid;
    pthread_mutex_t mutex;
}SharedData;

//Variabile globale
SharedData  shared_data = {0,0,PTHREAD_MUTEX_INITIALIZER};

volatile int terminate_flag = 0;



void signal_handler(int sig){
    printf("Terminazione in corso \n");
    terminate_flag = 1 ;
}



/*Funzione per connettersi al server NIst ed ottenere l'orario*/
time_t get_nist_time(const char* server, int port){
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *host_entry;
    char buffer[BUFFER_SIZE];
    int bytes_received;
    time_t nist_timestamp;
    struct tm tm_time;
    char *line_end;


    /*RISOLUZIONE NOME HOST*/

    host_entry = gethostbyname(server);
    if(host_entry == NULL){
        fprintf(stderr, "Errore impossibile risolvere %s\n",server);
        return -1;
    }


    /*Creazione del socket*/
    sockfd =  socket(AF_INET,SOCK_STREAM,0);
    if(sockfd < 0){
        perror("Errore creazione socket");
        return -1;
    }

    /*CONFIGURAZIONE INDIRIZZO SERVER*/
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr.s_addr,host_entry->h_addr_list[0],host_entry->h_length);

    /*Connessione al server*/
    if(connect(sockfd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
        perror("Errore di connessione");
        close(sockfd);
        return -1;
    }

    /*Recezione dati dal server*/
    bytes_received = recv(sockfd,buffer, BUFFER_SIZE-1,0);
    close(sockfd);


    buffer[bytes_received] = '\0';

    printf("%s\n",buffer);

    /*Rimozione caratteri di fine riga*/
    /*60857 25-07-01 09:31:56 50 0 0   2.2 UTC(NIST) * */
        /* Rimozione caratteri di fine riga */

    
    printf("Ricevuto dal server NIST: %s\n", buffer);


    memset(&tm_time,0,sizeof(tm_time));

    if(sscanf(buffer,"%*d %d-%d-%d %d:%d:%d",
        &tm_time.tm_year,&tm_time.tm_mon,&tm_time.tm_mday,
        &tm_time.tm_hour,&tm_time.tm_min,&tm_time.tm_sec)   == 6){
            

             /* Correzione valori tm */
        tm_time.tm_year += 100;  /* Anno base 1900, formato YY -> 20YY */
        tm_time.tm_mon -= 1;     /* Mese 0-11 */
        tm_time.tm_isdst = -1;   /* Lascia che mktime determini DST */
        
        /* Conversione a timestamp */
        nist_timestamp = mktime(&tm_time);
        
        if (nist_timestamp != -1) {
            return nist_timestamp;
        }
    }

    fprintf(stderr, "Errore: Impossibile interpretare la risposta del server\n");
    return -1;



}


void* nist_thread(void* arg){
    time_t current_nist_time;


    printf("Thread NIST avviato\n");

    while (!terminate_flag)
    {
        current_nist_time = get_nist_time(NIST_SERVER,NIST_PORT);
        if(current_nist_time!= -1){
            pthread_mutex_lock(&shared_data.mutex);
            shared_data.nist_time = current_nist_time;
            shared_data.valid = 1;
            pthread_mutex_unlock(&shared_data.mutex);


            printf("Orario NIST aggiornato : %s\n",ctime(&current_nist_time));
        }else{
            printf("Errore in get_nist_time\n");
        }    
        sleep(SLEEP_INTERVAL);
    }
    printf("Thread NIST terminato\n");
    return NULL;

}


void* monitor_thread(void* arg){
    time_t system_time;
    time_t nist_time; 
    int nist_valid;
    double time_diff;


    while(!terminate_flag){

        /*lettura orario sistema*/
        time(&system_time);


        /*lettura area memoria condivisa*/

        pthread_mutex_lock(&shared_data.mutex);
        nist_time = shared_data.nist_time;
        nist_valid = shared_data.valid;
        pthread_mutex_unlock(&shared_data.mutex);



        if(nist_valid){
            time_diff = difftime(system_time,nist_time);
            printf("Controllo: Sistema=%s", ctime(&system_time));
            printf("          NIST   =%s", ctime(&nist_time));
            printf("          Differenza: %.1f secondi\n", time_diff);


            if(time_diff>30.0 || time_diff < -30.0){
                printf("AVVISO DIFFERENZA ORARIO SUPERIORE AI 30 SECONDI \n");
                printf("DIFFERENZA : %.1f secondi\n",time_diff);
            }
        }else{
            printf("Nessun controllo valido disponibile\n");
        }
        printf("-----------------------");
        sleep(SLEEP_INTERVAL);
    }
        printf("THREAD MONITOR TERMINATO\n");
        return NULL;
    }






int main(){
    pthread_t nist_tid,monitor_tid;
    int ret;
    time_t date;



    printf("=== NIST Internet Time Service Client ===\n");
    printf("Server: %s:%d\n", NIST_SERVER, NIST_PORT);
    printf("Intervallo: %d secondi\n", SLEEP_INTERVAL);
    printf("Premere Ctrl+C per terminare\n\n");


    signal(SIGINT, signal_handler);
    signal(SIGTERM,signal_handler);


    /*inizializzazione mutex*/
    if(pthread_mutex_init(&shared_data.mutex,NULL)!= 0){
        fprintf(stderr,"Errore inizializzazione mutex");
        return 1;
    }


     /*Creazione thread Nist*/
     ret=pthread_create(&nist_tid,NULL,nist_thread,NULL);
     if(ret!=0){
        fprintf(stderr,"Errore creazione threas NIST: %s\n",strerror(ret));
        terminate_flag =1;
        return 1;
     }


      /*Creazione thread Nist*/
     ret=pthread_create(&monitor_tid,NULL,monitor_thread,NULL);
     if(ret!=0){
        fprintf(stderr,"Errore creazione threas monitor: %s\n",strerror(ret));
        terminate_flag = 1;
        pthread_join(nist_tid,NULL);
        return 1;
     }


     /*Terminazione thread Attesa*/
     pthread_join(nist_tid,NULL);
     pthread_join(monitor_tid,NULL);

     /*Pulizia mutex*/

     pthread_mutex_destroy(&shared_data.mutex);

     printf("Programma terminato\n");


    return 0;




}



