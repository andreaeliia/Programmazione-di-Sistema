/**
@addtogroup Group10
@{
*/
/**
@file 	UDPClient.c
@author Catiuscia Melle

@brief 	Implementazione di un client UDP

All'avvio, oltre a specificare un messaggio ed il server da contattare, è possibile 
attivare la modalità connessa o meno (se si inserisce 1, il socket UDP verrà connesso). 
Il client quindi apre un socket UDP dal quale contatta il server.
Se si inserisce "1" per il parametro <em>connected</em>, allora la modalità connessa 
viene attivata sul socket e sarà possibile le info locali del socket UDP 
con <em>getsockname()</em> ed invocare send() e recv().
Altrimenti, si farà riferimento al workflow base delle chiamate UDP.
*/

#include "Header.h"


/**
@brief Utility 
@param name - nome del programma
@return nulla
*/
void usage(char *name){
	
	printf("Usage: %s <servername> <domain> <message> <connected>\n", name);
	
	printf("\tdomain=0 => AF_UNSPEC domain\n");
	printf("\tdomain=4 => AF_INET domain\n");
	printf("\tdomain=6 => AF_INET6 domain\n");
	
	printf("\tconnected=1 to connect the UDP socket to the destination\n");
}

/**
@brief Utility function per la visualizzazione dell'indirizzo associato ad un socket,
protocol-independent
@param addr, ptr alla struct sockaddr da leggere
@param len, dimensione della struttura puntata da addr
@return nulla
*/
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



int main(int argc, char *argv[]) {
	
	if (argc != 5) 
	{
		usage(argv[0]);
		return INVALID;
	}

	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	
	memset(&hints, 0, sizeof hints);
	
	//specifico il dominio di comunicazione (e di indirizzi richiesti)
	switch ( atoi(argv[2]) ){
		case 0:	
			hints.ai_family = AF_UNSPEC;
			break;
		case 4: 
			hints.ai_family = AF_INET;
			break; 
		case 6: 
			/*
			per favorire l'interoperabilità IPv4-IPv6, se richiedo 
			AF_INET6 come dominio di comunicazione devo essere preparato alla 
			possibilità che il server non abbia un AAAA Resource Record associato al  suo
			hostname e richiedere gli indirizzi IPv4-mapped-IPv6 
			*/
			hints.ai_family = AF_INET6;
			hints.ai_flags |= AI_V4MAPPED;
			break;
	}//switch
	
	hints.ai_socktype = SOCK_DGRAM;


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
		fprintf(stderr, "%s: failed to open socket\n", argv[0]);
		return FAILURE;
	}
	
	ssize_t numbytes;
	char buf[BUFSIZE];
	char IPv6[INET6_ADDRSTRLEN];
	
	if (strncmp(argv[4],"1", 1) == 0)
	{
		//eseguiamo la connect sul socket descriptor...
		rv = connect(sockfd, p->ai_addr, p->ai_addrlen);
		if (rv != 0 )
		{
			perror("UDP connect() error: ");
			close(sockfd);
			return FAILURE;
		}//fi

		//quindi posso liberare la linked-list...
		freeaddrinfo(servinfo);
	
		struct sockaddr_storage local;
		socklen_t locallen = sizeof(struct sockaddr_storage);
		memset(&local, 0, locallen);

		//con un socket connesso possiamo leggere i dati locali del socket
		rv = getsockname(sockfd, (struct sockaddr *)&local, &locallen);
		if (rv < 0)
		{
			perror("error on getsockname: ");
			close(sockfd);
			return FAILURE;
		}
		
		//no reverse lookup in getnameinfo
		int niflags = NI_NUMERICSERV | NI_NUMERICHOST;
		char myIP[INET6_ADDRSTRLEN] = "";
		char myport[PORT_STRLEN] = "";
		
		//visualizzo l'indirizzo locale del socket
		rv = getnameinfo((struct sockaddr *)(&local), locallen, myIP, INET6_ADDRSTRLEN, myport, PORT_STRLEN, niflags);
		
		if (rv == 0)
		{
			printf("***** Local Address  '%s:%s'\n", myIP, myport);
		}
		else
		{
			printf("getnameinfo() error: %s\n", gai_strerror(rv));
		}

		//posso ora eseguire la send invece di sendto
		numbytes = send(sockfd, argv[3], strlen(argv[3]), 0);
		if (numbytes == -1)
		{
			perror("UDP send() error: ");
			close(sockfd);
			return FAILURE;
		}
		printf("UDP client: sent %d bytes\n", (int)numbytes);
	
	
		//ma possiamo leggere anche i dati remoti
		rv = getpeername(sockfd, (struct sockaddr *)&local, &locallen);
		if (rv < 0)
		{
			perror("error on getpeername: ");
		}
		printf("Connected to remote peer address ");
		printAddressInfo((struct sockaddr *)&local, locallen);
		printf("\n");
		
		numbytes = recv(sockfd, buf, BUFSIZE-1, 0);
		if (numbytes == -1)
		{
			perror("UDP recv() error: ");
			close(sockfd);
			return FAILURE;
		}
		else
		{
			buf[numbytes] = 0;
			printf("Received %d bytes Message '%s'\n", (int)numbytes, buf);
		}
	}
	else
	{ 
		//socket non connesso
		numbytes = sendto(sockfd, argv[3], strlen(argv[3]), 0, p->ai_addr, p->ai_addrlen);
		if (numbytes == -1) 
		{
			perror("udp sendto() error: ");
			close(sockfd);
			return FAILURE;
		}
		
		//quindi posso liberare la linked-list...
		freeaddrinfo(servinfo);
		
		struct sockaddr_storage peer;
		socklen_t len = sizeof(struct sockaddr_storage);
		memset(&peer, 0, len);
		
		//con un socket non connesso possiamo leggere i dati locali del socket dopo l'invio
		rv = getsockname(sockfd, (struct sockaddr *)&peer, &len);
		if (rv < 0)
		{
			perror("error on getsockname: ");
			close(sockfd);
			return FAILURE;
		}
		printf("Local UDP Address: ");
		printAddressInfo((struct sockaddr *)&peer, len);

		//blocks in recvfrom()
		numbytes = recvfrom(sockfd, buf, BUFSIZE-1, 0, (struct sockaddr *)&peer, &len);
		if (numbytes == -1) 
		{
			perror("udp recvfrom() error: ");
			close(sockfd);
			return FAILURE;
		}
		else
		{
			buf[numbytes]=0;
			printf("\n\tReceived '%s' from ", buf);
			//visualizzo l'indirizzo del peer che mi ha contattato
			printAddressInfo((struct sockaddr *)&peer, len);
		}

	}//fi connect
	printf("\nClosing socket\n");
	close(sockfd);

return 0;
}

/**@}*/