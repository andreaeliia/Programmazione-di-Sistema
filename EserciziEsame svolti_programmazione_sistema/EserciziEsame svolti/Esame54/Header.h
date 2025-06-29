/**
@addtogroup Group4 
@{
*/
/**
@file 	Header.h
@author Catiuscia Melle

@brief 	Definizione di un header comune al client e server dell'esempio.
*/

#ifndef __HEADER_H__
#define __HEADER_H__

#include <stdio.h>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>


#define FAILURE 3 		/**< definizione del valore di errore di ritorno del processo 
						 in caso di errori delle Socket APIs */
#define PORT 49152 		/**< UDP server port */

#define COUNT 10 		/**< Dimensione della sequenza di messaggi inviati dal client */

#define BUFSIZE 256 	/**< Dimensione del buffer applicativo */


#endif /* __HEADER_H__ */

int UDPInit(){
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
	
	
	server.sin_addr.s_addr = htonl(INADDR_ANY);
		
	
	server.sin_port = htons(PORT);
	
	//setto l'indirizzo well-known del socket
	res = bind(sockfd, (struct sockaddr *)&server, len);
	if (res == -1)
	{
		perror("Bind error: ");
		close(sockfd);
		exit(1);
	}//fi
	
	return sockfd;
}

/** @} */
