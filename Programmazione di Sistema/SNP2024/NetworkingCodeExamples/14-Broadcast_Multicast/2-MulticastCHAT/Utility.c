/**
@addtogroup Group14
@{
*/
/**
@file 	Utility.c
@author Catiuscia Melle
*/

#include "Utility.h"

void getAddress(struct sockaddr *addr, socklen_t addrlen, char *result, int size){
	
	int niflags = NI_NUMERICHOST | NI_NUMERICSERV;
	char ip[INET6_ADDRSTRLEN] = "";
	char port[INET6_ADDRSTRLEN] = "";
	int ecode = getnameinfo(addr, addrlen, \
			ip, INET6_ADDRSTRLEN, port,INET6_ADDRSTRLEN, niflags);
	if (ecode)
	{
		printf("getnameinfo error: %s\n", gai_strerror(ecode));
		result = NULL;
	}
	else
	{
		snprintf(result, size -1, "%s:%s", ip, port);
	}

}


void read_addrinfo(struct addrinfo *res){
	
	char buf4[INET_ADDRSTRLEN];
	char buf6[INET6_ADDRSTRLEN];
	int result = 0;
	struct addrinfo *ptr;
	
	for (ptr = res; ptr != NULL; ptr=ptr->ai_next)
	{	
		if (ptr->ai_family == AF_INET)
		{
			printf("ipv4\n");
			struct sockaddr_in *ipv4 = (struct sockaddr_in *)ptr->ai_addr;
			inet_ntop(AF_INET, &(ipv4->sin_addr), buf4, INET_ADDRSTRLEN);
			printf("addr = %s\n", buf4); 
		}
			
		if (ptr->ai_family == AF_INET6)
		{
			printf("ipv6\n");
			struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)ptr->ai_addr;	
			inet_ntop(AF_INET6, &(ipv6->sin6_addr), buf6, INET6_ADDRSTRLEN);
			printf("addr = %s\n", buf6); 
		}
			
		printf("flags = %d  ", ptr->ai_flags);
		printf("family = %d  ", ptr->ai_family);
		printf("socktype = %d ", ptr->ai_socktype); 
		printf("protocol = %d ", ptr->ai_protocol);
		printf(" canonname = '%s'\n", ptr->ai_canonname);
	}//for-wend
    	
}



void handle_multicast_session(int sockfd){
	
	int quit = 0;
	ssize_t nbytes = 0;
	char MultiMessage[BUFSIZE] = "";
	
	struct sockaddr_storage source;
	socklen_t source_len = sizeof(source);
	
	//printf("Ready for receiving multicast traffic\n");
	
	while (!quit)
	{
		nbytes = recvfrom(sockfd, MultiMessage, BUFSIZE-1, 0,\
						 (struct sockaddr *)&source, &source_len);
		if (nbytes < 0)
		{	
			perror("recvfrom() error: ");
			quit = 1;
		} 
		else 
		{
			MultiMessage[nbytes]=0;
			char sourceAddress[INET6_ADDRSTRLEN*2] = "";
			getAddress((struct sockaddr *)&source, source_len, sourceAddress, INET6_ADDRSTRLEN*2);

			color_print(MultiMessage, sourceAddress);
			
			if (isMsgQuit(MultiMessage))
			{
				printf("Quit received\n..closing\n");
				quit = 1;
			}
		}
	}//wend
}




void emitMulticast(int sockfd, char *msg, struct sockaddr *MulticastDest, socklen_t destLen){

	ssize_t numBytes = 0;
	int quit = 0;
	int count = 0;
	while (!quit)
	{ 
  		// Run forever of for a fixed number of times
    	if (count == THRESHOLD)
    	{
    		msg = QUIT;
    		quit = 1;
    	}
    	
    	printf("Emit a new multicast message: %d - '%s'\n", count, msg);
    	
    	// Multicast the string to all who have joined the group
    	numBytes = sendto(sockfd, msg, strlen(msg), 0, MulticastDest, destLen);
    	
    	if (numBytes < 0)
    	{
    		perror("sendto() failed: ");
    		quit = 1;
		} 
		else if (numBytes != strlen(msg))
		{
    		printf("sendto() sent unexpected number of bytes\n");
    		quit = 1;
		}
		
      	count++;
    	sleep(FREQ);
    	
  	}//wend 	
}



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


int join_group(int sockfd, struct sockaddr *multicastAddr, socklen_t addrlen){

	// Unfortunately we need some address-family-specific pieces
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



bool isMsgQuit(char *msg){
	return (strncmp(msg, QUIT, strlen(msg)) == 0);
}



int setTTL(int sockfd, int TTL, int family){
  	
  	if (family == AF_INET6) 
  	{ 	
  		// v6-specific
    	// The v6 multicast TTL socket option requires that the value be
    	// passed in as an integer
    	if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &TTL, sizeof(TTL)) < 0)
    	{
    		perror("setsockopt(IPV6_MULTICAST_HOPS) failed: ");
    		return FAILURE;
		}
      return SUCCESS;
  	} 
  	else if (family == AF_INET) 
  	{ 
  		// v4 specific
    	// The v4 multicast TTL socket option requires that the value be
    	// passed in an unsigned char
    	//casting esplicito
    	u_char multicastTTL = (u_char)TTL;
    	if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, &multicastTTL, sizeof(multicastTTL)) < 0)
    	{
    		perror("setsockopt(IP_MULTICAST_TTL) failed: ");
    		return FAILURE;
		}
		return SUCCESS;
	} 

//qui in caso di errore
return FAILURE;
}


int disableLoop(int family, int sockfd){
	
	if (family == AF_INET)
	{
		printf("Disable loop for IPv4\n");
		u_char loop = 0;
		return ( setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)));
	}
	
	if (family == AF_INET6)
	{
		printf("Disable loop for IPv6\n");
		u_int loop = 0;
		return ( setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &loop, sizeof(loop)));
	}

//non dovrei arrivarci	
return FAILURE;
}	


void color_print(char *msg, char *from){
	printf("%ssource:%s - '%s'%s\n", "\x1B[31m",from, msg, "\x1B[0m");
}


/**@}*/