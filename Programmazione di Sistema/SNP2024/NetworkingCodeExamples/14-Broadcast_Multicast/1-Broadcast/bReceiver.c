/**
@addtogroup Group14
@{
*/
/** 
@file 	bReceiver.c
@author Catiuscia Melle
@brief 	Implementazione di un receiver UDP di traffico broadcast.

Un receiver broadcast deve solo eseguire il bind del suo socket UDP alla porta prescelta, 
affinché il kernel possa correttamente assegnargli i pacchetti in arrivo dalla rete.

*/

#include "Header.h"


int main(int argc, char *argv[]){
	
	/*
	if (argc != 2)
	{
		printf("Usage: %s <broadcast port>\n", argv[0]);
		return FAILURE;
	}
	*/
	
	
	int sockfd = 0;
	int status = 0;
	struct addrinfo hints, *res, *ptr;
	
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	
	status = getaddrinfo(NULL, SERVICEPORT, &hints, &res);
	if (status != 0)
	{
		printf("%s error for %s port:\n\t%s", argv[0], SERVICEPORT, gai_strerror(status));
		return FAILURE;
	}
		
	ptr = res;
	while (ptr != NULL) 
	{
		sockfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (sockfd < 0)
		{
			close(sockfd);
			ptr = ptr->ai_next;
			continue; //success
		}
					
		//lego l'indirizzo al socket	
		status = bind(sockfd, ptr->ai_addr, ptr->ai_addrlen);
		if (status == 0)
			break;
			
		close(sockfd);
		ptr = ptr->ai_next;
	}//wend
	
	
	if (ptr == NULL)
	{	
		/* errno set from final socket() */
		printf("%s error for %s port\n", argv[0], SERVICEPORT);
		return FAILURE;
	}
	
	//deallochiamo res
	freeaddrinfo(res);
	
	printf("Ready for receiving on port %s\n", SERVICEPORT);	
	
	
	int quit = 0;
	ssize_t nbytes = 0;
	char recv_buffer[BUFSIZE] = "";
	
	struct sockaddr_storage bsource;
	memset(&bsource, 0, sizeof(struct sockaddr_storage));
	socklen_t bsource_len = sizeof(bsource);
	
	int niflags = NI_NUMERICSERV | NI_NUMERICHOST;
	
	char ip[INET6_ADDRSTRLEN] = "";
	char port[INET6_ADDRSTRLEN] = "";
	
	int ecode = 0;
	
	while (!quit)
	{
		//wait for a message
		nbytes = recvfrom(sockfd, recv_buffer, BUFSIZE-1, 0, (struct sockaddr *)&bsource, &bsource_len);
		if (nbytes < 0)
		{	
			perror("recvfrom() error: ");
			quit = 1;
		} 
		else 
		{
			recv_buffer[nbytes]=0;
			ecode = getnameinfo((struct sockaddr *)&bsource, bsource_len, \
					ip, INET6_ADDRSTRLEN, port, INET6_ADDRSTRLEN, niflags);
			if (ecode)
			{
				printf("getnameinfo error: %s\n", gai_strerror(ecode));
			}
			else
			{
				printf("Received %d bytes from: %s:%s\n", (int)nbytes, ip, port);
				printf("\tmessage = '%s'\n", recv_buffer);
			}
			
			if (strncmp(recv_buffer, QUIT, strlen(QUIT)) == 0)
			{
				printf("Quit message received\n");
				quit = 1;
			}
		}//fi
		
		nbytes = 0;
		memset(recv_buffer, 0, BUFSIZE);
	}//wend

	close(sockfd);

return 0;
}

/** @} */ 
