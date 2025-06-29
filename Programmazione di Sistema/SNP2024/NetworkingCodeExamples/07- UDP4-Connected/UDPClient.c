/**
@addtogroup Group7 
@{
*/
/**
@file 	UDPClient.c
@author Catiuscia Melle

@brief 	Presentazione di un UDP Echo Client IPv4, senza hostname resolution ma con 
		l'utilizzo della funzione inet_pton() per la conversione di un IPv4 dotted 
		decimal in intero a 32 bit.

Se il server non è running, il client rimane bloccato nella recvfrom (asynchronous error ICMP not delivered to the process). 
*/

#include "Header.h"

void usage(char *name){
	printf("Usage: %s <serverIP> <msg> <option>\n", name);
	printf("\toption = 1 means that the client will trigger the connect() on the server\n");
}


int main(int argc, char *argv[]){
	
	if (argc != 4)
	{
		usage(argv[0]);
		return 0;
	}
	printf("\tUDP4 Client\n");
	
	
	bool isConnected;
	if (atoi(argv[3]) == 1){
		isConnected = true;
	}
	
	int res = 0; //valore di ritorno delle APIs
	int sockfd = 0; //socket descriptor per la comunicazione col server
	
	// open socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == -1)
	{
		perror("socket error: ");
		return FAILURE;
	}
	
	struct sockaddr_in server; //indirizzo IPv4 del server
	socklen_t len = sizeof(server);
	
	memset(&server, 0, sizeof(server)); //azzero la struttura dati
	server.sin_family = AF_INET; //specifico l'Address FAmily IPv4
	
	//Specifico l'indirizzo del server inet_pton()
	res = inet_pton(AF_INET, argv[1], &(server.sin_addr));
	if (res == -1)
	{
		perror("Errore inet_pton(): ");
		return FAILURE;
	}
	else if (res == 0)
	{
		//se fallisce la conversione
		printf("The input value is not a valid IPv4 dotted-decimal string, using loopback\n");
		server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	}
	//if (res == 1){ printf("OK\n");}
	
	
	server.sin_port = htons(PORT); //porta del server in NETWORK BYTE ORDER
	char serverIP[INET_ADDRSTRLEN] = "";
	printf("Client ready to send messages to server@'%s:%d'\n", inet_ntop(AF_INET, &(server.sin_addr), serverIP, INET_ADDRSTRLEN), ntohs(server.sin_port));
	
	ssize_t n = 0;
	char msg[BUFSIZE] = ""; //messaggio da inviare al server
	memset(msg, 0, BUFSIZE-1);//memset(msg, '@', BUFSIZE-1);
	memcpy(msg, argv[2], strlen(argv[2]));
	
	char buf[BUFSIZE] = ""; //messaggio ricevuto dal server
	
	if (isConnected){
		//questo nodo invia un messaggio per far scattare la connect sul server
		n = sendto(sockfd, CONNECTMSG, strlen(CONNECTMSG), 0, (struct sockaddr *)&server, len);
		if (n == -1)
		{
			perror("sendto() error: ");
			close(sockfd);
			return FAILURE;
		}
		
	}
	
	//a questo punto parte un invio di 3 messaggi
	int i = 0;
	
	for (i=0; i < 5; i++) 
	{
		n = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&server, len);
		if (n == -1)
		{
			perror("sendto() error: ");
			close(sockfd);
			return FAILURE;
		}
		
		//int drop = n/2; 
		
		n = recvfrom(sockfd, buf, BUFSIZE-1, 0, (struct sockaddr *)&server, &len);
		if (n == -1)
		{
			perror("recvfrom() error: ");
			close(sockfd);
			return FAILURE;
		}
		buf[n] = '\0';
		printf("Echoed message = '%s'\n", buf);
		
		//blocco processo ai fini di monitoraggio dello stesso
		sleep(1);
	}//for

	if (isConnected){
		//fa scattare la disconnessione sul server (se aveva fatto scattare la connect)
		n = sendto(sockfd, DISCONNECTMSG, strlen(DISCONNECTMSG), 0, (struct sockaddr *)&server, len);
		if (n == -1)
		{
			perror("sendto() error: ");
			close(sockfd);
			return FAILURE;
		}
		
	}
	//non ricevo l'ultimo messaggio


	close(sockfd);
	
return 0;
}

/** @} */
