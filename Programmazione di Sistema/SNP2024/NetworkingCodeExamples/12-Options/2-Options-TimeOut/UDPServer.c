/** 
@addtogroup Group12
@{
*/
/**
@file 	UDPServer.c
@author Catiuscia Melle

@brief 	Implementazione di un server UDP

Il server dell'esempio invia un messaggio vuoto, un messaggio di Echo e dopo 3 secondi un 
messaggio conclusivo.

*/
#include "Shares.h"

void printAddressInfo(struct sockaddr * addr, socklen_t len){
	
	//no reverse lookup in getnameinfo
	int niflags = NI_NUMERICSERV | NI_NUMERICHOST;
	
	char IP[INET6_ADDRSTRLEN] = "";
	char port[PORT_STRLEN] = "";
	
	//visualizzo l'indirizzo locale del socket
	int rv = getnameinfo(addr, len, IP, INET6_ADDRSTRLEN, port, PORT_STRLEN, niflags);
	
	if (rv == 0)
	{
		printf("'%s:%s'", IP, port);
	}
	else
	{
		printf("getnameinfo() error: %s\n", gai_strerror(rv));
	}
}



int main(int argc, char *argv[]){

    printf("\tUDP Server example\n");
 	
 	int status = 0;
	int sockfd = 0;
	   
    struct addrinfo hints, *result, *ptr;
	
    memset(&hints, 0, sizeof(hints)); //azzeramento della struttura
    
    hints.ai_family = AF_INET6; 
    hints.ai_socktype = SOCK_DGRAM; //tipo di socket desiderato 
    hints.ai_flags = AI_PASSIVE; //server side 
    hints.ai_flags = AI_V4MAPPED; 
    hints.ai_flags |= AI_NUMERICSERV; 
    
    status = getaddrinfo(NULL, SERVICEPORT, &hints, &result);
    if (status != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return FAILURE;
    }


    /* getaddrinfo() returns a list of address structures.
       Try each address until a successful bind().
       If socket(2) (or bind(2)) fails, close the socket
       and try the next address. */
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) 
    {
        sockfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (sockfd == -1)
            continue;

        if (bind(sockfd, ptr->ai_addr, ptr->ai_addrlen) == 0)
            break;            /* Success */

        close(sockfd);
    }//for


    if (ptr == NULL) 
    {         /* No address succeeded */
        fprintf(stderr, "Could not bind\n");
        return FAILURE;
    }
    
    freeaddrinfo(result);     /* No longer needed */

	
	
	//prepariamo il server UDP
	printf("\tUDP echo server ready on port %s\n", SERVICEPORT);
	
	/*
	poiché il mittente del datagram non è noto a priori, memorizziamo in una struct
	sockaddr_storage l'info ritornata col datagram UDP.
	*/
	struct sockaddr_storage client_addr; 
	socklen_t len = sizeof(struct sockaddr_storage);
	
	char buf[BUFSIZE] = ""; 
	ssize_t numbytes = 0;
	char nullstring[BUFSIZE] = "";
	
	int quit = 0;
	int j = 0;
	
	
	while( !quit)
	{
		//chiamata bloccante
		numbytes = recvfrom(sockfd, buf, BUFSIZE-1, 0, (struct sockaddr *)(&client_addr), &len);
		if (numbytes == -1)
		{
			perror("UDP server recvfrom error: ");
			return FAILURE;
		}
		
		printf("Received packet from:");
		printAddressInfo((struct sockaddr *)(&client_addr), len);
		
		buf[numbytes] = 0;
		printf("\nMessage: len = %d\t'%s'\n", (int)numbytes, buf);
		
		for (j = 0; j < strlen(buf); j++)
		{
			buf[j] = toupper(buf[j]);
		}
		
		//invio il messaggio ricevuto
		numbytes = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)(&client_addr), len);
		if (numbytes < 0)
		{
			perror("UDP sendto() error: ");
			return FAILURE;
		}
		
		//invio metà del messaggio ricevuto
		numbytes = sendto(sockfd, buf, strlen(buf)/2, 0, (struct sockaddr *)(&client_addr), len);
		if (numbytes < 0)
		{
			perror("UDP sendto() error: ");
			return FAILURE;
		}
		
		printf("Sent two messages, waiting 3 seconds\n");
		
		sleep(3);
		
		//invio 0 bytes
		numbytes = sendto(sockfd, buf, 0, 0, (struct sockaddr *)(&client_addr), len);
		if (numbytes < 0)
		{
			perror("UDP sendto() error: ");
			return FAILURE;
		}

		printf("Sent final message after 3 seconds\n\n");
	}//wend
		
	close(sockfd);
	
return 0;
}

/**@}*/


