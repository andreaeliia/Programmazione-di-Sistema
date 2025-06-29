/**
@addtogroup Group4 
@{
*/
/**
@file 	UDPServer-connected.c
@author Catiuscia Melle

@brief 	Presentazione di un UDP Echo Server IPv4 che utilizza connect() sul 
		socket utilizzato per ricevere i messaggi dei client.

Il server connette il socket all'indirizzo del primo client che lo contatta.

Se altri client UDP cercano di contattarlo (usando la versione UDPClient.c) 
rimarranno appesi nella recvfrom...
*/

#include "Header.h"


/**
@brief gestione del socket UDP connesso
@param sockfd - socket UDP connesso
@return nulla
*/
void handle_peer(int sockfd, char *buffer){

	int i = 0;
	ssize_t n = 0;
	for (i = 0; i < COUNT-1; i++)
	{
		printf("Sending reply %d\n", i);
		n = send(sockfd, buffer, strlen(buffer), 0);
		if (n == -1)
		{
			perror("send() error: ");
		}
		
		n = recv(sockfd, buffer, BUFSIZE-1, 0);
		if (n == -1)
		{
			perror("recv() error: ");
		}
	}//for
	
	printf("Finish with this client\n");
	
return;
}


/**
@brief Disconnessione di un socket UDP connesso
@param sockfd - socket UDP connesso
@return nulla
*/
void disconnect(int sockfd){

	struct sockaddr_in none;
	memset(&none, 0, sizeof(none));
	none.sin_family = AF_UNSPEC;
	
	int res = connect(sockfd, (struct sockaddr *)&none, sizeof(none));
	
	if (res == -1)
	//if (res != 0 && errno == EAFNOSUPPORT)
	{
		perror("connect() error: ");
	}
	else
	{
		printf("Connect to null address successful\n");
	}
	
return;
}


int main(int argc, char *argv[]){
	
	printf("\tIPv4 UDP Server (connect to first peer) app\n");
		
	int res = 0; //valore di ritorno delle APIs

	/*
	socket: servirà per la comunicazione col server
	*/
	int sockfd = 0;
	
	/*
	open socket
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
	
	
	//bind to well-known server address
	res = bind(sockfd, (struct sockaddr *)&server, len);
	if (res == -1)
	{
		perror("Bind error: ");
		close(sockfd);
		exit(1);
	}//fi
	
		
	ssize_t n = 0;
	char buffer[BUFSIZE] = "";
	struct sockaddr_in client;
	char address[INET_ADDRSTRLEN] = "";
	
	bool isFirst = true;
	
	while (1)
	{
		printf("Waiting messages...\n");
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
		
		if (isFirst)
		{	
			printf("Connect the socket\n");
			res = connect(sockfd, (struct sockaddr *)&client, len);
			if (res == -1)
			{
				perror("Error on UDP connect(): ");
				close(sockfd);
				exit(1);
			}//fi
			
			handle_peer(sockfd, buffer);
			isFirst = false;
			disconnect(sockfd);
			continue;
			
		}//first client
			
		printf("Sending reply...\n");
		n = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client, len);
		if (n == -1)
		{
			perror("sendto() error: ");
			close(sockfd);
			return FAILURE;
		}

	}//wend
		
	close(sockfd);
	
return 0;
}

/** @} */

