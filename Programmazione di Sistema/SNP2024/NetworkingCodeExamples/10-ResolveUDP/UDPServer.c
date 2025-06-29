/**
@defgroup Group10 UDP Client-Server communications with resolution
@brief UDP Client-Server communications with resolution

Introduzione alle Sockets API per UDP ed utilizzo di <em>getaddrinfo()</em>
@{
*/
/**
@file 	UDPServer.c
@author Catiuscia Melle

@brief 	Presentazione di un UDP Echo Server con hostname resolution.

Il server UDP, iterativamente, riceve messaggi ed invia risposte.

*/

#include "Header.h"



/**
@brief Utility 
@param name - nome del programma
@return nulla
*/
void usage(char *name){
	printf("Usage: %s <domain>\n", name);
	printf("\tdomain=0 => AF_UNSPEC domain\n");
	printf("\tdomain=4 => AF_INET domain\n");
	printf("\tdomain=6 => AF_INET6 domain\n");
}



int main(int argc, char *argv[]) {
	
	if (argc!= 2){
		usage(argv[0]);
		return INVALID;
	}

	int status = 0; // valore di ritorno delle Sockets API

	//parametri in input al resolver 
	struct addrinfo hints, *result, *ptr; 
	memset(&hints, 0, sizeof(hints)); //azzeramento della struttura

	//specifico il dominio di comunicazione (e di indirizzi richiesti)
	switch ( atoi(argv[1]) ){
		case 0:	
			hints.ai_family = AF_UNSPEC;
			break;
		case 4: 
			hints.ai_family = AF_INET;
			break; 
		case 6: 
			hints.ai_family = AF_INET6;
			break;
	}//switch

	//server side resolution
	hints.ai_flags = AI_PASSIVE; 
	//servizio datagram per il socket 
	hints.ai_socktype = SOCK_DGRAM; 
	//non passo un service name ma un valore di porta UDP
	hints.ai_flags |= AI_NUMERICSERV; 

	status = getaddrinfo(NULL, SERVICEPORT, &hints, &result);
	if (status != 0) 
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return FAILURE;
	}

	int sockfd = 0; /* socket descriptor per il server UDP */
	//predisponiamo il socket UDP
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) 
	{
		//apro un socket 
		//con i parametri dell'elemento corrente della lista di risultati
		sockfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (sockfd == -1)
			continue;//passo al prossimo elemento della lista

		if (bind(sockfd, ptr->ai_addr, ptr->ai_addrlen) == 0)
			break; // Success
		
		/*
		altrimenti chiudo il socket per cui il bind è fallito
		e passo al prossimo elemento della lista
		*/
		close(sockfd);
	}//for

	if (ptr == NULL) 
	{ 
		/* No address succeeded */
		fprintf(stderr, "Could not bind\n");
		return FAILURE;
	}

	//dealloco le risorse usate da getaddrinfo
	freeaddrinfo(result); 

	//il server UDP è pronto per ricevere messaggi
	printf("UDP Echo Server waiting messages on port %s ...\n", SERVICEPORT);
	
	
	char msg[BUFSIZE] = ""; /* buffer che conterrà i datagra ricevuti */
	ssize_t numbytes = 0;
	char peerIP[INET6_ADDRSTRLEN] = ""; // stringa per memorizzare IPv4/IPv6 addresses 
	char peerPort[PORT_STRLEN] = ""; //stringa per rapprensentazione porta UDP
	
	// struttura generica per gestire sia IPv4 che IPv6 
	struct sockaddr_storage peerAddr; 
	// dimensione della struttura di indirizzo generica 
	socklen_t len = sizeof(struct sockaddr_storage); 
	
	
	int quit = 0; /* condizione di iterazione del server */
	
	//no reverse lookup in getnameinfo
	int niflags = NI_NUMERICSERV | NI_NUMERICHOST;
	
	//ciclo (infinito) basato sul valore di quit - condizione di uscita
	while(!quit)
	{
		/*
		chiamata bloccante:
		ritorna quando c'è almeno un datagram da elaborare, o in caso di errore sul socket.
		*/
		numbytes = recvfrom(sockfd, msg, BUFSIZE-1, 0, (struct sockaddr *)(&peerAddr), &len);
		
		if (numbytes == -1)
		{
			perror("UDP Server recvfrom error: ");
			quit = 1;
			//return FAILURE;
		}
		
		//visualizzo l'indirizzo del peer che mi ha contattato:
		status = getnameinfo((struct sockaddr *)(&peerAddr), len, peerIP, INET6_ADDRSTRLEN, peerPort, PORT_STRLEN, niflags);
		
		if (status == 0)
		{
			printf("***** Received message from  '%s:%s' ", peerIP, peerPort);
		}
		else
		{
			printf("getnameinfo() error: %s\n", gai_strerror(status));
		}
		
		//null-terminated string 
		msg[numbytes] = '\0';
		printf("Message (len = %d): '%s'\n", (int)numbytes, msg);
		
		//elaborazione della risposta
		msg[0] = toupper(msg[0]);
		msg[numbytes-1]=toupper(msg[numbytes-1]);


		//invio del datagram contenente la risposta per il client
		numbytes = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)(&peerAddr), len);
		if (numbytes == -1)
		{
			perror("UDP Server sendto error: ");
			quit = 1;
			//return FAILURE;
		}	
	}//wend
		
	//quando quit=1, esco dal ciclo While e dealloco le risorse
	close(sockfd);
	
return 0;
}

/** @} */