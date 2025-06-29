/** 
@addtogroup Group12 
@{
*/
/**
@file 	linger-client.c
@author Catiuscia Melle

@brief Chiusura di una connessione TCP

Questo esempio illustra il 4-way-teardown di una connessione TCP,
facendo uso dell'opzione SO_LINGER sul socket connesso per 
modificare la fase di chiusura della connessione TCP col server.

L'applicazione server da utilizzare può essere il server TCP concorrente del folder 09-ResolveTCP.
*/

#include "Header.h"


int main(int argc, char *argv[]){

	if ( argc != 4)
	{
		printf("Usage: %s <servername> <l_onoff> <l_linger> (seconds)\n", argv[0]);
		return FAILURE;
	}
	
	
	int status = 0;
	
	
    struct addrinfo hints, *result, *ptr;
    
    memset(&hints, 0, sizeof(hints)); //azzeramento della struttura
    hints.ai_family = AF_UNSPEC; //l'applicazione accetta sia indirizzi IPv4 che IPv6
    hints.ai_socktype = SOCK_STREAM; //tipo di socket desiderato 
    hints.ai_flags |= AI_NUMERICSERV; //no service name resolution
   
	status = getaddrinfo(argv[1], SERVICEPORT, &hints, &result);
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

		//imposto l'opzione SO_LINGER sul socket TCP		
		struct linger myLinger;
		myLinger.l_onoff = atoi(argv[2]);
		myLinger.l_linger = atoi(argv[3]);
		
		status = setsockopt(sockfd, SOL_SOCKET, SO_LINGER, &myLinger, sizeof(myLinger)); 
		if (status < 0)
		{
			perror("setsockopt linger option error: ");
			close(sockfd);
			//return FAILURE;
			continue;
		}
		printf("\tSO_LINGER option %s with a time of %d seconds\n", (myLinger.l_onoff == 1)?"on":"off", myLinger.l_linger); 
    
		//avvio 3-way handshake
        if (connect(sockfd, ptr->ai_addr, ptr->ai_addrlen) == -1)
        {
        	perror("client connect error: ");
        	close(sockfd);
        	continue;
    	}
    	
    	break;
    } //for


    if (ptr == NULL) 
    {         
    	/* No address succeeded */
        fprintf(stderr, "Client failed to connect\n");
        return FAILURE;
    }


	char remoteIP[INET6_ADDRSTRLEN] = ""; //stringa di indirizzo IP del server
	char premote[PORT_STRLEN] = ""; //stringa di porta TCP del server

	//no reverse lookup
	int niflags = NI_NUMERICSERV | NI_NUMERICHOST;
	
	status = getnameinfo(ptr->ai_addr, ptr->ai_addrlen, remoteIP, INET6_ADDRSTRLEN, premote, PORT_STRLEN, niflags);
	
	if (status == 0)
	{
		printf("***** TCP Connection established to '%s:%s' ", remoteIP, premote);
	}
	else
	{
		printf("getnameinfo() error: %s\n", gai_strerror(status));
	}
	
	freeaddrinfo(result);     /* No longer needed */
	
	    
    char buf[BUFSIZE] = "";
	char msg[BUFSIZE] = "";
	ssize_t numbytes = 0;
	
	/*
	il client invia una sequenza di messaggi
	lo scopo è inviare molti dati velocemente e mostrare che la receiving app non è in grado di 
	elaborarli tutti.
	Infatti, a seconda che l'opzione SO_LINGER sia on o off, non è garantito che tutti i dati 
	saranno trasmessi e riscontrati.
	Ad esempio, se SO_LINGER on con 0 secondi, il RST viene generato e parte dei dati andranno persi.
	(dopo la ricezione del RST, il remote peer chiuderà il socket a sua volta).
	*/
	int i = 0;
	while ( i < 300)
	{
		snprintf(msg, BUFSIZE-1, "***Linger Client - Hello %d---", ++i);
		numbytes = send(sockfd, msg, strlen(msg), 0);
		
		if (numbytes == -1)
		{
			perror("send() error: ");
			close(sockfd);
			return FAILURE;
		}
		else
		{
			printf("Sent message %d\n", i);
		}
	
	}//wend
	
	
	/*
	Ora il comportamento di close() è modificato dalla presenza dell'opzione e dai valori 
	che assumono i suoi campi.
	*/
	status = close(sockfd);
	if (status == -1) {
		perror("close error: ");
	} else {
		printf("Socket closed without error.\nRemote Peer acknowledged data and FIN\n");
	}
	
return 0;
}

/** @} */
