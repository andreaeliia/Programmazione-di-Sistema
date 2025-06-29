/**
@addtogroup Group14
@{
*/
/**
@file 	ChatMulticast.c
@author Catiuscia Melle
@brief 	Implementazione di un multicast sender/receiver per simulare una chat di gruppo.

Attenzione: questa applicazione apre 2 socket UDP:
- il primo serve per fare l'invio al gruppo multicast;
- il secondo serve per ricevere un traffico multicast;

L'obiettivo è quello di implementare uno scambio N:M. 
Se l'opzione LOOPBACK è disabilitata, ogni processo figlio riceverà solo il traffico degli 
altri nodi partecipanti all'esperimento.
Se l'opzione di LOOPBACK è attiva, allora riceverà anche il traffico generato 
dal suo parent.

*/

#include "Utility.h"


int main(int argc, char *argv[]){
	
	if (argc != 2)
	{
		printf("Usage: %s <loop flag> (1 loop on)\n", argv[0]);
		return FAILURE;
	}
	
	printf("Multicast CHAT@%s:%s\n", MGROUP, MPORT);
	
	
	struct addrinfo hints, *res, *p;
	memset(&hints, 0, sizeof(hints)); //Zero out hints
  	
  	//resolution hints:
	hints.ai_family = AF_UNSPEC;      // v4 or v6 is OK
  	hints.ai_socktype = SOCK_DGRAM;   // Only datagram sockets
  	hints.ai_protocol = IPPROTO_UDP;  // Only UDP protocol
  	hints.ai_flags |= AI_NUMERICHOST; // Don't try to resolve address
	hints.ai_flags |= AI_NUMERICSERV; // Don't try to resolve port

	int ecode = getaddrinfo(MGROUP, MPORT, &hints, &res);
	if (ecode != 0)
	{
		printf("getaddrinfo error: %s\n", gai_strerror(ecode));
		return FAILURE;
	}
		
	p = res;
	int sender = -1;
	int receiver = -1; 
	
	//Attenzione: dobbiamo aprire 2 socket
	while (p != NULL) 
	{
		sender = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sender != -1)
			break; //success
		
		p = p->ai_next;
	}//wend
	
	if (p == NULL)
	{	/* errno set from final socket() */
		perror("socket error: ");
		freeaddrinfo(res);
		return FAILURE;
	}
	
	//apriamo il secondo socket
	receiver = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
	if (receiver < 0)
	{
		perror("socket() error: ");
		close(sender);
		freeaddrinfo(res);
		return FAILURE;
	}
	
	int y = 1;
	int result = setsockopt(receiver, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(y));
	if (result < 0)
	{
		perror("setsockopt() error: ");
		return FAILURE;
	}
	
	//il receiver socket è legato al multicast address
	result = bind(receiver, p->ai_addr, p->ai_addrlen);
	
	//copio l'indirizzo multicast
	struct sockaddr multicast_address;
	memset(&multicast_address, 0, sizeof(struct sockaddr));
	
	socklen_t multicast_len = p->ai_addrlen;
	memcpy(&multicast_address, p->ai_addr, multicast_len);	
	
	//dealloco res
	freeaddrinfo(res);
	
		
	//Join al gruppo multicast per il receiver socket
	result = join_group(receiver, &multicast_address, multicast_len);
	if (result != SUCCESS)
	{
		printf("Error on join multicast group\n");
		close(receiver);
		close(sender);
		return FAILURE;
	}
	
	if (atoi(argv[1]) != 1)
	{
	
		result = disableLoop(multicast_address.sa_family, sender);
		if ( result != 0)
		{
			perror("setsockopt error on disable loopback: ");
			close(sender);
			close(receiver);
			return FAILURE;
		}
	}
	
	printf("invoke fork()\n");
	int pid = fork();
	
	if (pid == 0)
	{ 
		close(sender);
		//child process: receiver of the multicast traffic
		handle_multicast_session(receiver);
		
		//abbandono esplicito del gruppo
		result = leave_group(receiver, &multicast_address, multicast_len);
		close(receiver);
	}
	else
	{
		close(receiver);
		
		char sendline[BUFSIZE] = "";
		ssize_t res = 0;
		printf("Insert a message for the chat GROUP:\n");
		//char send_buffer[BUFSIZE] = "";
		//snprintf(send_buffer, BUFSIZE-1, "I'm process %d", (int)getpid());
		int quit = 0; 
		while ( (fgets(sendline, BUFSIZE, stdin) != NULL ) || (!quit) )
		{
	    	res = sendto(sender, sendline, strlen(sendline), 0, &multicast_address, multicast_len);
	    	if (res < 0)
	    	{
	    		perror("sendto() failed: ");
	    		quit = 1;
			} 
			else if (res != strlen(sendline))
			{
	    		printf("sendto() sent unexpected number of bytes\n");
	    		quit = 1;
			}
			
	    	sleep(FREQ);		
		}//wend
		
		close(sender);
	}//fi fork
	
	
return 0;
}

/** @} */

