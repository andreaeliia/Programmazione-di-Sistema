/**
@defgroup Group3 TCP/IPv6 Client-Server skeletons

@brief TCP/IPv6 Client-Server skeletons 

Si presenta la variante per IPv6 degli esempi client-server TCP.
@{
*/
/**
@file 	TCPServer6.c
@author Catiuscia Melle

@brief 	Presentazione di un TCP Echo Server IPv6, senza hostname resolution

Questo esempio illustra le chiamate base che è necessario implementare per poter avviare un TCP server e come gestire gli indirizzi IPv6 nelle <em>struct sockaddr_in6</em>. 

Il server, dopo aver accettato la connessione del client, attende la ricezione di un 
messaggio che visualizza assieme ai dati del client connesso, utilizzando <em>inet_ntop()</em>. 
Quindi chiude la connessione e termina l'esecuzione.

In questo esempio, il server esegue una <em>accept()</em> senza farsi restituire dal kernel i dati del client ed utilizza in seguito <em>getpeername()</em> per recuperare le info del peer.
*/

#include "Header.h"

int main(){
	int res = 0; //valore di ritorno delle Sockets API
	printf("\tIPv6 TCP Server app\n");
	
	int sockfd = 0; /* listening socket del server TCP */
	
	//step 1: open listening socket per IPv6
	sockfd = socket(AF_INET6, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		perror("socket() error: ");
		return FAILURE;
	}
	
	//step 2: definizione dell'indirizzo IPv6 su cui il server deve mettersi in ascolto
	struct sockaddr_in6 server;
	socklen_t len = sizeof(server);
	
	memset(&server, 0, sizeof(server)); //azzero la struttura dati
	server.sin6_family = AF_INET6; //specifio il dominio IPv6
	
	/*
	variante rispetto ad IPv4
	l'inizializzazione al wildcard address IPv6 viene realizzata con questo assegnamento, in cui in6addr_any è il wildcard nullo IPv6
	*/
	server.sin6_addr = in6addr_any;
	
	/*
	Host to Network Short della porta TCP di ascolto del server
	*/
	server.sin6_port = htons(PORT);
	
	
	//step 3: assegno l'indirizzo al socket, mediante opportuno cast
	res = bind(sockfd, (struct sockaddr *)&server, len);
	if (res != 0)
	{
		perror("bind() error: ");
		close(sockfd);
		return FAILURE;
	}
	
	res = listen(sockfd, BACKLOG);
	if (res != 0)
	{
		perror("listen() error: ");
		close(sockfd);
		return FAILURE;
	}
	
	
	int clientfd = 0; /* connected socket del client TCP */
	
	/*
	aspetto una connessione TCP, ma non prelevo le info sul client
	clientfd = accept(sockfd, (struct sockaddr *)&client, &len);
	*/
	clientfd = accept(sockfd, NULL, NULL);
	if (clientfd == -1)
	{
		perror("accept() error: ");
		close(sockfd);
		return FAILURE;
	}
	
	/*
	struttura di indirizzo IPv6 per memorizzare i dati del client
	*/
	struct sockaddr_in6 client;
	memset(&client, 0, sizeof(client));//azzero la struttura
	len = sizeof(client);

	//buffer di ricezione del messaggio
	char msg[BUFSIZE] = "";
	ssize_t n = 0;
	
	//aspetto il messaggio
	n = recv(clientfd, msg, BUFSIZE-1, 0);
	if (n == -1)
	{
		perror("recv() error: ");	
	}
	else if (n==0)
	{
		printf("Client closed connection\n");
	}
	else 
	{
		printf("Received %d bytes\n", (int) n);
		msg[n] ='\0';
		printf("Message '%s'\n", msg);
		
		char ipv6[INET6_ADDRSTRLEN];
		
		//utilizzo getpeername per leggere i dati remoti del socket
		int res = getpeername(clientfd, (struct sockaddr *)&client, &len);
		if (res == -1)
		{
			perror("getpeername():");
		}
		else
		{
			printf("from %s:%d\n", inet_ntop(AF_INET6, &(client.sin6_addr), ipv6, INET6_ADDRSTRLEN), ntohs(client.sin6_port));	
		}	
		
	}
	
	//dealloco le risorse	
	close(clientfd);
	close(sockfd);
	
return 0;
}

/** @} */