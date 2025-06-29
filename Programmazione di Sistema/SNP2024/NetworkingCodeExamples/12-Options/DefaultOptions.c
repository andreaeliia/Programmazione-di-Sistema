/**
@defgroup Group12 Socket Options

@brief Fetching and setting socket options
@{
*/
/**
@file 	DefaultOptions.c
@author Catiuscia Melle

@brief 	Questo codice mostra come leggere il valore di default - sul sistema di riferimento -
		di alcune socket options.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include <sys/socket.h>


/**
@brief, funzione che apre un socket nel dominio e del tipo indicato
@param family, the domain
@param type, tipo STREAM or DGRAM
@return, socket descriptor
*/
int openSocket(int family, int type){
	int sock = 0;

	sock = socket(family, type, 0);
	
	if (sock == -1)
	{
		perror("socket() error: ");
		//return sock;
	}

return sock;
}



int main(int argc, char *argv[]){
	
	
	if (argc != 3){
		printf("Usage: %s <domain> <type>\n", argv[0]);
		printf("\tdomain: 4 (AF_INET), 6 (AF_INET6)\n");
		printf("\ttype: T (TCP), U (UDP)\n");
		return 1;
	}
	
	int family = atoi(argv[1]) == 4 ? AF_INET : AF_INET6;
	int type = strncmp(argv[2], "T", 1) == 0 ? SOCK_STREAM : SOCK_DGRAM; 
	
	
	int sock = openSocket(family, type);
	if (sock == -1 ){
		printf("Error while opening the socket\n");
		return 2;
	}
	
	printf("\nSocket Option Level options\n\tDefault values for a %s socket\n", type == SOCK_STREAM ? "TCP" : "UDP");
		
	int reuse = 0;
	socklen_t size = sizeof(reuse); 
	
	int res = getsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, &size);
	if (res == -1)
	{
		perror("getsockopt() SO_REUSEADDR error: ");
	}
	else
		printf("Default value for SO_REUSEADDR is \t%s\n", reuse==0 ? "OFF" : "ON");

	
	struct timeval timeouts;
	timeouts.tv_sec = timeouts.tv_usec = 0;
	size = sizeof(timeouts);

	res = getsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeouts, &size);
	if (res == -1)
	{
		perror("getsockopt() SO_RCVTIMEO error: ");
	}
	else
		printf("Default value for SO_RCVTIMEO is \t%s\n", !(timeouts.tv_sec && timeouts.tv_usec) ? "OFF" : "ON");
		

	res = getsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeouts, &size);
	if (res == -1)
	{
		perror("getsockopt() SO_SNDTIMEO error: ");
	}
	else
		printf("Default value for SO_SNDTIMEO is \t%s\n", !(timeouts.tv_sec && timeouts.tv_usec) ? "OFF" : "ON");


	int bytes = 0;
	size = sizeof(int);
	
	res = getsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bytes, &size);
	if (res == -1)
	{
		perror("getsockopt() SO_RCVBUF error: ");
	}
	else
		printf("Default value for SO_RCVBUF is \t%d\n", bytes);

	
	res = getsockopt(sock, SOL_SOCKET, SO_SNDBUF, &bytes, &size);
	if (res == -1)
	{
		perror("getsockopt() SO_SNDBUF error: ");
	}
	else
		printf("Default value for SO_SNDBUF is \t%d\n", bytes);



	res = getsockopt(sock, SOL_SOCKET, SO_RCVLOWAT, &bytes, &size);
	if (res == -1)
	{
		perror("getsockopt() SO_RCVLOWAT error: ");
	}
	else
		printf("Default value for SO_RCVLOWAT for a TCP socket is \t%d\n", bytes);


	res = getsockopt(sock, SOL_SOCKET, SO_SNDLOWAT, &bytes, &size);
	if (res == -1)
	{
		perror("getsockopt() SO_SNDLOWAT error: ");
	}
	else
		printf("Default value for SO_SNDLOWAT for a TCP socket is \t%d\n", bytes);


	
	if (type == SOCK_STREAM) 
	{
		reuse = 0;
		size = sizeof(reuse); 
		
		res = getsockopt(sock, SOL_SOCKET, SO_ACCEPTCONN, &reuse, &size);
		if (res == -1)
		{
			perror("getsockopt() SO_ACCEPTCONN error: ");
		}
		else
			printf("Default value for SO_ACCEPTCONN is \t%s\n", reuse==0 ? "OFF" : "ON");

		
		reuse = 0;
		size = sizeof(reuse); 
		
		res = getsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &reuse, &size);
		if (res == -1)
		{
			perror("getsockopt() SO_KEEPALIVE error: ");
		}
		else
			printf("Default value for SO_KEEPALIVE is \t%s\n", reuse==0 ? "OFF" : "ON");
			
			
		
		struct linger lValue = {0,0};
		size = sizeof(lValue);
		res = getsockopt(sock, SOL_SOCKET, SO_LINGER, &lValue, &size);
		if (res == -1)
		{
			perror("getsockopt() SO_LINGER error: ");
		}
		else
			printf("Default value for SO_LINGER for a TCP socket is \t%d, %d\n", lValue.l_onoff, lValue.l_linger);

	}//fi SOCK_STREAM


	if (type == SOCK_DGRAM) 
	{
		reuse = 0;
		size = sizeof(reuse); 
		
		res = getsockopt(sock, SOL_SOCKET, SO_BROADCAST, &reuse, &size);
		if (res == -1)
		{
			perror("getsockopt() SO_BROADCAST error: ");
		}
		else
			printf("Default value for SO_BROADCAST is \t%s\n", reuse==0 ? "OFF" : "ON");

	}//fi SOCK_DGRAM


	close(sock);
	printf("\n");

return 0;
}


