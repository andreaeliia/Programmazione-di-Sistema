#include "Header.h"
#include <sys/time.h>

#define SIZE 	1000000

struct timeval end_time;

int main(int argc, char *argv[]){
	
	printf("\n\tIPv4 TCP server demo\n");
	
	int sockfd = 0; /* listening socket del server TCP */
	int peerfd = 0; /* connected socket del client TCP */
	int ret = 0; 	/* valore di ritorno delle Sockets API */
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		perror("socket() error: ");
		return FAILURE;
	}
	
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family= AF_INET;
	
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(PORT);
	ret = bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
	if (ret == -1)
	{
		perror("bind() error: ");
		close(sockfd);
		return FAILURE;
	}
	
	ret = listen(sockfd, BACKLOG);
	if (ret == -1)
	{
		perror("listen() error: ");
		close(sockfd);
		return FAILURE;
	}
	
	printf("\n\tServer listening on port %d\n", (int)PORT);

	struct sockaddr_in peer_addr;
	socklen_t len = sizeof(peer_addr);
	
	int quit = 0;
	int connected = 0;
	while (!quit) 
	{
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
	
		int buf[SIZE];
		ssize_t n = 0;
		long int total_bytes = 0;
		connected = 1;
		
		while (connected) 
		{
			n = recv(peerfd, buf, sizeof(buf), 0);
			gettimeofday(&end_time, NULL);
			double millis_end = (end_time.tv_sec * 1000) + ((double) end_time.tv_usec / 1000.0);
			total_bytes += n;

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
				if (total_bytes == SIZE * sizeof(int))
				{
					printf("Received message: %ld bytes\n", total_bytes);
					n = send(peerfd, &millis_end, sizeof(double), 0);
				}
				
				if (n == -1) {
					perror("send error:");
				}
			}
		}
		close(peerfd);
	}
	close(sockfd);
	
return 0;
}