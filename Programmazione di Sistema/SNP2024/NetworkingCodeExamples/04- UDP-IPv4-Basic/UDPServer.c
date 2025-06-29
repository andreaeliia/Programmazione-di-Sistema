/**
@defgroup Group4 UDP Client and Server

@brief UDP/IPv4 Client and Server
@{
*/
/**
@file 	UDPServer.c
@author Catiuscia Melle

@brief 	Presentazione di un UDP Echo Server IPv4.

Il server, in un ciclo infinito:
- riceve un messaggio 
- lo re-invia in echo.
*/


#include "Header.h"



int main(){
	
	printf("\tIPv4 UDP Server app\n");
		
	int res = 0; //valore di ritorno delle APIs

	/*
	socket: servirà per la comunicazione col server
	*/
	int sockfd = 0;
	
	/*
	open socket di tipo datagram
	*/
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == -1)
	{
		perror("socket error: ");
		return FAILURE;
	}
	
	/*
	indirizzo IPv4 del server, senza hostname resolution
	*/
	struct sockaddr_in server;
	socklen_t len = sizeof(server);
	
	memset(&server, 0, sizeof(server)); //azzero la struttura dati
	server.sin_family = AF_INET; //specifico l'Address Family IPv4
	
	/*
	Specifico l'indirizzo del server: qualsiasi interfaccia
	*/
	server.sin_addr.s_addr = htonl(INADDR_ANY);
		
	/*
	Specifico la well-known port
	*/
	server.sin_port = htons(PORT);
	
	//setto l'indirizzo well-known del socket
	res = bind(sockfd, (struct sockaddr *)&server, len);
	if (res == -1)
	{
		perror("Bind error: ");
		close(sockfd);
		exit(1);
	}//fi
	
		
	ssize_t n = 0;
	char buffer[BUFSIZE];
	
	struct sockaddr_in client;
	char address[INET_ADDRSTRLEN] = "";
	
	int quit = 0;
	
	while (!quit)
	{
		n = recvfrom(sockfd, buffer, BUFSIZE-1, 0, (struct sockaddr *)&client, &len);
		if (n == -1)
		{
			perror("recvfrom() error: ");
			continue;
// 			close(sockfd);
// 			return FAILURE;
		}
	
		buffer[n] = '\0';
		printf("\tRicevuto messaggio:\n\t'%s'\n\tda: %s:%d\n", buffer, \
		inet_ntop(AF_INET, &(client.sin_addr), address, INET_ADDRSTRLEN), \
		ntohs(client.sin_port) );

		printf("Sending reply...\n");
		
		n = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client, len);
		if (n == -1)
		{
			perror("sendto() error: ");
			continue;
// 			close(sockfd);
// 			return FAILURE;
		}
	
	}//wend
		
	//qui non ci arrivo mai...	
	close(sockfd);
	
return 0;
}

/** @} */
