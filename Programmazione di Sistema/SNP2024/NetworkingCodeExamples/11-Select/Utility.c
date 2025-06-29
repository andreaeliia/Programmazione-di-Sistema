/**
@addtogroup Group11
@{
*/
/**
@file 	Utility.c
@author Catiuscia Melle

@brief 	Implementazione di funzioni di utilità.

*/

#include "Utility.h"


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





int open_socket(int family, char *hostname, char *servicename, int type){

	int sockfd = 0; //valore di ritorno della funzione
	int result = 0;
	
	struct addrinfo hints, *res, *p;
		
	memset(&hints, 0, sizeof(hints)); //azzero hints
		
	hints.ai_family = family; //richiedo la risoluzione per IPv6
	hints.ai_socktype = type; //service type
	
	/*
	Se richiedo AF_INET6 come family, 
	allora specificando il flag AI_V4MAPPED, 
	getaddrinfo() deve ritornare l'IPv4-mapped IPv6 address 
	se non trova indirizzi IPv6. 
	Diversamente, l'opzione non è presa in considerazione
	*/
	hints.ai_flags = AI_V4MAPPED;
	
	result = getaddrinfo(hostname, servicename, &hints, &res);
	if (result != 0)
	{
		printf("getaddrinfo: %s\n", gai_strerror(result));
		return INSUCCESS; 
	}
	
	printf("Resolution for: %s:%s Done!\n", hostname, servicename);
	
	for (p = res; p != NULL; p = p->ai_next)
	{
		sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sockfd == -1)
		{	
			perror("socket() error: ");
			continue;	
		}
		//TCP o UDP, il socket viene comunque connesso al destination address
		result = connect(sockfd, p->ai_addr, p->ai_addrlen);
		if (result == -1)
		{
			perror("connect() error: ");
			close(sockfd);
			continue;
		}

	break;
	}//for

	if (p == NULL) //if (!p)
	{
		printf("Connessione al server %s:%s fallita\n", hostname, servicename);
		return INSUCCESS; 
	}

	//dealloco risorse resolver
	freeaddrinfo(res);

return sockfd;
}



int getFamily(char *param){
	int family = 0;
	if (strncmp(param, "4", 1) == 0){
		family = AF_INET;
	} else if (strncmp(param, "6", 1) == 0){
		family = AF_INET6;
	} else {
		family = AF_UNSPEC;
	} 
	return family;
}








/*************************************************************************/ 

int passiveOpen(int family, char *nodename, char *servicename, int socktype) {

	int sock = INSUCCESS; 
	int result = 0;

	struct addrinfo hints, *res, *p;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = family;
	hints.ai_flags = AI_PASSIVE; 
	hints.ai_socktype = socktype;

	hints.ai_flags |= AI_V4MAPPED;
	hints.ai_flags |= AI_NUMERICSERV;

	result = getaddrinfo(nodename, servicename, &hints, &res);
	if (result != 0)
	{
		printf("getaddrinfo: %s\n", gai_strerror(result));
		return INSUCCESS;
	}

	int reuse = 1; //opzione SO_REUSEADDR

	for (p = res; p != NULL ; p = p->ai_next)
	{
		sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sock == -1)
		{
			perror("socket() error: ");
			continue;	
		}

		if ( setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0 )
		{
			perror("setsockopt() error: ");
			continue;
		}

		result = bind(sock, p->ai_addr, p->ai_addrlen);
		if (result == -1)
		{
			perror("bind() error: ");
			close(sock);
			continue;
		}

		break;
	}//for
	
	if (p == NULL) 
	{
		printf("Non siamo riusciti ad avviare il server %s:%s per il servizio %d\n", nodename, servicename, socktype);
		sock = INSUCCESS;
	}

	//dealloco le risorse
	freeaddrinfo(res);

	if (socktype == SOCK_STREAM)
	{
		//TCP passive open 
		result = listen(sock, BACKLOG);
		if (result == -1)
		{
			perror("listen() error: ");
			close(sock);
			return INSUCCESS;
		}
	}

return sock;
}




void handleUDPMessage(int sockU){
	
	//protocol-independent addresses management
	struct sockaddr_storage client; 
	socklen_t sslen = sizeof(client);
	
	char msg[BUFSIZE] = "";
	ssize_t n = 0;
	
	n = recvfrom(sockU, msg, BUFSIZE-1 , 0, (struct sockaddr *)&client, &sslen);

	if (n < 0)
	{
		perror("error on recvfrom(): ");
		return;
	}
	
	printAddressInfo((struct sockaddr *)&client, sslen);
	msg[n] = '\0';
	printf("Received UDP %d bytes message '%s'\n", (int)n, msg);
		
	char c = toupper(msg[n-1]);
	msg[n-1] = toupper(msg[0]);	
	msg[0] = c;
	
	printf("Reply to msg\n");
	n = sendto(sockU, msg, strlen(msg), 0, (struct sockaddr *)&client, sslen);
	if (n == -1)
	{
		perror("sendto() error: ");
	}
	
return;
}



int handleTCPClient(int sock){
	
	char msg[BUFSIZE] = "";
	ssize_t n = 0;
	
	//get the message from client
	n = recv(sock, msg, BUFSIZE-1, 0);
	
	if (n > 0)
	{
		//protocol-independent management of connections
		struct sockaddr_storage client; 
		socklen_t sslen = sizeof(client);
		char ipstr[INET6_ADDRSTRLEN] = "";
		char portstr[INET6_ADDRSTRLEN] = "";
		
		getpeername(sock, (struct sockaddr *)&client, &sslen);
		
		
		int niflags = NI_NUMERICSERV | NI_NUMERICHOST;
		int res = getnameinfo( (struct sockaddr *)&client, sslen, \
							ipstr, INET6_ADDRSTRLEN, portstr, INET6_ADDRSTRLEN, niflags);
		
		if (res == 0)
		{
			printf("Received TCP message from client da %s:%s\n", ipstr, portstr);
		}
		
		msg[n] = '\0';
		printf("\tmessage '%s'\n\t%d bytes\n",  msg, (int)n);
		
		msg[0] = toupper(msg[0]);
		msg[n/2] = toupper(msg[n/2]);	
		msg[n-1] = toupper(msg[n-1]);	
		n = send(sock, msg, strlen(msg), 0);
		if (n == -1)
		{
			perror("send() error: ");
		}
	return 1;
	}
	
	if (n == 0)
	{
		printf("il client ha chiuso la connessione\n");
		return 0;
	} 
	
	
	if (n < 0)
	{
		perror("error on recv(): ");
		return -1;
	}
    
    return -1;
}




