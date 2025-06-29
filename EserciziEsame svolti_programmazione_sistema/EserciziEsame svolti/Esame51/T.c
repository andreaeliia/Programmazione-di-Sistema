/**
@addtogroup Group14
@{
*/
/**
@file 	MulticastSender.c
@author Catiuscia Melle
@brief 	Implementazione di un sender ad un gruppo multicast

Non necessita di nessuna opzione, se non l'invio UDP al gruppo (e porta) multicast.
*/


#include "Utility.h"


/**
@brief Specifica il TTL per i pacchetti multicast.
@param sockfd - socket per cui settare il TTL
@param TTL - valore da settare;
@return intero, stato dell'esecuzione.

L'opzione TTL richiede codice address-family-specific perché:
- IPv6 richiede l'opzione IPV6_MULTICAST_HOPS di tipo <em>int</em>;
- IPv4 richiede l'opzione IP_MULTICAST_TTL di tipo <em>u_char</em>;
*/   	
int setTTL(int sockfd, int TTL, int family){
  	
  	if (family == AF_INET6) 
  	{ 
  		// v6-specific
    	// The v6 multicast TTL socket option requires that the value be
    	// passed in as an integer
		u_int multicastHop = TTL;
    	if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &multicastHop, sizeof(multicastHop)) < 0)
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

char posizione[9] = "{40,40}";
void randomPosition();




int main(int argc, char *argv[]) { 

  	if (argc != 3) 
  	{
    	printf("Usage: %s <Multicast Address> <Port> ", argv[0]);
    	return FAILURE;
	}
	
  	char *multicastIPString = argv[1];   // First:  multicast IP address
  	char *service = argv[2];             // Second: multicast port (or service)
  	
  	struct addrinfo hints, *res, *p;                   
  	
	// Zero out structure
	memset(&hints, 0, sizeof(hints)); 
  	
	//resolution criteria
  	hints.ai_family = AF_UNSPEC;            // v4 or v6 is OK
  	hints.ai_socktype = SOCK_DGRAM;         // Only datagram sockets
  	hints.ai_protocol = IPPROTO_UDP;        // Only UDP please
  	hints.ai_flags |= AI_NUMERICHOST;       // Don't try to resolve address

	//only if port is a numeric string: Don't try to resolve port
	hints.ai_flags |= AI_NUMERICSERV;       
		
  	int ecode = getaddrinfo(multicastIPString, service, &hints, &res);
  	if (ecode != 0)
  	{
    	printf("getaddrinfo() failed: %s", gai_strerror(ecode));
    	return FAILURE;
	}
	
  	int sockfd = -1;
  	
  	for (p=res; p != NULL; p=p->ai_next)
  	{
  		// Create socket 
  		sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
  		if (sockfd < 0)
    	{
    		perror("socket() failed: ");
    		continue;
		}
		break;
	}//for
	
	if (p == NULL)
	{
		printf("Unable to open socket for multicast transmission\n");
		freeaddrinfo(res);
		return FAILURE;
	}
	
		
  	/*
  	Fourth arg (optional):  
  	TTL for transmitting multicast packets
  	Default = 1: hop limit al link-local scope...
  	*/
  	int multicastTTL = (argc == 5) ? atoi(argv[4]) : 1;

  	ecode = setTTL(sockfd, multicastTTL, p->ai_family);
  	if (ecode == FAILURE)
  	{
  		perror("setsocktopt error for TTL: ");
  	}
  	
	//invio multicast
	ssize_t numBytes = 0;
 	int quit = 0;
  	
 	
	
	
	/*
	struct timeval tv = {3,0};
	ecode = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	if (ecode == -1){
		perror("setsockopt error for RCVTimeO: ");
	}else{
		printf("set a 3 seconds timeout on sockfd\n");
	}
	
	char response[BUFSIZE] = "";
	*/

	while (!quit)
  	{ 
		
		
		randomPosition();
		
		// Multicast the string to all who have joined the group
		numBytes = sendto(sockfd, posizione, strlen(posizione), 0,  p->ai_addr, p->ai_addrlen);
		if (numBytes < 0)
		{
			perror("sendto() failed: ");
			quit = 1;
		}
		
		sleep(3);
	  	
	
	}//wend
	
 	//on close:
  	close(sockfd);
  	freeaddrinfo(res); 	
  	
return 0;
}

/** @} */

void randomPosition(){
int x = strtol(&posizione[1], NULL, 10);
		int y = strtol(&posizione[4], NULL, 10);
		
		srand(time(NULL));
		int x1;
		int randx = rand() % 3 - 1;
		while((x1 = randx + x) > 80 || x1 < 0)
			randx = rand() % 3 - 1;
		
		int y1;
		int randy = rand() % 3 - 1;
		while((y1 = randy + y) > 80 || x1 < 0)
			randy = rand() % 3 - 1;
		
		x = x + randx;
		y = y + randy;
		
		sprintf(posizione, "{%d,%d}",x,y);
		printf("\n posizione %s\n",posizione);
}
