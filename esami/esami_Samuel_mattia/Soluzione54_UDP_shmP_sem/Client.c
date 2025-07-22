#include "Header.h"

int main(int argc, char *argv[]){
	
	if (argc < 2)
	{
		printf("Usage: %s <IPv4 (dotted string) server address> [message]\n", argv[0]);
		return FAILURE;
	}
	
	printf("\tIPv4 UDP Client app\n");
	
	char msg[BUFSIZE] = "";
	if (argc == 3)
	{
		memcpy(msg, argv[2], BUFSIZE-1);
	}
	else
	{
		memset(msg, '@', BUFSIZE-1);
	}
	
	msg[BUFSIZE-1] = '\0';

	int res = 0;
	int sockfd = 0;
	
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == -1)
	{
		perror("socket error: ");
		return FAILURE;
	}
	
	struct sockaddr_in server;
	socklen_t len = sizeof(server);
	
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	
	res = inet_pton(AF_INET, argv[1], &(server.sin_addr));
	if (res == 1)
	{
		printf("Memorizzato l'indirizzo IPv4 del server %s\n", argv[1]);
	}
	else if (res == -1)
	{
		perror("Errore inet_pton(): ");
		return FAILURE;
	}
	else if (res == 0)
	{
		printf("The input value is not a valid IPv4 dotted-decimal string, using loopback\n");
		server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	}
	
	server.sin_port = htons(PORT);

	ssize_t n = 0;
	n = sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&server, len);
	if (n == -1)
	{
		perror("sendto() error: ");
		close(sockfd);
		return FAILURE;
	}
	char serverIP[INET_ADDRSTRLEN] = "";
	
	printf("Client sent %d bytes to server at '%s:%d'\n", (int) n, inet_ntop(AF_INET, &(server.sin_addr), serverIP, INET_ADDRSTRLEN), ntohs(server.sin_port));
	
	sleep(10);
	
	int drop = n/2; 
	n = recvfrom(sockfd, msg, drop, 0, (struct sockaddr *)&server, &len);
	/* n = recvfrom(sockfd, msg, BUFSIZE-1, 0, (struct sockaddr *)&server, &len); */
	if (n == -1)
	{
		perror("recvfrom() error: ");
		close(sockfd);
		return FAILURE;
	}
	
	msg[n] = '\0';
	printf("Echoed message = '%s'\n", msg);
	
	close(sockfd);
	
return 0;
}