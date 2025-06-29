/** 
@addtogroup Group12
@{
*/
/**
@file 	UDPClient.c
@author Catiuscia Melle
@brief 	Implementazione di un UDP client con setting di un timeout sull'I/O.

Gli esempi mostrano l'utilizzo delle opzioni SO_RCVTIMEO e SO_SNDTIMEO per impostare un 
tempo massimo di blocco delle operazioni di read e write su di un socket.
Queste opzioni consentono di evitare che un processo si blocchi indefinitamente in una 
operazione di I/O su di un socket. 
*/

#include "Shares.h"


/**
@brief Legge i valori di timeout associati alle operazioni I/O sul socket
@param sockfd, il socket in esame
@return nulla
*/
void printTimeouts(int sockfd){

	struct timeval tv;
	tv.tv_sec = 0;
 	tv.tv_usec = 0;
 	
 	socklen_t optlen = sizeof(tv);
 	
 	int rv = 0;
 	
 	rv = getsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, &optlen);
 	if (rv < 0)
 	{
 		perror("getsockopt error on SO_RCVTIMEO: ");
 		close(sockfd);
 		exit(1);
 	}
 	else
 	{
 		printf("Valore di default del RCVTIMEO = %.3f [sec] %.3f[microseconds]\n", (float)tv.tv_sec, (float)tv.tv_usec);
 	}
 	
 	tv.tv_sec = 0;
 	tv.tv_usec = 0;
 	optlen = sizeof(tv);
 	
 	rv = getsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, &optlen);
 	if (rv < 0)
 	{
 		perror("getsockopt error on SO_SNDTIMEO: ");
 		close(sockfd);
 		exit(1);
 	}
 	else
 	{
 		printf("Valore di default del SNDTIMEO = %.3f [sec] %.3f[microseconds]\n", (float)tv.tv_sec, (float)tv.tv_usec);
 	}
}



int main(int argc, char *argv[]){
	
	if (argc != 2) 
	{
		printf("Usage: %s <servername>\n", argv[0]);
		return FAILURE;
	}
	
	int sockfd;
	int rv;
	
	struct addrinfo hints, *servinfo, *p;
	memset(&hints, 0, sizeof hints); 
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_DGRAM;
 	
	hints.ai_flags |= AI_V4MAPPED; 
	hints.ai_flags |= AI_NUMERICSERV;
 	
	if ((rv = getaddrinfo(argv[1], SERVICEPORT, &hints, &servinfo)) != 0) 
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return FAILURE;
	}

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) 
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 
		{
			perror("UDP client socket() error: ");
			continue;
		}

		break;
	}//for

	if (p == NULL) 
	{
		fprintf(stderr, "%s: failed to open UDP socket\n", argv[0]);
		return FAILURE;
	}
		
	//copiamo l'indirizzo del server 
	struct sockaddr_storage serv;
	socklen_t len = sizeof(struct sockaddr_storage);
	memset(&serv, 0, len);
	
	memcpy(&serv, p->ai_addr, p->ai_addrlen);
	len = p->ai_addrlen;
	
	//quindi posso liberare la linked-list...
	freeaddrinfo(servinfo);
	
	
	//valori di default dei timeout del socket
	printTimeouts(sockfd);
		
 	//impostiamo ora un valore di timeout per la lettura
 	struct timeval tv;
 	tv.tv_sec = 2;
 	tv.tv_usec = 0;
 	
	rv = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	if (rv < 0)
 	{
 		perror("setsockopt error on SO_RCVTIMEO: ");
 		close(sockfd);
 		return FAILURE;
 	}
	
	//impostiamo ora un valore di timeout per la scrittura
	/*
	rv = setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
	if (rv < 0)
 	{
 		perror("setsockopt error on SO_SNDTIMEO: ");
 		close(sockfd);
 		return FAILURE;
 	}
	*/
	
	//i valori impostati dal processo
	printTimeouts(sockfd);
	
	/*
	prepariamo il messaggio da inviare
	*/
	char buf[BUFSIZE]; 
	memset(buf, 0, BUFSIZE);
	memset(buf, 'b', BUFSIZE/2);
	memset(buf+BUFSIZE/2, 'a', (BUFSIZE/2-1));
	
	ssize_t numbytes;
	
	//invio il messaggio
	if ((numbytes = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&serv, len)) == -1) 
	{
		if (errno == EWOULDBLOCK) {
			printf("Sendto() interrupted by timeout\n");
		}
		else
		{
			perror("udp client sendto() error: ");
			return FAILURE;
		}
	}//fi	
	
	printf("\nUDP client: sent %d bytes message '%s' to %s\n\n", (int)numbytes, buf, argv[1]);
	
	int quit = 0;
	while (!quit)
	{

		numbytes = recvfrom(sockfd, buf, BUFSIZE-1, 0, (struct sockaddr *)&serv, &len);
		if ( numbytes > 0) 
		{
			printf("\nReceived %d bytes from server\n", (int)numbytes);
			buf[numbytes] = 0;
			printf("Message was = '%s'\n", buf);
		} 
		else if (numbytes == 0)
		{
			printf("Received empty datagram... \n");
			quit = 1;
		}
		else
		{
			//se abbiamo abilitato l'opzione SO_RCVTIMEO, allora dobbiamo verificare il timeout
			if (errno == EWOULDBLOCK )
			{ 
				printf("Timeout su Recvfrom()\n");
			} 
			else
			{
				perror("UDP client recvfrom() error: ");
			}
			
		}//fi
	}//wend
	
	close(sockfd);
	
return 0;
}

/**@}*/
