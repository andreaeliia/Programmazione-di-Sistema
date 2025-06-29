/**
@defgroup Group9 TCPResolution
@brief TCP Echo client-server <em>with hostname resolution</em>
@{
*/
/**
@file 	TCPConcurrentS.c
@author Catiuscia Melle

@brief 	Esempio TCP echo server concorrente con utilizzo di <em>getaddrinfo()<em>.

Esempio base di TCP server concorrente basato su <em>fork()</em>: 
		il processo padre, all'arrivo di una nuova connessione, 
		crea un child cui affida la gestione della richiesta. 
L'esempio illustre l'utilizzo delle funzioni <em>getaddrinfo()<em> e <em>getnameinfo()<em> per la scrittura di codice protocol-indipendent.
*/

#include "Header.h"
#include <errno.h>


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

/**
@brief Gestione dei child che terminano l'elaborazione della richiesta...
*/
void sigchld_handler(int s){
	
	while(waitpid(-1, NULL, WNOHANG) > 0);
}


/**
@brief Elaborazione effettuata dai child per l'elaborazione di una richiesta 
@param clientfd - socket descriptor relativo alla connessione stabilita con un client
@return - nulla
*/
void handleClientConnection(int clientfd){
	int res = 0;
	
	//struttura generica per IPv4 ed IPv6 - protocol independent code
	struct sockaddr_storage client; 
	//dimensione della struttura di indirizzo
	socklen_t sslen = sizeof(struct sockaddr_storage);
	
	
	//getpeername per estrarre il remote address dal socket connesso
	res = getpeername(clientfd, (struct sockaddr *)&client, &sslen);
	if (res == -1)
	{
		perror("getpeername error: ");
		exit(1);
	}
	
	char ip[INET6_ADDRSTRLEN] = "";//stringa di indirizzo IP del client
	char port[PORT_STRLEN] = "";//stringa di porta TCP del client
	
	int niflags = NI_NUMERICSERV | NI_NUMERICHOST;
	
	//getnameinfo per ottenere la rappresentazione testuale dell'indirizzo del client
	res = getnameinfo((struct sockaddr *)(&client), sslen, ip, INET6_ADDRSTRLEN, port, PORT_STRLEN, niflags);
	
	if (res == 0)
	{
		printf("\n\tConnection with client from '%s:%s' ", ip, port);
	}
	else
	{
		printf("getnameinfo() error: %s\n", gai_strerror(res));
	}
	
	//e per verificare se è un IPv4-mapped-IPv6 o meno:
	if (client.ss_family == AF_INET6)
	{
		struct sockaddr_in6 *addr = (struct sockaddr_in6 *)&client;
		/*
			la macro IN6_IS_ADDR_V4MAPPED, preso in input un puntatore
			ad una struct in6_addr, ritorna
			- !=0 se l'indirizzo IPv6 è del tipo specificato
		*/
		res = IN6_IS_ADDR_V4MAPPED( &(addr->sin6_addr));
		if (res != 0){
			printf("[IPv4-mapped-IPv6 address] - ");
		} else {
			printf("[IPv6 address] - ");
		}
		
		/*
		la macro IN6_IS_ADDR_LOOPBACK, preso in input un puntatore
		ad una struct in6_addr, ritorna
		- 0 se l'indirizzo è IPv6
		- !=0 se è un indirizzo IPv4-mapped-IPv6
		*/
		res = IN6_IS_ADDR_LOOPBACK( &(addr->sin6_addr));
		if (res != 0){
			printf("[loopback IPv6 address]\n");
		}else{
			printf("[not the IPv6 loopback address]\n");
		}
		
		
	}//fi family
	
	//Alla vecchia maniera, in un codice protocol independent, avrei dovuto fare:
	/*
	if (client.ss_family == AF_INET)
	{	
		char IPv4[INET_ADDRSTRLEN] = "";
		struct sockaddr_in *ip4 = (struct sockaddr_in *)&client;
		inet_ntop(AF_INET, &(ip4->sin_addr), IPv4, INET_ADDRSTRLEN);
		printf("Remote Address = %s:%d\n", IPv4, ntohs(ip->sin_port));
	}
	else if (client.ss_family == AF_INET6)
	{
		char IPv6[INET6_ADDRSTRLEN] = "";
		struct sockaddr_in6 *ip6 = (struct sockaddr_in6 *)&client;
		inet_ntop(AF_INET6, &(ip6->sin6_addr), IPv6, INET6_ADDRSTRLEN);
		printf("Remote Address = %s:%d\n", IPv6, ntohs(ip6->sin6_port)); 
	}
	*/


	//prelevo i messaggi del client
	char msg[BUFSIZE] = "";
	ssize_t num = 0;
	
	int quit = 0;
	
	while (!quit)
	{
		
		num = recv(clientfd, &msg, BUFSIZE - 1, 0); 
		
		if (num == -1)
		{
			perror("Recv error: ");
			close(clientfd);
			exit(1);
		}

		if (num == 0)
		{
			printf("client closed the connection\n");
			quit = 1;
			
		} else {
			msg[num] = '\0';
			printf("\tReceived %d bytes msg: '%s'\n", (int)num, msg);
			
			num = send(clientfd, msg, strlen(msg), 0);
			(num == -1 ) ? perror("Send failed: ") : printf("sent %d bytes\n", (int) num);
		}//fi
	}//wend
	
	//il client ha chiuso la connessione, dealloco il socket
	close(clientfd);
}



int main(int argc, char *argv[]){

	if (argc != 2) {
		usage(argv[0]);
		return INVALID;
	}
	int domain = atoi(argv[1]);
// aggiunta per leggere sockaddr
	struct sockaddr_in *temp;
	temp = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
// fine parte aggiunta per leggere sockaddr
	
	printf("TCP Server app with hostname resolution in the AF_INET%s domain\n", argv[1]);
	
	//signal handler per la gestione dei child
	struct sigaction sa;
	sa.sa_handler = sigchld_handler; 
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    
    if (sigaction(SIGCHLD, &sa, NULL) == -1) 
    {
    	perror("sigaction error: ");
		exit(1); 
	}
	
	
	int sockfd = 0; //listening socket 
	int ecode = 0; //per valore di ritorno delle system call invocate
	
	//hostname resolution: definisco gli hints
	struct addrinfo hints, *result, *p;
	
	//azzero hints
	memset(&hints, 0, sizeof(hints)); 
	
	//specifico il dominio di comunicazione (e di indirizzi richiesti)
	switch (domain){
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
		
	hints.ai_socktype = SOCK_STREAM; //TCP
	//hints.ai_protocol = 0; //non necessario, abbiamo già azzerato la struttura 
	
	//Definisco i restanti flags: Passive Open
	hints.ai_flags |= AI_PASSIVE; 
	//in OR Logico per disabilitare 'service name resolution' 
	// porta TCP già stringa numerica
	hints.ai_flags |= AI_NUMERICSERV;
	
	//invoco il resolver
	ecode = getaddrinfo(NULL, SERVICEPORT, &hints, &result);
	if (ecode != 0)
	{
		printf("getaddrinfo: %s\n", gai_strerror(ecode));
		return ecode;
	}
// aggiunta per leggere sockaddr
	errno=0;
	memcpy(temp, result->ai_addr, sizeof(struct sockaddr_in));
	perror(argv[0]);
// fine parte aggiunta per leggere sockaddr
	
	//loop through the linked list until success
	for (p = result; p != NULL; p = p->ai_next)
	{
		//crea il socket
		sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sockfd == -1)
		{	
			perror("socket() error: ");
			continue;	//skip to next
		}
		
		printf("open a socket in the %s domain\n", (p->ai_family == AF_INET) ? "AF_INET": "AF_INET6");
		
		//assegna l'indirizzo risolto
		ecode = bind(sockfd, p->ai_addr, p->ai_addrlen);
		if (ecode == -1)
		{
			perror("bind() error: ");
			close(sockfd);
			continue; //skip to next
		}
			
		break; //success
	}//for
	
	
	if (p == NULL) 
	{
		printf("Abbiamo testato tutte le strutture di indirizzo ma non siamo riusciti ad aprire un socket ed assegnargli l'indirizzo desiderato nel dominio AF_INET%s\n", argv[1]);
		return FAILURE;
	}//fi NULL
	
	
	//libero le risorse di getaddrinfo(): non mi servono più
	freeaddrinfo(result);
	
	//completo la passive open
	ecode = listen(sockfd, BACKLOG);
	if (ecode == -1)
	{
		perror("listen() error: ");
		close(sockfd);
		return FAILURE;
	}
	
	
	//le informazioni sul local address le ottengo con getsockname:
	struct sockaddr_storage local;
	socklen_t len = sizeof(local);
	ecode = getsockname(sockfd, (struct sockaddr *)&local, &len);
	if (ecode == -1 ){
		perror("getsockname on listening socket ERROR:");
		return FAILURE;
	}
	
	//Passive Open completata con successo, possiamo servire i client 
	printf("\tTCP Server listening on port %s, domain %s\n\n", SERVICEPORT, (local.ss_family == AF_INET6) ? "IPv6": "IPv4");
	
	
	//per il connected socket
	int clientfd = 0;

	//indefinitamente
	while (1)
	{

		//prelevo connessione dalla coda delle connessioni completate
		//non mi interessa l'indirizzo del client 
		clientfd = accept(sockfd, NULL, NULL);
		if (clientfd == -1)
		{
			perror("accept() error: ");
			continue; 
		}

		//creo un child per ogni connessione 
		if ( fork() == 0)
		{
			//i child elaborano le richieste dei client
			//chiudo il listening socket
			close(sockfd);
			//gestisco il client connesso
			handleClientConnection(clientfd);
			printf("\n\tChild %d terminating..\n", (int)getpid());
			exit(0);
		}

		//al parent non interessa gestire il client
		close(clientfd);
	}//wend

	//never reached
	close(sockfd);

return 0;
}

/** @} */
