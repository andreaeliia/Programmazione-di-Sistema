/**
@defgroup Group13 NonBlocking I/O 
@brief NonBlocking I/O 

Confronto tra la modalità bloccante e non bloccante delle Sockets API.
@{
*/
/**
@file 	EchoServer.c
@author Catiuscia Melle

@brief 	Implementazione di un UDP echo server, modalità bloccante.
*/

#include "Shares.h"

int main(int argc, char *argv[]){

	int status = 0;
	struct addrinfo hints, *result, *ptr;

	memset(&hints, 0, sizeof(hints)); //azzeramento della struttura
	hints.ai_family = AF_UNSPEC; //l'applicazione accetta sia indirizzi IPv4 che IPv6
	hints.ai_flags = AI_PASSIVE; //server side 
	hints.ai_socktype = SOCK_DGRAM; //tipo di socket desiderato 

	status = getaddrinfo(NULL, ECHO_PORT, &hints, &result);
	if (status != 0) 
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return FAILURE;
	}


	int sockfd = 0;
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) 
	{
		sockfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (sockfd == -1)
			continue;

		int reuse = 1;
		status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 1, sizeof(int));
		if (status == -1)
		{
			perror("setsockopt() error: ");
			close(sockfd);
			return FAILURE;
		}

		if (bind(sockfd, ptr->ai_addr, ptr->ai_addrlen) == 0)
			break;/* Success */

		close(sockfd);
	}//for


	if (ptr == NULL) 
	{
		/* No address succeeded */
		fprintf(stderr, "Could not bind\n");
		return FAILURE;
	}

	freeaddrinfo(result);     /* No longer needed */


	//prepariamo il server UDP
	printf("\tUDP echo server ready on port %s\n", ECHO_PORT);


	struct sockaddr_storage client_addr;
	socklen_t len = sizeof(struct sockaddr_storage);

	char buf[BUFSIZE] = ""; 
	char previous[BUFSIZE]; 
	memset(previous,0, BUFSIZE);

	char response[BUFSIZE] = ""; 
	ssize_t numbytes = 0;

	char IPv6[INET6_ADDRSTRLEN];

	int j = 0;

	int quit = 0;
	while(!quit)
	{

		//chiamata bloccante
		numbytes = recvfrom(sockfd, buf, BUFSIZE-1, 0, (struct sockaddr *)(&client_addr), &len);

		if (numbytes == -1)
		{
			perror("recvfrom() error: ");
			return FAILURE;
		}
		else if (numbytes == 0)
		{
			printf("Received an empty datagram\n");
			sendto(sockfd, JOKE, strlen(JOKE), 0, (struct sockaddr *)(&client_addr), len);
		}
		else
		{
			buf[numbytes] = 0;
			if (strncmp(previous, buf, strlen(buf)) == 0)
			{
				printf("Again the same message\t discard...\n");
				continue;
			}
			else
			{
				printf("New message: len = %d, '%s'\n", (int)numbytes, buf);
				memcpy(previous, buf, strlen(buf));
			}

			//elaborazione della richiesta...
			for (j=0; j < numbytes; j++)
			{
				buf[j] = toupper(buf[j]);
			}
			printf("Receive message from ");
			printf("\nmessage '%s'\n", buf);

			sleep(3);

			//invio della risposta
			numbytes = sendto(sockfd, response, strlen(response), 0, (struct sockaddr *)(&client_addr), len);
			if (numbytes == -1)
			{
				perror("sendto() error: ");
				close(sockfd);
				return FAILURE;
			}

		}//fi

	}//wend

	close(sockfd);

return 0;
}

/**@}*/
