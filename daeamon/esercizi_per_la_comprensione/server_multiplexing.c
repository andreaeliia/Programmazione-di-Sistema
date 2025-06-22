#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>      // NECESSARIO per thread e mutex
#include <sys/select.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h> 
#include <stdarg.h>

#define NUM_PORTS 10
#define BASE_PORT 8080
#define MAX_CLIENTS 20
#define BUFFER_SIZE 1024

// ===== VARIABILI GLOBALI CONDIVISE TRA THREAD =====
// IMPORTANTE: Questi dati sono accessibili da entrambi i thread
int client_fds[MAX_CLIENTS];  // Array di socket client (condiviso)
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;  // Protezione per client_fds[]
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
volatile sig_atomic_t daemon_running = 1;

// ===== STRUTTURA PER PASSARE DATI AL THREAD I/O =====
// I thread non possono accedere alle variabili locali di altri thread
// Quindi passiamo i dati tramite questa struttura
typedef struct {
    int server_fds[NUM_PORTS];  // Array dei socket server (8080, 8081, 8082)
    int num_ports;              // Numero di porte (3 in questo caso)
} thread_data_t;



int create_socket (int port){
    int server_fd;
    struct sockaddr_in server_addr;
    int opt = 1;



    //1.CREAZIONE SOCKET TCP
    server_fd = socket(AF_INET,SOCK_STREAM,0);
    if(server_fd<0){
        perror("socket error");
        return -1;
    }


    //2.RIUTILIZZO INDIRIZZO (si evita l'errore adress already in use)
    setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    //3. Configurazione indirizzo Ip+porta
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    //4.BIND (associazione socket all'indirizzo)
    if(bind(server_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)
    {
        perror("bind failed");
        close(server_fd);
        return -1;
    }

    //5.SOCKET IN MODALITA ASCOLTO
    if(listen(server_fd,5)<0){
        perror("listen error");
        close(server_fd);
        return -1;
    }

    return server_fd;
    
}

//======Gestione di tutta la comunciazione di rete=====//
void* io_thread(void* arg){

    thread_data_t* data = (thread_data_t*)arg;  //CAST del parametro per accedere ai dati passati dal main


    //Varibili per select() che servono per la gestione dell'I/O multiplexing
    fd_set master_fds,read_fds;

    int max_fd = 0;   //FIle descriptor piu' alto che serve per il select

    daemon_log("🧵 Thread I/O avviato - gestisco comunicazione di rete\n");

    //======INIZIALIZZAZIONE DEL SELECT=====///
    FD_ZERO(&master_fds);   //pulizia del set

    //Aggiungiamo utti i server socket al set master
    for (int  i = 0; i < data->num_ports; i++)
    {
        FD_SET(data->server_fds[i],&master_fds);

        //Inoltre teniamo traccia del Fd piu' alto in quanto serve al select
        if(data->server_fds[i] >max_fd){
            max_fd = data ->server_fds[i];
        }

        daemon_log("🔌 Monitoraggio socket porta %d (fd: %d)\n", 
               BASE_PORT + i, data->server_fds[i]);
    }
    //MAIN LOOP DEL THREAD I/O
    while (1)
    {
        read_fds = master_fds; //Select ogni volta modifica il set, e' importante quindi copiarlo ogni volta


        daemon_log("Aspetto attivita sui socket...\n");

        //SElect blocco finao a quando almeno un socket ha un attivita
        int activity = select(max_fd +1,&read_fds,NULL,NULL,NULL);
        if(activity<0){
            perror("socket failed");
            break;
        }

         daemon_log("⚡ Attività rilevata su %d socket\n", activity);


         //GESTIONE NUOVE CONNESIONI 
         //controlla se qualche server scoket ha una nuova connessione
         for(int s = 0;s < data->num_ports;s++){
            if (FD_ISSET(data->server_fds[s],&read_fds))
            {
                daemon_log("Nuova connesione in arrivo su porta %d\n", BASE_PORT +s);

                //accetta la connessione
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);

                int new_client = accept(data->server_fds[s],(struct sockaddr*)&client_addr,&client_len);
                if( new_client<0){
                    perror("accept failed");
                    continue; 
                }
                daemon_log("✅ Client connesso da %s:%d (fd: %d)\n",
                       inet_ntoa(client_addr.sin_addr),
                       ntohs(client_addr.sin_port),
                       new_client);

                //====TROVA SLOT LIBERO PER IL CLIENT (THREAD_SAFE)=====//
                pthread_mutex_lock(&clients_mutex); //blocca accesso a client_fds[]



                int slot = -1;
                for (int i = 0; i < MAX_CLIENTS; i++)
                {
                    if(client_fds[i] == -1){
                        slot = i;
                        break;
                    }
                }
                if(slot == -1){
                    daemon_log("Troppi clienti connessi, rifiuto connessione\n");
                    close(new_client);
                }else{

                    client_fds[slot] = new_client;

                    //aggiungiamo il nuovo client al set per select
                    FD_SET(new_client,&master_fds);

                    //Aggioniamo max_fd se necessario?
                    if(new_client>max_fd){
                        max_fd =  new_client;
                    }

                    daemon_log("🎯 Client assegnato allo slot %d\n", slot);


                    //Inviare messagio di benvenuto
                    char welcome[100];
                    snprintf(welcome,sizeof(welcome),"BENVENUTO RICCHIONE.\nPORTA %d (slot %d)\n",BASE_PORT +s,slot);
                    send(new_client,welcome,strlen(welcome),0);

                }
                pthread_mutex_unlock(&clients_mutex);
                

            }
            
         }


         //GESTIONE MESSAGGI DAI CLIENT ESISTENI

        //CONTROLLO SE QUALCHE CLIENT HA INVIATO UN MESSAGGIO

        pthread_mutex_lock(&clients_mutex); //blocco del mutex
        
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            int client_fd = client_fds[i];  


            //SE QUESTO SLOT HA UN CLIENT ED HA INVIATO I DATI
            if(client_fd !=-1 && FD_ISSET(client_fd,&read_fds)){
                daemon_log("Client slot %d ha inviato un messaggio",i);

                char buffer[BUFFER_SIZE];
                memset(buffer,0,BUFFER_SIZE); //pulizia del buffer 

                //Ricevi il messaggio dal client
                ssize_t bytes = recv (client_fd,buffer,BUFFER_SIZE-1, 0);

                if(bytes <=0){
                    //chiudere connessione

                    daemon_log("Client slot %d disconnesso",i);
                    close(client_fd); //1.Chiusura del select
                    FD_CLR(client_fd,&master_fds); //2.rimossione da select
                    client_fds[i] = -1; //3.Rendiamo libero lo slot
                }else{
                    //Messagio ricevuto con successo
                    buffer[bytes] = '\0';  // Null-terminate la stringa
                    daemon_log("💬 Slot %d: %s", i, buffer);


                    //BREADCAST a tutti gli altri client
                    char broadcast_msg [BUFFER_SIZE + 50];
                    snprintf(broadcast_msg,sizeof(broadcast_msg),"[Client%d]: %s",i,buffer);



                    daemon_log("📡 Faccio broadcast a tutti gli altri client\n");


                    //Invia a tutti i client  connessi (eccetto il mittente)
                    int sent_count = 0;
                    for (int j = 0; j < MAX_CLIENTS; j++)
                    {
                        if(client_fds[j] != -1 &&  j!= i){
                            if(send(client_fds[j],broadcast_msg,strlen(broadcast_msg),0)>0){
                                sent_count++;
                            }
                        }
                    }
                    
                    // Conferma al mittente
                    char confirm[100];
                    snprintf(confirm, sizeof(confirm), 
                             "✅ Messaggio inviato a %d client\n", sent_count);
                    send(client_fd, confirm, strlen(confirm), 0);
                    
                    daemon_log("📊 Broadcast completato: %d destinatari\n", sent_count);

                }
            }
        }
        
    pthread_mutex_unlock(&clients_mutex);  // SBLOCCA accesso
    }
    


    daemon_log("🧵 Thread I/O terminato\n");
    return NULL;  // IMPORTANTE: thread function deve ritornare void*

}



//Funzioni per daemon
void daemon_log(const char *format, ...) {  // Aggiungi ...
    pthread_mutex_lock(&log_mutex);
    
    FILE *log = fopen("/tmp/daemon_server.log", "a");  // Cambia path
    if (log) {
        time_t now = time(NULL);
        char *timestr = ctime(&now);
        timestr[strlen(timestr)-1] = '\0';
        
        fprintf(log, "[%s] ", timestr);
        
        // AGGIUNGI QUESTA PARTE:
        va_list args;
        va_start(args, format);
        vfprintf(log, format, args);  // Usa vfprintf invece di fprintf
        va_end(args);
        
        fprintf(log, "\n");
        fflush(log);
        fclose(log);
    }
    
    pthread_mutex_unlock(&log_mutex);
}

void signal_handler(int sig) {
    switch(sig) {
        case SIGTERM:
            daemon_running = 0;  // Flag per terminazione
            break;
        case SIGINT:
            daemon_running = 0;  // Flag per terminazione
            break;
    }
}

void become_daemon() {
    pid_t pid = fork();
    if (pid > 0) {
        printf("Daemon creato con PID: %d\n", pid);
        exit(0);
    }
    if (pid < 0) {
        perror("fork");
        exit(1);
    }
    
    if (setsid() < 0) {
        perror("setsid");
        exit(1);
    }
    
    chdir("/");
    
    close(0); close(1); close(2);
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_WRONLY);
}
int main(){

    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);


    become_daemon();

    daemon_log("🚀 Avvio server multi-thread con I/O multiplexing\n");

     // ===== PREPARAZIONE DATI PER IL THREAD =====
    thread_data_t thread_data;  // Struttura per passare dati al thread
    pthread_t io_tid;           // ID del thread I/O


        // ===== INIZIALIZZAZIONE ARRAY CLIENT =====
    // IMPORTANTE: Inizializza l'array globale client_fds
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_fds[i] = -1;  // -1 significa "slot vuoto"
    }

    daemon_log("🔧 Array client inizializzato (%d slot disponibili)\n", MAX_CLIENTS);

    //CREAZIONE SOCKET SERVER//
    //Creazione di un socket per ogni porta
    for (int  i = 0; i < NUM_PORTS; i++)
    {
        thread_data.server_fds[i] =  create_socket(BASE_PORT+i);
        if(thread_data.server_fds[i] < 0){
            daemon_log("Errore creazione sulla porta %d",BASE_PORT+i);
            exit(1);
        }
        daemon_log("🔌 Socket creato sulla porta %d (fd: %d)\n", 
               BASE_PORT + i, thread_data.server_fds[i]);
    }
    thread_data.num_ports = NUM_PORTS;
    daemon_log("✅ Tutti i server socket creati (porte %d-%d)\n", 
           BASE_PORT, BASE_PORT + NUM_PORTS - 1);
    

    //CREAZIONE DEL THREAD I/O
    //IL thread I/O gestira tutta la comunicazione di rete
    daemon_log("🧵 Creo thread I/O...\n");
    if(pthread_create(&io_tid,NULL,io_thread,&thread_data)!=0){
        perror("pthread_create failed");
        exit(0);
    }

    daemon_log("📊 Avvio loop statistiche (ogni 10 secondi)\n");

    while (daemon_running)
    {
        sleep(10);


        //CALCOLA STATISTICA THREAD-SAFW
        pthread_mutex_lock(&clients_mutex);

        int connected = 0;

        daemon_log("STATISTICHE SERVER");


        //CONTEGGIO DEI CLIENT CONNESSI E DETTAGLI
        for (int i = 0;  i < MAX_CLIENTS;  i++)
        {
            if (client_fds[i] != -1)
            {
                connected++;
                daemon_log("   Slot %d: client connesso (fd: %d)\n", i, client_fds[i]);
            }
            
        }

        pthread_mutex_unlock(&clients_mutex);
        daemon_log("   📈 Totale client connessi: %d/%d\n", connected, MAX_CLIENTS);
        daemon_log("   🔌 Porte in ascolto: %d-%d\n", BASE_PORT, BASE_PORT + NUM_PORTS - 1);
        daemon_log("📊 === FINE STATISTICHE ===\n\n");
        

     
    }
       // ===== CLEANUP (mai raggiunto in questo esempio) =====
    // In un'applicazione reale, dovresti gestire segnali per terminazione pulita
    pthread_join(io_tid, NULL);  // Aspetta terminazione thread I/O
    
    // Chiudi tutti i socket
    for (int i = 0; i < NUM_PORTS; i++) {
        close(thread_data.server_fds[i]);
    }
    
    daemon_log("🏁 Server terminato\n");
    
}