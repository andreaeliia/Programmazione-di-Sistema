/**
@addtogroup Group13
@{
*/
/**
@file 	UDP-Client.c
@author Catiuscia Melle

@brief 	Esempio d'uso della modalità non bloccante dei socket con un client UDP 

In questo esempio, se attivato il blocking flag come parametro al lancio del programma, il client UDP mette il socket in modalità non bloccante dopo l'invio del messaggio al server. 
Poiché il server risponderà con un ritardo di 5 secondi, durante questo 
intervallo di tempo il client - come se avesse constatato una perdita del pacchetto precedente - reinvia il messaggio ad intervalli di 2 secondi fino alla ricezione del messaggio di risposta.
Se il blocking mode sul socket non è attivato, il client UDP utilizza il flag MSG_DONTWAIT per la singola operazione di I/O sul socket. 
Il flag MSG_DONTWAIT, utilizzato in una send/recv o in una sendto/recvfrom, consente di rendere la singola operazione di I/O non bloccante 
(<em> on-operation basis instead that on socket-basis</em>).
L'unico problema è che sebbene l'opzione sia implementata in molti sistemi Unix, 
 non è riportata dallo standard SuSv4.

*/

#include "Shares.h"



int main(int argc, char *argv[]){
	
	if (argc != 4) 
	{
		fprintf(stderr,"Usage: %s <servername> <message> <blocking>\n", argv[0]);
		fprintf(stderr, "blocking=1 to set non-blocking mode\n");
		return FAILURE;
	}
	int blocking_mode = atoi(argv[3]);

	int rv, sockfd;
	struct addrinfo hints, *servinfo, *p;
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags |= AI_NUMERICSERV;
	
	rv = getaddrinfo(argv[1], ECHO_PORT, &hints, &servinfo);
	if ( rv != 0) 
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return FAILURE;
	}

	// loop through all the results and make a socket
	for (p = servinfo; p != NULL; p = p->ai_next) 
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) 
		{
			perror("socket() error: ");
			continue;
		}

		break;
	}//for

	if (p == NULL) 
	{
		fprintf(stderr, "%s: failed to open socket\n", argv[0]);
		return FAILURE;
	}
	
	//salvo l'indirizzo del server
	struct sockaddr_storage server;
	memset(&server, 0, sizeof(struct sockaddr_storage));
	memcpy(&server, p->ai_addr, p->ai_addrlen);
	socklen_t serverlen = p->ai_addrlen;
	
	//dealloco risorse del resolver
	freeaddrinfo(servinfo);
	
	char buf[BUFSIZE] = ""; 
	ssize_t numbytes;
	int count = 0;
	
	//numbytes = sendto(sockfd, argv[2], strlen(argv[2]), 0, p->ai_addr, p->ai_addrlen);
	numbytes = sendto(sockfd, argv[2], strlen(argv[2]), 0, (struct sockaddr *)&server, serverlen);
	if (numbytes == -1)
	{
		perror("Blocking sendto() error: ");
		close(sockfd);
		return FAILURE;
	}
	
	printf("%d) Sent %d bytes, \t", ++count, (int)numbytes);
	
	if (blocking_mode)
	{
		//settiamo il socket in modalità non bloccante
		rv = set_nonblock(sockfd);
		if (rv == FAILURE)
		{
			close(sockfd);
			return FAILURE;
		}
	}//fi blocking_mode
	
	struct sockaddr_storage peer;
	socklen_t len = sizeof(peer);
	
	int quit = 0;
	while (!quit)
	{
		printf("Wait for a response..\n");
		
		if (blocking_mode)
		{
			//non-blocking mode on sockfd 
			numbytes = recvfrom(sockfd, buf, BUFSIZE-1, 0, (struct sockaddr *)&peer, &len);
		
			if (numbytes == -1 && errno == EWOULDBLOCK)
			{
				printf("No datagram arrived at all\n%d) Send again, ", ++count);
				numbytes = sendto(sockfd, argv[2], strlen(argv[2]), 0, (struct sockaddr *)&server, serverlen);
				if (numbytes == -1)
				{
					perror("NON-Blocking sendto() error: ");	
					continue;
				}
				sleep(2);
			} 
			else if (numbytes >= 0)
			{
				quit = 1;
			}
		} 
		else 
		{
			//non-blocking recvfrom() call, with flag
			numbytes = recvfrom(sockfd, buf, BUFSIZE-1, MSG_DONTWAIT, (struct sockaddr *)&peer, &len);
			
			if ((numbytes == -1) && (errno == EWOULDBLOCK))
			{
				printf("No datagram arrived at all\n%d) Send again, ", ++count);
				numbytes = sendto(sockfd, argv[2], strlen(argv[2]), 0, (struct sockaddr *)&server, serverlen);
				if (numbytes == -1)
				{
					perror("Blocking sendto() error: ");	
					continue;
				}
				sleep(2);
			}
			else if (numbytes >= 0)
			{
				buf[numbytes]=0;
				printf("\n\tClient received:\n'%s'\n", buf);
				break;
			}
			else
			{
				perror("Error on nonblocking recvfrom(): ");
				break;
			}//fi
		}//fi non-blocking
	}//wend
	
	close(sockfd);

return 0;
}

/**@}*/