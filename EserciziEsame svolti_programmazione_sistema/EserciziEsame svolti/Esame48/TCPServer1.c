#include "Header.h"
#include <string.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <stdbool.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>


int inizializzaSHM();
void freeSHM();
char* str;

int main(int argc, char *argv[]){
	
	int sockfd = 0; /* listening socket del server TCP */
	int peerfd = 0; /* connected socket del client TCP */
	int ret = 0; 	/* valore di ritorno delle Sockets API */
	
	/*step 1: open listening socket per IPv4*/
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		perror("socket() error: ");
		return FAILURE;
	}
	
	inizializzaSHM();
	
	
	/*definizione di un indirizzo IPv4 su cui il server deve mettersi in ascolto*/
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family= AF_INET;/*IPv4 family*/
	
	/*
	IPv4 wildcard address: qualsiasi IP
	Notare che l'indirizzo deve essere assegnato al campo s_addr in Network Byte Order
	*/
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	/*
	TCP port su cui il server si metterà in ascolto, in Network Byte Order
	*/
	addr.sin_port = htons(PORTA);

	/*
	Assegno al socket l'indirizzo IPv4 memorizzato in addr
	*/
	ret = bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	if (ret == -1)
	{
		perror("bind() error: ");
		close(sockfd);
		return FAILURE;
	}
	
	/*
	Passive Open obbligatoria per un server TCP: abilito il TCP layer ad
	accettare connessioni per questo socket
	*/
	ret = listen(sockfd, BACKLOG);
	if (ret == -1)
	{
		perror("listen() error: ");
		close(sockfd);
		return FAILURE;
	}
	
	printf("\n\tServer listening on port %d\n", (int)PORTA);
	
	/*
	Il server TCP memorizzerà in peer_addr l'indirizzo del client connesso
	*/
	struct sockaddr_in peer_addr;
	socklen_t len = sizeof(peer_addr);
	
	int quit = 0; //regola il loop infinito nel server
	int connected = 0; //regola la gestione della connessione col client
	while (!quit) 
	{
		/*
		chiamata bloccante:
		fino a quando non c'è una connessione completa (3WHS terminato) il server rimane bloccato in accept. 
		Al ritorno, peerfd è il socket connesso della connessione col client 
		- salvo errori.
		*/
		peerfd = accept(sockfd, (struct sockaddr *)&peer_addr, &len);
		if (peerfd == -1)
		{
			perror("accept() error: ");
			close(sockfd);
			return FAILURE;
		}
	
		char clientaddr[INET_ADDRSTRLEN] = "";
		inet_ntop(AF_INET, &(peer_addr.sin_addr), clientaddr, INET_ADDRSTRLEN);
		printf("\tAccepted a new TCP connection from %s:%d\n", clientaddr, ntohs(peer_addr.sin_port));
	
		char buf[BUFSIZE];
		ssize_t n = 0;
		connected = 1;
		
		while (connected) 
		{
			n = recv(peerfd, buf, BUFSIZE, 0);
			
			if (n == -1)
			{
				perror("recv() error: ");
			}
			else if (n==0)
			{
				printf("Peer closed connection\n");
				connected = 0;
			}
			else 
			{	
			
				char * shared = str;
				strncpy(shared,buf,511);
				printf("%s",str);
				
			}
			
		}//wend connected
		close(peerfd);
		freeSHM();
	}//wend quit
	close(sockfd);
	
return 0;
}

/** @} */

int inizializzaSHM(){
	key_t key = ftok("./shmfile",65);
	int shmid = shmget(key,1024,0666|IPC_CREAT);
	str = (char*) shmat(shmid,(void*)0,0);
	printf("MEMORIA CONDIVISA INIZIALIZZATA\n");
}

void freeSHM(){
shmdt(str);
}


