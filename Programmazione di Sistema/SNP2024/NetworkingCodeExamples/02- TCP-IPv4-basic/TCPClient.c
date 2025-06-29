/**
@addtogroup Group2
@{
*/
/**
@file 	TCPClient.c
@author Catiuscia Melle

@brief 	Presentazione di un TCP Echo Client IPv4, senza hostname resolution

Questo esempio illustra le chiamate base che è necessario implementare per poter avviare un TCP client. 

Il client deve:
- definire l'indirizzo del server da contattare: 
	- o usiamo l'interfaccia di loopback per dialogare col server;
	- o usiamo la funzione <em>inet_pton()</em> per la conversione di un IPv4 dotted decimal in intero a 32 bit in Network Byte Order, da potersi utilizzare in una <em>connect()</em> verso il server TCP.
- stabilire la connessione verso il server TCP;
- inviare una serie di messaggi;
- chiudere la connessione;
- terminare l'esecuzione.
*/

#include "Header.h"


int main(int argc, char *argv[]){
	
	int res = 0; //valore di ritorno delle Sockets API
	
	int sockfd = 0; //connection socket: servirà per la comunicazione col server
	
	struct sockaddr_in server; //IPv4 server address, senza hostname resolution 
	socklen_t len = sizeof(server); //dimensione della struttura di indirizzo
	
	memset(&server, 0, sizeof(server)); //azzero la struttura dati
	server.sin_family = AF_INET; //specifico l'Address FAmily IPv4
	
	/*
	Specifico la porta del server da contattare, verso cui avviare il 3WHS
	*/
	server.sin_port = htons(PORT);
	
	if (argc == 1)
	{
		//non è stato indicato un indirizzo IPv4 per il server, usiamo localhost
		printf("\tTCP Client app connecting to localhost TCP server...\n");
		
		/* Specifico l'indirizzo del server usando 
		il wildcard address IPv4 INADDR_LOOPBACK per specificare 
		l'indirizzo di loopback 127.0.0.1 in Network Byte Order
		*/
		server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	} 
	else 
	{
		printf("\tTCP Client app connecting to TCP server at '%s'\n", argv[1]);
		
		/* Utilizzo inet_pton() per convertire l'indirizzo dotted decimal */
		res = inet_pton(AF_INET, argv[1], &(server.sin_addr));
		if (res == 1){
			printf("Memorizzato l'indirizzo IPv4 del server\n");
		}
		else if (res == -1)
		{
			perror("Errore inet_pton: ");
			return FAILURE;
		}
		else if (res == 0)
		{
			printf("The input value is not a valid IPv4 dotted-decimal string\n");
			return FAILURE;
		}
	}
	
	//connessione al server
	/* open socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		perror("socket error: ");
		return FAILURE;
	}
	
	//avvio il 3WHS
	res = connect(sockfd, (struct sockaddr *)&server, len);
	if (res != 0)
	{
		perror("connect() error: ");
		close(sockfd);
		return FAILURE;
	}
	
	//messaggio predefinito da inviare al server
	char msg[BUFSIZE] = "";
	int counter = 0;
	ssize_t n = 0;
	
	int i = 0;
	for (i = 0; i < 20; i++){
		snprintf(msg, BUFSIZE-1, "msg number %d", ++counter);
		
		sleep(1);
		
		n = send(sockfd, msg, strlen(msg), 0);
		if (n == -1)
		{
			perror("send() error: ");
			close(sockfd);
			return FAILURE;
		}
		printf("Client queued %d bytes to server\n", (int) n);
		
		n = recv(sockfd, msg, BUFSIZE -1, 0);
		if (n == -1) {
			perror("recv() error: ");
			close(sockfd);
			return FAILURE;
		} else if (n == 0) {
			printf("server closed connetion\n");
			break;
		}
		if (n > 0) {
			msg[n] = 0;
			printf("server reply: '%s'\n", msg);
		}
		
	}//for
	
	//4-way teardown
	close(sockfd);

return 0;
}

/** @} */
