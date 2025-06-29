/**
@addtogroup Group4 
@{
*/
/**
@file 	UDPClient.c
@author Catiuscia Melle

@brief 	Presentazione di un UDP Echo Client IPv4, senza hostname resolution ma con 
		l'utilizzo della funzione inet_pton() per la conversione di un IPv4 dotted 
		decimal in intero a 32 bit.
*/

#include "Header.h"

int main(int argc, char *argv[]){
	
	
	
	//messaggio da inviare al server
	char msg[BUFSIZE] = "CIAO";
	
	msg[BUFSIZE-1] = '\0'; //tronco il messaggio a BUFSIZE caratteri

	int res = 0; 	//valore di ritorno delle APIs
	int sockfd = 0; //socket descriptor per la comunicazione col server
	
	// open socket
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
	
	server.sin_family = AF_INET; //specifico l'Address FAmily IPv4	
	server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	server.sin_port = htons(PORT);


	ssize_t n = 0;
	n = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&server, len);
	if (n == -1)
	{
		perror("sendto() error: ");
		close(sockfd);
		return FAILURE;
	}
	char serverIP[INET_ADDRSTRLEN] = "";
	
	printf("Client sent %d bytes to server at '%s:%d'\n", (int) n, inet_ntop(AF_INET, &(server.sin_addr), serverIP, INET_ADDRSTRLEN), ntohs(server.sin_port));
	
	//blocco processo ai fini di monitoraggio dello stesso
	sleep(10);
	
	int drop = n/2; 
	n = recvfrom(sockfd, msg, drop, 0, (struct sockaddr *)&server, &len);
	//n = recvfrom(sockfd, msg, BUFSIZE-1, 0, (struct sockaddr *)&server, &len);
	if (n == -1)
	{
		perror("recvfrom() error: ");
		close(sockfd);
		return FAILURE;
	}
	
	msg[n] = '\0';
	printf("Echoed message = '%s'\n", msg);
	
	close(sockfd);
	
return 0;
}

/** @} */
