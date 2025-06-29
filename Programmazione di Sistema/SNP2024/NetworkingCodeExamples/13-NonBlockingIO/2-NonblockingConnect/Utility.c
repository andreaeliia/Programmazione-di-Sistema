/**
@addtogroup Group31
@{
*/
/**
@file 	Utility.c
@author Catiuscia Melle
*/

#include "Utility.h"


int set_nonblock(int sockfd){

	int flags;
	
	flags = fcntl(sockfd, F_GETFL, 0);
	if (flags < 0){
		perror("fcntl error");
		return FAILURE;
	}
	
	flags |=O_NONBLOCK;
	
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



void client_data(int sockfd){
	
	struct sockaddr_storage addr;
	socklen_t len = sizeof(struct sockaddr_storage);
	
	char IP[INET6_ADDRSTRLEN];
	
	int result = getpeername(sockfd, (struct sockaddr *)&addr, &len);
	
	if (result == 0)
	{
		if (addr.ss_family == AF_INET)
		{
			struct sockaddr_in *ip = (struct sockaddr_in *)&addr;
			inet_ntop(AF_INET, &(ip->sin_addr), IP, INET6_ADDRSTRLEN);
			printf("Client from %s:%d\n", IP, ntohs(ip->sin_port));
		} else if (addr.ss_family == AF_INET6){
			struct sockaddr_in6 *ip = (struct sockaddr_in6 *)&addr;
			inet_ntop(AF_INET6, &(ip->sin6_addr), IP, INET6_ADDRSTRLEN);
			printf("Client from %s:%d\n", IP, ntohs(ip->sin6_port));
		}
	}//fi
}


ssize_t FullWrite(int fd, const void *buf, size_t count){
	
	size_t nleft;
	ssize_t nwritten;
	nleft = count;
	
	while (nleft > 0)
	{
		if ( (nwritten = write(fd, buf, nleft)) < 0) 
		{
			if (errno == EINTR){
				continue;
			} 
			else 
			{
				return nwritten;
			}
		}//fi
		
		//decremento il residuo di byte da trasferire
		nleft -=nwritten;
		//sposto il ptr di lettura
		buf +=nwritten;
	}//wend
	
	return (nleft);
}



void read_addrinfo(struct addrinfo *res){
	
	char buf4[INET_ADDRSTRLEN];
	char buf6[INET6_ADDRSTRLEN];
	int result = 0;
	struct addrinfo *ptr;
	
	for (ptr = res; ptr != NULL; ptr=ptr->ai_next)
	{	
		if (ptr->ai_family == AF_INET)
		{
			printf("ipv4\n");
			struct sockaddr_in *ipv4 = (struct sockaddr_in *)ptr->ai_addr;
			inet_ntop(AF_INET, &(ipv4->sin_addr), buf4, INET_ADDRSTRLEN);
			printf("addr = %s\n", buf4); 
		}
			
		if (ptr->ai_family == AF_INET6)
		{
			printf("ipv6\n");
			struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)ptr->ai_addr;	
			inet_ntop(AF_INET6, &(ipv6->sin6_addr), buf6, INET6_ADDRSTRLEN);
			printf("addr = %s\n", buf6); 
		}
			
		printf("flags = %d  ", ptr->ai_flags);
		printf("family = %d  ", ptr->ai_family);
		printf("socktype = %d ", ptr->ai_socktype); 
		printf("protocol = %d ", ptr->ai_protocol);
		printf(" canonname = '%s'\n", ptr->ai_canonname);
	}//for-wend
    	
}
/**@}*/