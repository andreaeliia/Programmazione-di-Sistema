/**
@defgroup Group31 Non-blocking connect()
@brief Non-blocking connect()

Implementazione di una connect() in modalità non bloccante.
@{
*/
/**
@file 	TCPC-nonblock.c
@author Catiuscia Melle

@brief 	Implementazione di un TCP echo client in modalità non bloccante, che invia 5 
		messaggi al server. 

Esempio di non-blocking I/O: TCP connect().
Utilizzo della modalità non bloccante dei socket per l'esecuzione di una connect() 
verso un server.
L'esempio illutra le operazioni da compiere per effettuare un tentativo di connessione 
ad un server TCP, prevenendo il blocco dell'applicazione in caso di ritardo della risposta 
da parte del server.
L'esempio metta in luce l'avvio di un 3WH.
Inoltre, l'utilizzo di un timeout per le operazioni di I/O sul socket consentono al client 
di bloccare la recv() se il server non risponde entro un tempo massimo fissato. 
*/

#include "Utility.h"


/**
@brief Esecuzione di una connect con socket non bloccante.
@param sockfd - socket da cui eseguire la connect
@param dest - destinazione verso cui connettersi
@param len - dimensione dell'indirizzo del socket
@param timeout, intero pari al massimo numero di secondi che si desidera attendere per 
la conclusione del procedimento.

@return intero, stato della connessione.
*/
int nonblock_connect(int sockfd, struct sockaddr *dest, socklen_t len, int timeout){

	int result = 0; //return value
	
	//step 1: settiamo il socket in non-blocking mode
	result = set_nonblock(sockfd);
	if (result == FAILURE)
	{
		printf("Error on non-blocking socket descriptor\n");
		return FAILURE;
	}
	
	//step 2: eseguiamo la connect() che ritornerà subito
	//in genere con errno==EINPROGRESS 
	result = connect(sockfd, dest, len);
	
	//ritorno immediato
	if (result == 0)
	{
		printf("Connected with the destination\n");
		
		//step 2.1: rimettiamo il socket in blocking-mode
		if (set_block(sockfd) != SUCCESS)
		{
			return FAILURE;
		}
		else
		{
			return SUCCESS;
		}
	}//fi 
	
	//errore diverso da quello atteso
	if ( (result < 0) && (errno != EINPROGRESS) )
	{
		perror("Error on connect(): ");
		close(sockfd);
		return FAILURE;
	} 
	
	//EINPROGRESS
	fd_set rset, wset;		
	FD_ZERO(&rset);
	FD_ZERO(&wset);
		
	struct timeval tv;
	tv.tv_sec = timeout; //tempo massimo di attesa application-dependent, 1 RTT verso il server
	tv.tv_usec = 0;
			
	FD_SET(sockfd, &rset);
	FD_SET(sockfd, &wset);
	
	//step 3: attendiamo il tempo massimo 
	result = select(sockfd+1, &rset, &wset, NULL, &tv);
	
	if (result == 0)
	{
		//timeout with no changes
		close(sockfd); //stop 3-WHS
		errno = ETIMEDOUT;
		return FAILURE;	
	} 
	
	int error = 0; //conterrà il codice di errore sul socket se connect fallisce, 0 altrimenti
	socklen_t optlen = sizeof(error);
	
	
	//altrimenti verifichiamo se il socket è ready: in lettura OR in scrittura
	if ( FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset) )
	{
		/*  Se la connessione è andata a buon fine il socket descriptor sarà pronto in
		scrittura; in caso di errore, sarà sia readable che writable e potremo leggere 
		il suo pending error
		
		Verifico se ci sono errori pendenti sul socket
		Portabilità: 
		I) BSD-derived implementations ritornano l'errore pendente sul socket 
		in error e getsockopt ritorna 0.
		II) Solaris implementation fa ritornare -1 a getsockopt, con errno pari 
		all'errore pendente sul socket
		*/	
		if ( getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &optlen) < 0)
		{
			perror("getsockopt() error: (Solaris pending error on socket:) ");
			error=errno;
			//close(sockfd);
			//return FAILURE;
		}
		
	} 
	else 
	{	
		/* 
		se siamo qui, significa che 
		select ha ritornato un errore 
		*/
		perror("select() error: ");
		close(sockfd);
		return FAILURE;
	} 
	
	/* 
	ora possiamo controllare il valore dell'errore 
	pendente sul socket 
	*/	
	if (error != 0)
	{
		//c'è errore, connessione fallita
	 	close(sockfd);
		errno = error;
		perror("connection error: ");
		return FAILURE;
	}
	
	//altrimenti rimettiamo il socket in blocking mode, connessione andata a buon fine
	set_block(sockfd);
				
	return SUCCESS;
}


int main(int argc, char *argv[]){
	
	if (argc != 3) 
	{
		printf("Usage: %s <ServerName> <Message>\n", argv[0]);
		return FAILURE;
	} 
	
	int status = 0;
	int sockfd = 0;
	
	struct addrinfo hints, *result, *ptr;
    
    memset(&hints, 0, sizeof(hints)); //azzeramento della struttura
    hints.ai_family = AF_UNSPEC; //l'applicazione accetta sia indirizzi IPv4 che IPv6
    hints.ai_socktype = SOCK_STREAM; //tipo di socket desiderato 
   
    status = getaddrinfo(argv[1], SRV_PORT, &hints, &result);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return FAILURE;
    }
	
  	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) 
    {
        sockfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (sockfd == -1)
            continue;

        /*
        if (connect(sockfd, ptr->ai_addr, ptr->ai_addrlen) == -1)
        {  	
        	perror("client: connect()");
        	close(sockfd);
        	continue;
    	}
    	*/
    	
    	status = nonblock_connect(sockfd, ptr->ai_addr, ptr->ai_addrlen, 3);
    	if (status != SUCCESS)
    	{
    		perror("Connection not successful: ");
    		continue;
    	}
    	break;
    }


    if (ptr == NULL) 
    {         
    	/* No address succeeded */
        fprintf(stderr, "Client failed to connect\n");
        return FAILURE;
    }
    /*
	Per verificare se effettivamente il socket è connesso, tenendo in conto i problemi di 
	portabilità, possiamo scegliere tra queste alternative (che sostituirebbero quella 
	già vista del controllo dell'errore pendente con getsockopt):
	*/

	//opzione 1: getpeername
	struct sockaddr_storage dest;
	socklen_t len = sizeof(dest);
	
	status = getpeername(sockfd, (struct sockaddr *)&dest, &len);
	if ((status == -1) && (errno == ENOTCONN))
	{
		printf("connection was not successful\n");
		return FAILURE;
	}//fi 1

	//opzione 2: read 0 bytes
	char buf[BUFSIZE] = "";
    ssize_t numbytes = 0;
    
	status = read(sockfd, buf, 0);
	if ((status == -1) && (errno == ENOTCONN))
	{
		printf("connection was not successful\n");
		return FAILURE;
	}//fi 2

	//opzione 3: connect() again (blocking?)
	status = connect(sockfd, ptr->ai_addr, ptr->ai_addrlen);
	if ((status == -1) && (errno == EISCONN))
	{
		printf("Already connected: connection was successful\n");
	}//fi 3

	/*
	settiamo un timeout sul socket connesso
	*/

	struct timeval timeout;
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	status = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval));
	if (status != 0)
	{
		perror("setsockopt SO_RCVTIMEO error: ");
		close(sockfd);
		return FAILURE;
	}

	
	char IPv4[INET_ADDRSTRLEN] = "";
	char IPv6[INET6_ADDRSTRLEN] = "";
	
	if (ptr->ai_addr->sa_family == AF_INET)
	{
		struct sockaddr_in *ip = (struct sockaddr_in *)ptr->ai_addr;
		inet_ntop(AF_INET, &(ip->sin_addr), IPv4, INET_ADDRSTRLEN);
		printf("Client connected to: %s\n", IPv4);
	}
	else if (ptr->ai_addr->sa_family == AF_INET6)
	{
			struct sockaddr_in6 *ip = (struct sockaddr_in6 *)ptr->ai_addr;
			inet_ntop(AF_INET6, &(ip->sin6_addr), IPv6, INET6_ADDRSTRLEN);
			printf("Client connected to: %s\n", IPv6);
	}
	
	/* No longer needed */
	freeaddrinfo(result);   
	  
	/*
	ora avviamo la comunicazione con il server
	*/
	int i = 1;
	int quit = 0;
	
	while (!quit)
	{
		numbytes = send(sockfd, argv[2], strlen(argv[2]), 0);
	
		if (numbytes == -1)
		{
			perror("send() :");
			close(sockfd);
			return FAILURE;
		}
		else
		{
			printf("sent message %d\n", i);
			i++;
		}
	
		numbytes = recv(sockfd, buf, BUFSIZE-1,0);
		if (numbytes == 0)
		{
			printf("server closed connection\n");
			quit = 1;
		}
		if (numbytes > 0)
		{
			buf[numbytes] = 0;
			printf("Client received '%s' from server\n", buf);
			sleep(1);
			if (i==5) quit = 1;
		}
		
		if ((numbytes == -1) && (errno == EWOULDBLOCK))
		{
			printf("timeout! server not responding\n");
			quit = 1;
		}
		
	}//wend
	close(sockfd);
	
return 0;
}

/**@}*/