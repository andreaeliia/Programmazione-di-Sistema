/**
@addtogroup Group14 
@{
*/
/**
@file 	MulticastReceiver.c
@author Catiuscia Melle
@brief 	Implementazione di un receiver multicast.

Il receiver si mette in ascolto su un gruppo 
ed una porta multicast definiti nell'header comune o forniti da linea di comando.
*/

#include "apue.h"
#include "Utility.h"


int join_group(int sockfd, struct sockaddr *multicastAddr, socklen_t addrlen){

	//address-family-specific code for Join Multicast Groups
  	if (multicastAddr->sa_family == AF_INET6) 
  	{
    	// join the multicast v6 "group" (address)
    	struct ipv6_mreq joinRequest;
    	
    	memcpy(&joinRequest.ipv6mr_multiaddr, &((struct sockaddr_in6 *) multicastAddr)->sin6_addr,  
    	sizeof(struct in6_addr));
     	
     	//index=0: Let the system choose the ingoing interface
    	joinRequest.ipv6mr_interface = 0; 
    	
    	printf("Joining IPv6 multicast group...\n");
    
    	if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &joinRequest, sizeof(joinRequest)) < 0)
    	{
    		perror("setsockopt(IPV6_JOIN_GROUP) failed: ");
    		return FAILURE;
		}
		return SUCCESS;
  	} 
  	else if (multicastAddr->sa_family == AF_INET) 
  	{
    	//join the multicast v4 "group" (address)
    	struct ip_mreq joinRequest;
    	//set the multicast IPv4 address
    	joinRequest.imr_multiaddr = ((struct sockaddr_in *) multicastAddr)->sin_addr;
    	
    	//the system choose the ingoing interface - wildcard IPv4 address
    	joinRequest.imr_interface.s_addr = htonl(INADDR_ANY);
    	
    	printf("Joining IPv4 multicast group...\n");
    	
    	if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &joinRequest, sizeof(joinRequest)) < 0)
    	{
    		perror("setsockopt(IPV4_ADD_MEMBERSHIP) failed: ");
    		return FAILURE;
		}
      return SUCCESS;
  	} 

//se siamo qui, qualcosa è andato male...
return FAILURE;  
}
 
 
 
/**
@brief Funzione per esplicitare il leave da un gruppo multicast.
@param sockfd, socket descriptor per cui interrompere la ricezione multicast
@param multicastAddr, socket address contenente l'indirizzo multicast da abbandonare
@param addrlen, dimensione del socket address 
@return intero, valore di ritorno della funzione setsockopt() dopo l'esecuzione del LEAVE.
*/
int leave_group(int sockfd, struct sockaddr *multicastAddr, socklen_t addrlen){
	
	if (multicastAddr->sa_family == AF_INET)
	{
		// Leave the multicast v4 "group"
		struct ip_mreq leaveRequest;
		leaveRequest.imr_multiaddr = ((struct sockaddr_in *) multicastAddr)->sin_addr;
    	leaveRequest.imr_interface.s_addr = htonl(INADDR_ANY);
    	
    	return (setsockopt(sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &leaveRequest, sizeof(leaveRequest)));
	}
	else if (multicastAddr->sa_family == AF_INET6)
	{
		// Leave the multicast v6 "group"
		struct ipv6_mreq leaveRequest;
		leaveRequest.ipv6mr_multiaddr = ((struct sockaddr_in6 *) multicastAddr)->sin6_addr;
    	leaveRequest.ipv6mr_interface =0;
    	
    	return (setsockopt(sockfd, IPPROTO_IPV6, IPV6_LEAVE_GROUP, &leaveRequest, sizeof(leaveRequest))); 
	}
	
return FAILURE;
}



int main(int argc, char *argv[]){
	
	char multicastAddrString[INET6_ADDRSTRLEN] = ""; //multicast address
	char multicastPortString[INET6_ADDRSTRLEN] = ""; //UDP port
	
	if (argc != 3)
  	{
    	printf("Usage: %s <Multicast Address> <Port>\n", argv[0]);
    	return FAILURE;
	}
	else
	{
		//multicast group address
		memcpy(multicastAddrString, argv[1], INET6_ADDRSTRLEN);
		
		//multicast port 
		memcpy(multicastPortString, argv[2], INET6_ADDRSTRLEN);
	}

	printf("Multicast Receiver for <%s:%s>\n", multicastAddrString, multicastPortString); 
	
	int result = 0;
	int sockfd = -1;
	
	struct addrinfo hints, *res, *ptr;

	//Zero out hints
	memset(&hints, 0, sizeof(hints)); 

  	//resolution hints:
	hints.ai_family = AF_UNSPEC;      // v4 or v6 is OK
  	hints.ai_socktype = SOCK_DGRAM;   // Only datagram sockets
  	hints.ai_protocol = IPPROTO_UDP;  // Only UDP protocol
  	hints.ai_flags |= AI_NUMERICHOST; // Don't try to resolve address
	hints.ai_flags |= AI_NUMERICSERV; // Don't try to resolve port

	int ecode = getaddrinfo(multicastAddrString, multicastPortString, &hints, &res);
  	if (ecode != 0)
  	{
    	printf("getaddrinfo() failed: %s", gai_strerror(ecode));
    	return FAILURE;
	}
	
	for (ptr = res; ptr != NULL; ptr=ptr->ai_next)
	{
		//open UDP socket
		sockfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (sockfd == -1)
		{	
			perror("socket() error: ");
			continue;	
		}
		
		//opzione per il riuso dell'indirizzo
		int reuse_on = 1;
	
		//allow address reuse
		result = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse_on, sizeof(reuse_on));
		if (result == -1)
		{
			perror("setsockopt() error: ");
			close(sockfd);
			continue;
		}
			
		//bind socket to multicast address and port
		result = bind(sockfd, ptr->ai_addr, ptr->ai_addrlen);
		if (result < 0)
		{
			perror("bind() error: ");
			close(sockfd);
			continue;
		}
		break;
	}//for
	
	if (ptr == NULL)
	{	/* errno set from final socket() */
		printf("%s error for %s, %s\n", argv[0], multicastAddrString, multicastPortString);
		freeaddrinfo(res);
		return FAILURE;
	}

	//sockfd is binded to the multicast address and port
	//now we must join the group
	
	result = join_group(sockfd, ptr->ai_addr, ptr->ai_addrlen);
	if (result == FAILURE)
	{
		//chiudo e dealloco
		close(sockfd);
		freeaddrinfo(res);
		return FAILURE;
	}
	
	
	//handle_multicast_session(sockfd);
	int quit = 0;
	ssize_t nbytes = 0;
	char MultiMessage[BUFSIZE] = "";

	struct sockaddr_storage source;
	socklen_t source_len = sizeof(source);

	//printf("Ready for receiving multicast traffic\n");
	
	char ip[INET6_ADDRSTRLEN] = "";
	char port[INET6_ADDRSTRLEN] = "";
	char position[9] = "{40,40}";
	int		x=40,y=40;
	
	//CanonicalMode
	if (tty_raw(STDIN_FILENO) < 0)
				printf("ERRORE TTY");
				
	while (!quit)
	{	
		
		nbytes = recvfrom(sockfd, position, 9, 0,\
						 (struct sockaddr *)&source, &source_len);
		if (nbytes < 0)
		{
			perror("recvfrom() error: ");
			quit = 1;
		}
		else
		{   
			x = strtol(&position[1], NULL, 10);
			y = strtol(&position[4], NULL, 10);
			printf("\033[2A"); 
			printf("\033[2J");
			printf("\033[%d;%dH", y, x);
			printf("🥰{%d,%d}",x,y);
			fflush(stdout);
				
		}
	}//wend
		
	
	result = leave_group(sockfd, ptr->ai_addr, ptr->ai_addrlen);
	if (result == -1)
	{
		perror("setsockopt error: ");
	}
	
	
	//chiudo e dealloco
	close(sockfd);
	freeaddrinfo(res);
	
return 0;
}

/** @} */
