/**
@addtogroup Group31
@{
*/
/**
@file 	TCPS.c
@author Catiuscia Melle
@brief 	Implementazione di un TCP echo server iterativo, che si può bloccare 
		prima o dopo l'esecuzione della listen().
*/

#include "Utility.h"

/**
@brief Funzione per la lettura dell'indirizzo del client connesso
@param client - ptr alla struttura generica da interpretare
@return nulla
*/
void print_client_addr(struct sockaddr_storage *client){
	
	char IPv6[INET6_ADDRSTRLEN];
	if (client->ss_family == AF_INET)
	{
		struct sockaddr_in *ip = (struct sockaddr_in *)client;
		inet_ntop(AF_INET, &ip->sin_addr, IPv6, INET6_ADDRSTRLEN);
		printf("Client Address: %s:%d\n", IPv6, ntohs(ip->sin_port));
	}
	else if (client->ss_family == AF_INET6)
	{
		struct sockaddr_in6 *ip = (struct sockaddr_in6 *)client;
		inet_ntop(AF_INET6, &ip->sin6_addr, IPv6, INET6_ADDRSTRLEN);
		printf("Client Address: %s:%d\n", IPv6, ntohs(ip->sin6_port));
	}	
}


int main(int argc, char *argv[]){
	
    int status = 0;
	int sockfd = 0; 
	
	struct addrinfo hints, *result, *ptr;
    
    memset(&hints, 0, sizeof(hints)); //azzeramento della struttura
    
    hints.ai_family = AF_UNSPEC; //l'applicazione accetta sia indirizzi IPv4 che IPv6
    hints.ai_flags = AI_PASSIVE; //server side 
    hints.ai_socktype = SOCK_STREAM; //tipo di socket desiderato 
    //hints.ai_protocol = 0; //non necessario, abbiamo già azzerato la struttura
	hints.ai_flags |= AI_NUMERICSERV;
	
    status = getaddrinfo(NULL, SRV_PORT, &hints, &result);
    if (status != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return FAILURE;
    }

    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) 
    {
        sockfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (sockfd == -1)
            continue;

        if (bind(sockfd, ptr->ai_addr, ptr->ai_addrlen) == 0)
            break;            /* Success */

        close(sockfd);
    }//for


    if (ptr == NULL)
    {         /* No address succeeded */
        fprintf(stderr, "Could not bind\n");
        return FAILURE;
    }

	freeaddrinfo(result);     /* No longer needed */
   	printf(" %s ready on port %s\n", argv[0], SRV_PORT);
	
	/**/
	printf("Fake server: never call listen()\n");
	while(1)
	{
		//fake...
	}
	/**/
	
	
	//prepariamo il server TCP
	status = listen(sockfd, BACKLOG);
	if (status < 0)
	{
		perror("server listen error: ");
		return FAILURE;
	}
	
	
	/**/
	printf("Fake server: never call accept()\n");
	while(1)
	{
		//fake...
	}
	/**/
	
	struct sockaddr_storage c_addr;
	socklen_t len = sizeof(struct sockaddr_storage);
	
	char buffer[BUFSIZE]; 
	ssize_t numbytes = 0;
	
	int quit = 0;
    time_t timeval;
	
	char fd_open[FD_SETSIZE]; //array di tutti i socket descriptor monitorati dal server
    fd_set rset;//readable socket descriptor
    
    int fd;
    int max_fd, nread, nwrite;
    int i, n;

	/* initialize all needed variables */
    memset(fd_open, 0, FD_SETSIZE);   /* clear array of open files */
    max_fd = sockfd;                 /* maximum now is listening socket */
    fd_open[max_fd] = 1;//turn-on the bit of sockfd in the array
    
    
    /* main loop, wait for connection and data inside a select */
    while (!quit) 
    {
    
		FD_ZERO(&rset); /* clear fd_set */
		for (i = sockfd; i <= max_fd; i++) 
		{ 
			/* initialize fd_set */
	    	if (fd_open[i] != 0) FD_SET(i, &rset); 
		}
		
		n = select(max_fd + 1, &rset, NULL, NULL, NULL);
		if (n < 0) 
		{                          
	    	perror("select() error: ");
	    	return FAILURE;
		} 
		else if (n > 0)
		{
			printf("Trovati %d socket ready for reading\n", n);
			if (FD_ISSET(sockfd, &rset)) 
			{       
				/* if new connection */
	    		len = sizeof(c_addr);            
	    		
	    		/* call accept */
	    		if ((fd = accept(sockfd, (struct sockaddr *)&c_addr, &len)) < 0) 
	    		{
					perror("accept() error: ");
	    			return FAILURE;
	    		}
	    		printf("Accettata nuova connessione su fd %d\t", fd);
	    		print_client_addr(&c_addr);
	    		
	    		/* decrement active */
	    		printf("Restano %d socket attivi\n", --n);              
	    		
	   			fd_open[fd] = 1;                  /* set new connection socket */
	    		if (max_fd < fd) max_fd = fd;     /* if needed set new maximum */
	    		//printf("max_fd=%d\n", max_fd);
			}//fi accept
		
			/* loop on open connections */
			i = sockfd;                  /* first socket to look */
			while (n != 0) 
			{              
				/* loop until active */
	   	 		i++;                      /* start after listening socket */
	    		//printf("restano %d socket, fd %d\n", n, fd);
	    		if (fd_open[i] == 0) continue;   /* closed, go next */
	    		if (FD_ISSET(i, &rset)) 
	    		{        
	    			/* if active process it*/
					n--;                         /* decrease active */
					//printf("dati su fd %d\n", i);
					client_data(i);

					nread = read(i, buffer, BUFSIZE);     /* read operations */
					if (nread < 0) 
					{
		    			perror("recv() error: ");
		    			return FAILURE;
					}
			
					if (nread == 0) 
					{            
						/* if closed connection */
		    			printf("fd %d chiuso\n", i);
		    			close(i);                /* close file */
		    			fd_open[i] = 0;          /* mark as closed in table */
		    			if (max_fd == i) 
		    			{       
		    				/* if was the maximum */
							while (fd_open[--i] == 0);    /* loop down */
							max_fd = i;          /* set new maximum */
							//printf("nuovo max_fd %d\n", max_fd);
							break;               /* and go back to select */
		    			}
		    			continue;                /* continue loop on open */
					}//fi close connection
					
					buffer[nread]=0;
					printf("Ricevuto '%s'\n", buffer);
					nwrite = FullWrite(i, buffer, nread); /* write data */
					if (nwrite) 
					{
		    			perror("send() error: ");
		    			return FAILURE;
					}
	    		}//fi recv
			}//wend recv
 		}//fi
	}//wend

		
	close(sockfd);
	
return 0;
}
/**@}*/