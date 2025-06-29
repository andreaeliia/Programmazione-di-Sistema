/**
@defgroup Group5 TCP/IPv4 fake Client and Server

@brief TCP/IPv4 fake client and server to analyze TCP implementation

L'esempio vuole illustrare cosa accade nelle applicazioni client e server, interponendo opportuni blocchi.
In particolare, l'esecuzione del server su Linux e su Mach consente di confrontare le diverse implementazioni TCP sulle due piattaforme: 
- Linux risponde con un SYN a richieste di connessioni per socket non in LISTEN state
- Mac OS (su Mach3) scarta il SYN, generando la ritrasmissione dei SYN del client. 

Per approfondimento, vedere il file markdown.

@{
*/
/**
@file 	FakeServer.c
@author Catiuscia Melle

@brief 	Fake TCP Echo Server IPv4.

A seconda del valore intero inserito, il server:
- si blocca prima di aprire il socket;
- si blocca prima di effettuare il bind dell'indirizzo al socket;
- si blocca prima di effettuare la passive open;
- non esegue mai accept;
*/

#include "Header.h"

/**
@brief Mostra come usare il programma
@param exename, coincide con argv[0], nome dell'eseguibile
@return nulla
*/
void usage(char *exename){
	
	printf("\tUsage: %s <num>\nwhere num is either:\n", exename);
	printf("\t1 - server never opens the socket\n");
	printf("\t2 - server opens the socket but never binds\n");
	printf("\t3 - server binds the socket to its local address, but never listens\n");
	printf("\t4 - server executes the passive open, but never call accept\n");
	printf("\t5 - server executes normally\n");
	
}


/**
@brief definizione del local address IPv4 del server
@param addr, ptr alla struct sockaddr_in da inizializzare
*/
void initLocalAddress(struct sockaddr_in *addr){
	
	memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family= AF_INET;	//IPv4 family
	
	/*
	IPv4 wildcard address: qualsiasi IP
	Notare che l'indirizzo deve essere assegnato al campo s_addr in Network Byte Order
	*/
	addr->sin_addr.s_addr = htonl(INADDR_ANY);
	
	/*
	TCP port su cui il server si metterà in ascolto, in Network Byte Order
	*/
	addr->sin_port = htons(PORT);
}




int main(int argc, char *argv[]){
	
	if (argc != 2) {
		usage(argv[0]);
		return 0;
	}
	printf("\n\tFake TCP4 server\n");
	int stepAt = atoi(argv[1]);
	
	int sockfd = 0; /* listening socket del server TCP */
	int peerfd = 0; /* connected socket del client TCP */
	int ret = 0; 	/* valore di ritorno delle Sockets API */
	struct sockaddr_in addr; /* server local address, for bind() */
	

	if (stepAt == 1){
		printf("Server never open socket\n");
		while(1){ sleep(3); }
	}
	
	//step 1: open listening socket per IPv4
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		perror("socket() error: ");
		return FAILURE;
	}
	
	
	if (stepAt == 2){
		printf("Server never binding...\n");
		while(1){ sleep(3); }
	}
	
	/*
	Assegno al socket l'indirizzo IPv4 memorizzato in addr
	*/
	initLocalAddress(&addr);
	
	ret = bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	if (ret == -1)
	{
		perror("bind() error: ");
		close(sockfd);
		return FAILURE;
	}
	
	
	if (stepAt == 3){
		printf("Server never listening...\n");
		while(1){ sleep(3); }
	}
	
	/*
	Passive Open obbligatoria per un server TCP: abilito il TCP layer ad
	accettare connessioni per questo socket
	*/
	ret = listen(sockfd, BACKLOG);
	if (ret == -1)
	{
		perror("listen() error: ");
		close(sockfd);
		return FAILURE;
	}
	
	printf("\n\tServer listening on port %d\n", (int)PORT);
	
	
	if (stepAt == 4){
		printf("I'm a server never accepting...\n");
		while(1){ sleep(3); }
	}
	
	
	/*
	Il server TCP memorizzerà in peer_addr l'indirizzo del client connesso
	*/
	struct sockaddr_in peer_addr;
	socklen_t len = sizeof(peer_addr);
	
	int quit = 0; //regola il loop infinito nel server
	int connected = 0; //regola la gestione della connessione col client
	while (!quit) 
	{
		/*
		chiamata bloccante:
		fino a quando non c'è una connessione completa (3WHS terminato) 
		il server rimane bloccato in accept. 
		Al ritorno, peerfd è il socket connesso della connessione col client - salvo errori.
		*/
		peerfd = accept(sockfd, (struct sockaddr *)&peer_addr, &len);
		if (peerfd == -1)
		{
			perror("accept() error: ");
			close(sockfd);
			return FAILURE;
		}
	
		char clientaddr[INET_ADDRSTRLEN] = "";
		inet_ntop(AF_INET, &(peer_addr.sin_addr), clientaddr, INET_ADDRSTRLEN);
		printf("\tAccepted a new TCP connection from %s:%d\n", clientaddr, ntohs(peer_addr.sin_port));
	
		char buf[BUFSIZE];
		ssize_t n = 0;
		connected = 1;
		
		while (connected) 
		{
			n = recv(peerfd, buf, BUFSIZE-1, 0);
			if (n == -1)
			{
				perror("recv() error: ");
			}
			else if (n==0)
			{
				printf("Peer closed connection\n");
				connected = 0;
			}
			else 
			{
				buf[n] ='\0';
				printf("Received message '%s'\n", buf);
				//reply to client
				n = send(peerfd, buf, strlen(buf), 0);
				if (n == -1) {
					perror("send error:");
				}
			}
		}//wend connected
		close(peerfd);
	}//wend quit
	close(sockfd);
	
return 0;
}

/** @} */


