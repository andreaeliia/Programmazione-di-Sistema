/**
@addtogroup Group30
@{
*/
/**
@file 	Shares.c
@author Catiuscia Melle
*/

#include "Shares.h"

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




int set_nonblock(int sockfd){

	int flags;
	
	//preleviamo il set di flags con F_GETFL
	flags = fcntl(sockfd, F_GETFL, 0);
	if (flags < 0)
	{
		perror("fcntl error");
		return FAILURE;
	}
	
	//OR logico 
	flags |= O_NONBLOCK;
	
	//assegniamo il nuovo set di flags al socket con F_SETFL
	if (fcntl(sockfd, F_SETFL, flags) < 0)
	{
		perror("fcntl error");
		return FAILURE;
	}
	
return SUCCESS;
}


int set_block(int sockfd){
	int flags;
	
	flags = fcntl(sockfd, F_GETFL, 0);
	if (flags < 0){
		perror("fcntl error");
		return FAILURE;
	}
	
	flags &=~O_NONBLOCK;
	
	if (fcntl(sockfd, F_SETFL, flags) < 0)
	{
		perror("fcntl error");
		return FAILURE;
	}
	
return SUCCESS;
}


/**@}*/