#include "apue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <sys/times.h>

#define PORT 8080
#define BUFFER_SIZE 1024


/*========UTILS========*/
int random_value(){    
    int random_number;
    random_number = rand() % 10000;
    return random_number;
}

/*===============STUTTURA PER THREAD==================*/
typedef struct {
    int client_fd;
    struct sockaddr_in client_addr;
    int thread_id;
} ThreadData;

/*==============VARIBILI GLOBALE=====================*/



/*========ESEMPIO PER CALCOLARE IL TEMPO PER IL THREAD========*/
static void
pr_times(clock_t real, struct tms *tmsstart, struct tms *tmsend)
{
	static long		clktck = 0;

	if (clktck == 0)	/* fetch clock ticks per second first time */
		if ((clktck = sysconf(_SC_CLK_TCK)) < 0)
			err_sys("sysconf error");

	printf("  real:  %7.7f\n", real / (double) clktck);
	printf("  user:  %7.7f\n",
	  (tmsend->tms_utime - tmsstart->tms_utime) / (double) clktck);
	printf("  sys:   %7.7f\n",
	  (tmsend->tms_stime - tmsstart->tms_stime) / (double) clktck);
	printf("  child user:  %7.7f\n",
	  (tmsend->tms_cutime - tmsstart->tms_cutime) / (double) clktck);
	printf("  child sys:   %7.7f\n",
	  (tmsend->tms_cstime - tmsstart->tms_cstime) / (double) clktck);
}
/*=================THREAD===================*/
void* gestisci_client_thread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    int client_fd = data->client_fd;
    int thread_id = data->thread_id;
    char buffer[BUFFER_SIZE];
    int i;
    int A;
    
    struct tms	tmsstart, tmsend;
	clock_t		start, end;

    srand(NULL);
    printf(" Thread %d avviato per client %s:%d \n", 
           thread_id, inet_ntoa(data->client_addr.sin_addr), 
           ntohs(data->client_addr.sin_port));
    
    
    /*Messaggio benvenuto*/
    snprintf(buffer, BUFFER_SIZE, 
            " Gestione del client attraverso thread %d\n", thread_id);
    send(client_fd, buffer, strlen(buffer), 0);
    
    
        /*INSERIRE QUA IL TEMPO DI RISPOSTA*/
        if ((start = times(&tmsstart)) == -1)	/* starting values */
		    err_sys("times error");

        for ( i = 0; i < 50; i++)
        {
        memset(buffer, 0, BUFFER_SIZE); /*Pulisce il buffer*/
        A = random_value();
        
        /*risposta*/
        snprintf(buffer, BUFFER_SIZE, 
                "Numero inviato: %d\n", A);
        send(client_fd, buffer, strlen(buffer), 0);
        }

    
    if ((end = times(&tmsend)) == -1)		/* ending values */
		    err_sys("times error");
            
        pr_times(end-start, &tmsstart, &tmsend);
    
    
    close(client_fd);
    

    free(data);  
    pthread_exit(NULL);
}




/*===================CHILD================*/
/*
void gestisci_client(int client_fd, struct sockaddr_in client_addr) {
    char buffer[BUFFER_SIZE];
    pid_t pid = getpid();
    
    printf("🍴 Processo figlio [PID=%d] gestisce client %s:%d\n", 
           pid, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    
    // Messaggio benvenuto
    snprintf(buffer, BUFFER_SIZE, 
            " Benvenuto! Sei gestito dal processo %d\n", pid);
    send(client_fd, buffer, strlen(buffer), 0);
    
    // Loop gestione messaggi
    while (1) {
        int bytes_read = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytes_read <= 0) {
            if (bytes_read == 0) {
                printf("👋 [PID=%d] Client disconnesso\n", pid);
            } else {
                printf("❌ [PID=%d] Errore recv: %s\n", pid, strerror(errno));
            }
            break;
        }
        
        buffer[bytes_read] = '\0';
        printf("📩 [PID=%d] Ricevuto: '%s'\n", pid, buffer);
        
        // Elaborazione (simula lavoro)
        sleep(1);
        
        // Risposta al client
        snprintf(buffer, BUFFER_SIZE, 
                "✅ [Processo %d] Elaborato: %s", pid, buffer);
        send(client_fd, buffer, strlen(buffer), 0);
    }
    
    close(client_fd);
    printf("🔒 [PID=%d] Processo figlio terminato\n", pid);
    exit(0);  // Termina processo figlio
}
*/


int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    int opt ;
    pthread_t thread;


    opt = 1;
    
    
    /*Creazione socket*/
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    /* 2. BIND E LISTEN */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_fd, 5); /*5 dovrebbe essere il backlog*/
    
    printf("Server Thread avviato su porta %d\n", PORT);
    
    /*Main loop accetta e crea thread*/
    while (1) {
        printf("Aspettando connessioni...\n");
        
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept failed");
            continue;
        }
        
        printf("Nuova connessione da %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
     
        /* 4. PREPARA DATI PER THREAD*/
        ThreadData* data = malloc(sizeof(ThreadData));
        data->client_fd = client_fd;
        data->client_addr = client_addr;
        
        /* 5. CREA THREAD */
        if (pthread_create(&thread, NULL, gestisci_client_thread, data) != 0) {
            perror("pthread_create failed");
            free(data);
            close(client_fd);
        } else {
            /* Thread detached (si pulisce automaticamente) */
            pthread_detach(thread);
        }
    }
    
    close(server_fd);
    return 0;
}