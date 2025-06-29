/**
@addtogroup Group9
@{
*/
/**
@file 	TCPClient.c
@author Catiuscia Melle

@brief 	Presentazione di un TCP Client con hostname resolution.

Il client si collega al server, gli invia un messaggio ed ottiene una risposta.
*/

#include "Header.h" 

/**
@brief utility
@param name - nome dell'eseguibile
@return nulla

Illustra come lanciare il programma
*/
void usage(char *name){
	printf("Usage: %s <servername> <domain> <message>\n", name);
	printf("\tdomain=0, AF_UNSPEC\n");
	printf("\tdomain=4, AF_INET\n");
	printf("\tdomain=6, AF_INET6\n");
}

/**
@brief Funzione che visualizza tutti gli indirizzi ritornati dal resolver
@param res - ptr alla linked list ritornata da getaddrinfo()
@return nulla

La procedura illustra come leggere le strutture di indirizzo in una modalità ancora
protocol dependent. 
Per ottenere lo stesso risultato in protocol-indipendent mode, usare getnameinfo().
*/
void readAddrInfoResults(struct addrinfo *res){
	printf("******************** Elenco degli indirizzi risolti\n");
	int result = 0;
	struct addrinfo *p; //ptr addizionale

	char ip4[INET_ADDRSTRLEN]; //stringa IPv4 address
	char ip6[INET6_ADDRSTRLEN]; //stringa IPv6 address

	for (p = res; p != NULL; p=p->ai_next)
	{
		if (p->ai_family == AF_INET)
		{
			//cast a sockaddr_in
			struct sockaddr_in *addr4 = (struct sockaddr_in *)p->ai_addr;
			//conversione IPv4 in stringa
			inet_ntop(AF_INET, &(addr4->sin_addr), ip4, INET_ADDRSTRLEN);
			printf("\tIPv4 address: %s:%d\n", ip4, ntohs(addr4->sin_port)); 
		} 
		else if (p->ai_family == AF_INET6) {
			//cast a sockaddr_in6
			struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)p->ai_addr;	
			//conversione IPv6 in stringa
			inet_ntop(AF_INET6, &(addr6->sin6_addr), ip6, INET6_ADDRSTRLEN);
			printf("\tIPv6 address: %s:%d ", ip6, ntohs(addr6->sin6_port)); 
			
			result = IN6_IS_ADDR_V4MAPPED(&(addr6->sin6_addr));
			if (result != 0){
				printf("[IPv4-mapped-IPv6 address]\n");
			}else{
				printf("[IPv6 address]\n");
			}//fi
			
			
		}//fi ip6
		//visualizzazione degli altri campi:	
		printf("socktype = %d ", p->ai_socktype); 
		printf("protocol = %d ", p->ai_protocol);
		printf("canonname = '%s'\n", p->ai_canonname); //canonical-name
	}//for
	printf("********************\n");
}

/** 
@brief Funzione che recupera il socket pair di un socket connesso
@param sockfd il socket descriptor del socket connesso
@return nulla

La funzione illustra l'uso di getnameinfo() per scrivere codice protocol-independent
Se ho specificato come dominio di comunicazione AF_UNSPEC, non so a priori quale indirizzo mi sia stato ritornato né con quale sia riuscito a connettermi (IPv4/IPv6)
*/
void printSocketPair(int sockfd){
	
	int res = 0;
	struct sockaddr_storage local;
	struct sockaddr_storage remote;
	socklen_t locallen = sizeof(struct sockaddr_storage);
	socklen_t remotelen = sizeof(struct sockaddr_storage);
	
	res = getsockname(sockfd, (struct sockaddr *)&local, &locallen);
	if (res == -1 ){
		perror("getsockname error:");
	}
	
	res = getpeername(sockfd, (struct sockaddr *)&remote, &remotelen);
	if (res == -1 ){
		perror("getpeername error:");
	}

	char localIP[INET6_ADDRSTRLEN] = ""; //stringa di indirizzo IP del client
	char remoteIP[INET6_ADDRSTRLEN] = ""; //stringa di indirizzo IP del server
	char plocal[PORT_STRLEN] = ""; //stringa di porta TCP del client
	char premote[PORT_STRLEN] = ""; //stringa di porta TCP del server

	//no reverse lookup
	int niflags = NI_NUMERICSERV | NI_NUMERICHOST;
	
	res = getnameinfo((struct sockaddr *)(&local), locallen, localIP, INET6_ADDRSTRLEN, plocal, PORT_STRLEN, niflags);
	
	if (res == 0)
	{
		printf("***** TCP Connection from '%s:%s' ", localIP, plocal);
	}
	else
	{
		printf("getnameinfo() error: %s\n", gai_strerror(res));
	}
	
	res = getnameinfo((struct sockaddr *)(&remote), remotelen, remoteIP, INET6_ADDRSTRLEN, premote, PORT_STRLEN, niflags);
	
	if (res == 0)
	{
		printf("to '%s:%s' ", remoteIP, premote);
	}
	else
	{
		printf("getnameinfo() error: %s\n", gai_strerror(res));
	}
printf("*****\n\n");
}


int main(int argc, char *argv[]){

	if (argc != 4) {
		usage(argv[0]);
		return INVALID;
	}
	
	int domain = atoi(argv[2]);
	
	//prepariamo l'invocazione del resolver
	struct addrinfo hints, *result, *ptr;
	int status = 0;
	
    
	memset(&hints, 0, sizeof(hints)); //azzeramento della struttura
	
	//Definiamo il dominio di comunicazione
	switch (domain){
		case 0:
		//il resolver richiede A e AAAA RR al DNS
			hints.ai_family = AF_UNSPEC;
			break;
		case 4: 
		//solo A records => sockaddr_in addresses
			hints.ai_family = AF_INET;
			break;
		case 6: 
		//solo AAAA records => sockaddr_in6 addresses
			hints.ai_family = AF_INET6;
			//Specificando il flag AI_V4MAPPED, assieme ad ai_family = AF_INET6,
			//se il resolver non trova AAAA restituisce indirizzi IPv4-mapped-IPv6 
			hints.ai_flags |= AI_V4MAPPED;
			break;
	}//switch
	
	//tipo di servizio richiesto al socket 
	hints.ai_socktype = SOCK_STREAM; 
	
	//non necessario specificare il protocollo TCP
	hints.ai_protocol = IPPROTO_TCP; 

	//non necessaria la risoluzione del service name
	hints.ai_flags |= AI_NUMERICSERV;
	//richiediamo il nome canonico
	hints.ai_flags |= AI_CANONNAME; 
	
	//invoco il resolver ottenere gli indirizzi IP corrispondenti al nome inserito 
	status = getaddrinfo(argv[1], SERVICEPORT, &hints, &result);
	if (status != 0) 
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		return FAILURE;
	}//fi

	//leggiamo gli indirizzi ritornati, senza getnameinfo()
	readAddrInfoResults(result);

	//prepariamo la Active Open
	int sockfd = 0;
	
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) 
	{
		sockfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol); 
		if (sockfd == -1)
			continue; //passo al prossimo risultato
		
		//avvio il 3WHS
		if (connect(sockfd, ptr->ai_addr, ptr->ai_addrlen) == -1)
		{
			//se fallisce, devo chiudere il socket e aprirne un altro 
			close(sockfd);
			printf("client connecting in the %s domain error\n", (ptr->ai_family == AF_INET) ? "AF_INET": "AF_INET6");
			perror("connect error:");
			continue;
		}//fi connect

	//se sono qui, sono connesso 
	break;
	}//for

	if (ptr == NULL) 
	{
		//nessun address è stato valido per la connessione
		fprintf(stderr, "Client failed to connect\n");
		return FAILURE;
	}

	//se sono qui, sono connesso al server
	//la linked list non occorre più
	freeaddrinfo(result);
	
	//visualizzo i dati della connessione
	printSocketPair(sockfd); 
	
	//iniziamo lo scambio di messaggi
	char buf[BUFSIZE] = "";
	ssize_t numbytes = 0;
	ssize_t n = 0;
	
	//int quit = 0;
	//while(!quit)
	//{
		//invio un messaggio al server
		numbytes = send(sockfd, argv[3], strlen(argv[3]), 0);
		numbytes = send(sockfd, argv[3], strlen(argv[3]), 0);
		if (numbytes == -1)
		{
			perror("send(): ");
			close(sockfd);
			return FAILURE;
		}
		else
		{
			printf("Sent %d bytes message\n", (int) numbytes);
		}
		
		n = numbytes;
		size_t bufferLen = BUFSIZE/2;
		while (n > 0)
		{
			numbytes = recv(sockfd, buf, bufferLen, 0);
			if (numbytes > 0)
			{
				buf[numbytes] = '\0';
				printf("Received echo message: '%s'\n", buf);
				n -= numbytes;
			}
		}//wend
		printf("received all bytes previously sent\n");
	//}//wend
	
	//chiudiamo la connessione
	close(sockfd);
	
return 0;
}

/** @} */