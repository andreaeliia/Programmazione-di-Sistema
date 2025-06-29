/**
@addtogroup Group7 
@{
*/
/**
@file 	ClientConnected.c
@author Catiuscia Melle

@brief 	Client UDP che esegue la connect prima di inviare al server
*/

#include "Header.h"


int main(int argc, char *argv[]){
	
	if (argc != 3)
	{
		printf("Usage: %s <serverIP> <msg>\n", argv[0]);
		return 0;
	}
	printf("\tUDP4 Client\n");
	
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
	printf("Client connecting the UDP socket to server@'%s:%d'\n", inet_ntop(AF_INET, &(server.sin_addr), serverIP, INET_ADDRSTRLEN), ntohs(server.sin_port));
	
	res = connect(sockfd, (struct sockaddr *)&server, len);
	if (res == -1){
		perror("UDP Connect error:");
		exit(1);
	}
	
	ssize_t n = 0;
	char msg[BUFSIZE] = ""; //messaggio da inviare al server
	memcpy(msg, argv[2], strlen(argv[2]));
	
	char buf[BUFSIZE] = ""; //messaggio ricevuto dal server
	int i = 0;
	
	for (i = 0; i < 5; i++) {
		n = send(sockfd, msg, strlen(msg), 0);
		if (n == -1)
		{
			perror("send error: ");
			close(sockfd);
			return FAILURE;
		}
		//perror("rilevazione di errore intermedia:");
		sleep(1);
		n = recv(sockfd, buf, BUFSIZE -1 , 0);
		if (n == -1)
		{
			perror("recv error on connected UDP socket:");
			close(sockfd);
			exit(1);
		} else {
			buf[n] ='\0';
			printf("echoed %s\n", buf);
		}
		
	}

	close(sockfd);
	
return 0;
}

/** @} */
