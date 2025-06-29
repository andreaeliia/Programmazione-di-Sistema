/**
@addtogroup Group31
@{
*/
/**
@file 	TCPC.c
@author Catiuscia Melle
@brief 	Implementazione di un TCP client classico che utilizza le opzioni 
		SO_RCVTIMEO e SO_SNDTIMEO sul socket.
*/

#include "Utility.h"


int main(int argc, char *argv[]){

    if (argc != 3) 
    {
		fprintf(stderr,"Usage: %s <ServerName> <message>\n", argv[0]);
		return FAILURE;
	}
	
	
	struct addrinfo hints, *result, *ptr;
    char buf[BUFSIZE] = "";
    
	int quit = 0;
	
	
	char srvname[BUFSIZE] = "";
	memcpy(&srvname, argv[1], strlen(argv[1]));
	
    memset(&hints, 0, sizeof(hints)); //azzeramento della struttura
    hints.ai_family = AF_UNSPEC; //l'applicazione accetta sia indirizzi IPv4 che IPv6
    hints.ai_socktype = SOCK_STREAM; //tipo di socket desiderato 
   
    int status = 0;
	
	status = getaddrinfo(srvname, SRV_PORT, &hints, &result);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return FAILURE;
    }
	
	//read_addrinfo(result);

	int sockfd = 0;
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) 
    {
        sockfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (sockfd == -1)
            continue;
		
		/*
		Queste non hanno effetto sui tempi della connect(), ma attenzione:
		possono contrastare l'effetto di una mancata accept() lato server.
		*/
		struct timeval tv;
		socklen_t len = sizeof(tv);
		
		status = getsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, &len);
		if (status == 0)
		{
			printf("SO_RCVTIMEO = %d sec e %d usec\n", (int)tv.tv_sec, (int)tv.tv_usec);
		}
		
		/**/
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		
		status = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if (status != 0)
			return FAILURE;
		
		status = setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
		if (status != 0)
			return FAILURE;
		printf("Setted timeout for send/recv operation: %d:%d\n", (int)tv.tv_sec, (int)tv.tv_usec);
		/**/
        
        if (connect(sockfd, ptr->ai_addr, ptr->ai_addrlen) == -1)
        {
        	if (errno == EWOULDBLOCK) 
        	{
				printf("connect() EWOULDBLOCK timeout\n");
			}        	
        	close(sockfd);
        	perror("client connect() error: ");
        	continue;
    	}
    	
    	break;
    }


    if (ptr == NULL) {         /* No address succeeded */
        fprintf(stderr, "Client failed to connect\n");
        return FAILURE;
    }
	
	char IPv4[INET_ADDRSTRLEN] = "";
	char IPv6[INET6_ADDRSTRLEN] = "";
	
	if (ptr->ai_addr->sa_family == AF_INET)
	{
		struct sockaddr_in *ip = (struct sockaddr_in *)ptr->ai_addr;
		inet_ntop(AF_INET, &(ip->sin_addr), IPv4, INET_ADDRSTRLEN);
		printf("client connected to: %s\n", IPv4);
	} else if (ptr->ai_addr->sa_family == AF_INET6){
			struct sockaddr_in6 *ip = (struct sockaddr_in6 *)ptr->ai_addr;
			inet_ntop(AF_INET6, &(ip->sin6_addr), IPv6, INET6_ADDRSTRLEN);
			printf("client connected to: %s\n", IPv6);
	}
	
	freeaddrinfo(result);     /* No longer needed */
	ssize_t numbytes = 0;
	int i = 1;
	while (!quit){
		numbytes = send(sockfd, argv[2], strlen(argv[2]), 0);
	
		if (numbytes == -1){
			perror("send");
			close(sockfd);
			return FAILURE;
		}else{
			printf("sent message of %d bytes\n", (int)numbytes);
			i++;
		}
	
		numbytes = recv(sockfd, buf, BUFSIZE-1,0);
		if (numbytes == -1)
		{
			if (errno == EWOULDBLOCK)
				printf("attenzione, scaduto il timeout fissato per la recv\nServer not responding\n");
			
			return FAILURE;
		
		} else { 
			buf[numbytes] = 0;
			printf("Client received '%s' from server\n", buf);
		}
		
		sleep(2);
		if (i==10) quit = 1;
	}
	close(sockfd);
	
return 0;
}
/**@}*/