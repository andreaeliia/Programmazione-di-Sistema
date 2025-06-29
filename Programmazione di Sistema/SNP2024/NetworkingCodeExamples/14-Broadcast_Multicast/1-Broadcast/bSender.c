/**
@defgroup Group14 Broadcast applications
@brief Broadcast applications
@{
*/
/**
@file 	bSender.c
@author Catiuscia Melle

@brief 	Implementazione di un broadcast sender.

Un processo che voglia inviare traffico broadcast (dominio IPv4), deve specificare 
l'indirizzo broadcast, cioè coppia IP:porta, cui inviare il traffico. 
L'indirizzo IPv4 è il <em>limited broadcast 255.255.255.255</em>, mappato nella costante
<em>INADDR_BROADCAST</em>. 
Il socket <em>UDP</em>, dal quale effettuare l'invio dei messaggi diretti a tutti i nodi 
della propria rete, deve abilitare l'opzione SO_BROADCAST per consentire al processo di 
generare questo tipo di traffico. 

*/

#include "Header.h"


int main(int argc, char *argv[]){

	/*
	if (argc != 3)
  	{ 	
		printf("Usage: %s <IPv4> <Message>\n", argv[0]);
    	return FAILURE;
	}
	*/
	
	int res = 0;
	
	//setting broadcast address
	struct sockaddr_in broadcastAddr;
	memset(&broadcastAddr, 0, sizeof(broadcastAddr));
	
	broadcastAddr.sin_family = AF_INET;
	broadcastAddr.sin_port = htons(PORTNUMBER);
	
	if (argc==1) {
		//LIMITED BROADCAST: 
		broadcastAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST); 
	}
	else
	{
		//SUBNET-DIRECTED BROADCAST address
		res = inet_pton(AF_INET, argv[1], &broadcastAddr.sin_addr);
		if (res != 1) {
			perror("Error on inet_pton(): ");
			return FAILURE;
		}
  	}
  	
  	// Create socket for sending/receiving datagrams
  	int bsock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  	if (bsock < 0)
  	{
    	perror("socket() failed: ");
		return FAILURE;
	}
	
  	// Set socket to allow broadcast
  	int broadcastPermission = 1;
  	if (setsockopt(bsock, SOL_SOCKET, SO_BROADCAST, &broadcastPermission, sizeof(broadcastPermission)) < 0)
    {
    	perror("setsockopt() failed: ");
		return FAILURE;
	}

   	printf("Starting to broadcast ...\n");
	
	int count = 0;
    char message[BUFSIZE] = "";

	int quit = 0;
	while( !quit) 
  	{
  		// Run forever or while not quitting
    
  		if (count == THRESHOLD)
		{
			memset(message, 0, BUFSIZE);
		 	memcpy(message, QUIT, strlen(QUIT));
		 	quit = 1;
		} 
		else 
		{	
			snprintf(message, BUFSIZE-1, "Broadcast message number %d", count);
		}//fi
		
		// Broadcast msgString in datagram to clients every FREQ seconds
    	ssize_t numBytes = sendto(bsock, message, strlen(message), 0, (struct sockaddr *)&broadcastAddr, sizeof(broadcastAddr));
    
    	if (numBytes < 0)
    	{
    		perror("sendto() failed: ");
			return FAILURE;
		} 
		else if (numBytes != strlen(message))
      	{
    		perror("sendto(), sent unexpected number of bytes: ");
			return FAILURE;
		}
		else
		{
			printf("Sent %s\n", message);
		}
	
    	count++;
		
    	sleep(FREQ); //Avoid flooding the network
  	
  	}//wend
    
    printf("Broadcasting ended!\n");
	
return 0; 
}

/** @} */
