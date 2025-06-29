/**
@addtogroup Group4 
@{
*/
/**
@file 	UDPClient-connected.c
@author Catiuscia Melle

@brief 	Presentazione di un UDP Echo Client IPv4 che invia al server una sequenza 
		di messaggi.
		
Lo scopo di questo esempio è illustrare il funzionamento della connect() su un socket UDP.
Mentre questo processo invia i suoi messaggi, il server - che avrà connesso il socket a questo client - non potrà ricevere traffico da altri peer. 
Solo dopo la disconnessione riceverà nuovamente altro traffico.
*/

#include "Header.h"


int main(int argc, char *argv[]){
	
	if (argc != 2)
	{
		printf("Usage: %s <IPv4 (dotted string) server address>\n", argv[0]);
		return FAILURE;
	}
	
	printf("\tIPv4 UDP looping client app\n");
		
	char msg[BUFSIZE] = ""; //messaggio da inviare al server
	int res = 0; 		//valore di ritorno delle APIs
	int sockfd = 0; 	//socket: servirà per la comunicazione col server
	
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
	server.sin_family = AF_INET; //specifico l'Address FAmily IPv4
	
	/*
	Specifico l'indirizzo del server inet_pton()
	*/	
	res = inet_pton(AF_INET, argv[1], &(server.sin_addr));
	
	if (res == 1)
	{
		printf("Memorizzato l'indirizzo IPv4 del server\n");
	}
	else if (res == -1)
	{
		perror("Errore inet_pton: ");
	}
	else if (res == 0)
	{
		printf("The input value is not a valid IPv4 dotted-decimal string\n");
	}
	
	/*
	Specifico la porta del server da contattare
	*/
	server.sin_port = htons(PORT);
		
	ssize_t n = 0;
	bool condition = true;
	int counter = 0;
	
	for (counter = 0; counter < COUNT; counter++)
	{
		//compongo il messaggio
		snprintf(msg, BUFSIZE-1, "Message number %d", counter);
		
		
		//invio il messaggio
		n = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&server, len);
		if (n == -1)
		{
			perror("sendto() error: ");
			break;
		}
		printf("Client sent %d bytes to server\n", (int) n);
	
		if (counter < COUNT-1)
		{
			//aspetto risposta	
			n = recvfrom(sockfd, msg, BUFSIZE-1, 0, (struct sockaddr *)&server, &len);
			if (n == -1)
			{
				perror("recvfrom() error: ");
				break;
			}
			msg[n] = '\0';
			printf("\tEchoed message = '%s'\n", msg);
			
			sleep(3);
		}
		else
		{
			printf("Sequenza di invio terminata\n\n");
			break; //l'ultima recvfrom non occorre
		}//fi
		
	}//for
	
	close(sockfd);
	
return 0;
}

/** @} */


