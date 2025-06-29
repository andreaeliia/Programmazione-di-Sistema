/**
@defgroup Group6 TCP 4-way teardown 

@brief TCP 4-way teardown with TCP/IPv4 Client and Server to analyze TCP implementation

L'esempio evidenzia la relazione tra applicazioni e servizio di trasporto affidabile del TCP
@{
*/
/**
@file 	Server.c
@author Catiuscia Melle

@brief TCP Echo Server IPv4 iterativo

E' un server iterativo che mette in evidenza:
-# il confronto tra close() e shutdown();
Il server riceverà sempre dei RST per le send che effettua quando il client ha già chiuso la connessione.
Se invece il client esegue una half-close, lo scambio dei dati è effettuato fino alla fine.
-# l'inefficienza del server iterativo per la gestione di connessioni TCP
*/
#include "Header.h"


/*
void usage(char *name){
	printf("Usage: %s <numero>\n", name);
	printf("\t- 1 to stop after EOF\n");
	printf("\t- 2 to send a Bye message after EOF\n");
}

int stopFlag(char *param){
	if (strncmp(param, "1", 1) == 0) {
		return NOBYEMSG;
	} else {
		return SENDBYEMSG;
	}
}
*/


void initLocalAddress(struct sockaddr_in *addr){
	memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family= AF_INET;	//IPv4 family
	addr->sin_addr.s_addr = htonl(INADDR_ANY);
	addr->sin_port = htons(PORT);
}


/**
@brief Realizza la passive open per il server
@return intero che vale:
- (-1) in caso di errore
- (>0) pari al passive socket in caso di successo
*/
int passiveOpen(){
	
	int sockfd = 0; /* listening socket del server TCP */
	struct sockaddr_in addr; /* server local address, for bind() */
	int res = 0;
	
	//step 1: open listening socket per IPv4
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		perror("socket() error: ");
		return sockfd;
	}
	
	initLocalAddress(&addr);
	
	res = bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	if (res == -1)
	{
		perror("bind() error: ");
		close(sockfd);
		return res;
	}
	
	res = listen(sockfd, BACKLOG);
	if (res == -1)
	{
		perror("listen() error: ");
		close(sockfd);
		return res;
	}
	
	return sockfd;
}


/**
@brief Gestione di una connessione TCP con il client
Finché il client non chiude la connessione, il server riceve ed invia in echo il messaggio ricevuto
@param peerfd, socket handle della nuova connessione
@param finalMsg, determina se inviare un messaggio in echo all'EoF o meno
@return nulla
*/
void handleConnection(int peerfd) {
	
	char clientaddr[INET_ADDRSTRLEN] = "";
	struct sockaddr_in peer_addr;
	socklen_t len = sizeof(peer_addr);
	
	getpeername(peerfd, (struct sockaddr *)&peer_addr, &len);
	
	inet_ntop(AF_INET, &(peer_addr.sin_addr), clientaddr, INET_ADDRSTRLEN);
	printf("\tAccepted a new TCP connection from %s:%d\n", clientaddr, ntohs(peer_addr.sin_port));

	char buf[BUFSIZE];
	ssize_t n = 0;
	int connected = 1;
	
	while (connected) 
	{
		n = recv(peerfd, buf, BUFSIZE-1, 0);
		if (n == -1)
		{
			perror("recv() error: ");
			connected = 0;
		}
		else if (n==0)
		{
			printf("Peer closed connection\n");
			connected = 0;
			
			//send bye message
			n = send(peerfd, "BYE", 3, 0);
			if (n == -1) {
				perror("\tSend error:");
			}
			printf("Sent Bye message to client\n\n");
		}
		else 
		{
			buf[n] ='\0';
			printf("Received message '%s'\n", buf);
			//reply to client
			n = send(peerfd, buf, strlen(buf), 0);
			if (n == -1) {
				perror("\tSend error:");
			}
		}
	}//wend connected

	close(peerfd);
}



int main(int argc, char *argv[]){
	
	printf("\n\tTCP4 Echo server with BYE message\n");
	
	int sockfd = passiveOpen();
	if (sockfd == -1){
		printf("Failed during passive open\n");
		exit(1);
	}

	printf("\n\tServer listening on port %d\n", (int)PORT);
	
	int peerfd = 0; /* connected socket del client TCP */	
	int quit = 0; //regola il loop infinito nel server
	
	while (!quit) 
	{
		printf("\n\tServer waiting for connections...\n");
		
		peerfd = accept(sockfd, NULL, NULL);
		if (peerfd == -1)
		{
			perror("accept() error: ");
			close(sockfd);
			return FAILURE;
		}
		handleConnection(peerfd);
		
	}//wend quit
	close(sockfd);
	
return 0;
}

/** @} */

