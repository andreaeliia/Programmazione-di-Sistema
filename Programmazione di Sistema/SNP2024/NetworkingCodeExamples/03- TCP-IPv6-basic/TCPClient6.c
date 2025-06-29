/**
@addtogroup Group3
@{
*/
/**
@file 	TCPClient6.c
@author Catiuscia Melle

@brief 	Presentazione di un TCP Echo Client IPv6, senza hostname resolution

In questo esempio, si illustra il workflow da attuare per realizzare un TCP Client over IPv6 ed inoltre si mostra come utilizzare la chiamata di sistema <em>getsockname()</em> per recuperare, per un 
socket connesso, le info locali associate al socket.
*/

#include "Header.h"

int main(){

	printf("\tIPv6 TCP Client app\n");
	
	int res = 0; //valore di ritorno delle Sockets API
	int sockfd = 0; //connecting socket
	
	
	//step 1: open IPv6 socket di tipo SOCK_STREAM (TCP)
	sockfd = socket(AF_INET6, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		perror("socket() error: ");
		return FAILURE;
	}
	
	//step 2: indirizzo IPv6 del server TCP da contattare
	struct sockaddr_in6 server;
	socklen_t len = sizeof(server);
	memset(&server, 0, sizeof(server)); //azzero la struttura
	server.sin6_family = AF_INET6; //family IPv6
		
	//utilizzo l'indirizzo di loopback IPv6
	server.sin6_addr = in6addr_loopback;
	
	//host to network short della TCP port
	server.sin6_port = htons(PORT);
	
	
	/*
	utilizzo inet_pton per convertire l'indirizzo IPv6 in notazione esadecimale in un 
	intero a 128 bit in Network Byte Order
	*/
	/*
	res = inet_pton(AF_INET6, "::1", &(server.sin6_addr));
	if (res == 1)
	{
		printf("Memorizzato l'indirizzo IPv6 del server\n");
	} 
	else if (res == -1)
	{
		perror("Errore inet_pton: ");
		close(sockfd);
		return FAILURE;
	} 
	else if (res == 0)
	{
		printf("The IPv6 hexadecimal string in input is not a valid IPv6 address\n");
		close(sockfd);
		return FAILURE;
	}
	*/
	
	
	/*
	step 3: avvio il 3WHS verso il server TCP
	*/
	res = connect(sockfd, (struct sockaddr *)&server, len);
	if (res != 0)
	{
		perror("connect() error: ");
		close(sockfd);
		return FAILURE;
	}
	
	/*
	Otteniamo le informazioni locali del socket connesso
	*/
	struct sockaddr_in6 local;
	len = sizeof(local);
	
	//getsockname estrae IP e porta da cui la connessione client è stata attivata
	res = getsockname(sockfd, (struct sockaddr *)&local, &len);
	if (res == -1)
	{
		perror("getsockname: ");
		close(sockfd);
		return FAILURE;
	} 
	
	char myIP[INET6_ADDRSTRLEN] = "";
	printf("Client connesso dall'indirizzo '%s:%d'\n", inet_ntop(AF_INET6, &(local.sin6_addr), myIP, INET6_ADDRSTRLEN), ntohs(local.sin6_port) );
	
	
	//definizione del messaggio da inviare al server
	char msg[BUFSIZE] = "questo messaggio contiene hello world";
	
	ssize_t n = 0;
	n = send(sockfd, msg, strlen(msg), 0);
	if (n == -1)
	{
		perror("send() error: ");
	}
	else
	{
		printf("sent %d bytes\n", (int) n);
	}
	
	close(sockfd);	
return 0;
}

/** @} */
