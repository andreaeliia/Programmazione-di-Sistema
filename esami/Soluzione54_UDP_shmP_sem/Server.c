#include "Header.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "tlpi_hdr.h"
#include <string.h>

#define shm_name "IPText"

int main(){

	int fd;
    size_t size;
    void *addr;

    fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (fd == -1){
        perror("shm_open");
        exit(-1);
    }



	
	printf("\tIPv4 UDP Server app\n");
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
	
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(PORT);
	
	res = bind(sockfd, (struct sockaddr *)&server, len);
	if (res == -1)
	{
		perror("Bind error: ");
		close(sockfd);
		exit(1);
	}
		
	ssize_t n = 0;
	char buffer[BUFSIZE];
	
	struct sockaddr_in client;
	char address[INET_ADDRSTRLEN] = "";
	
	int quit = 0;
	
	while (!quit)
	{
		n = recvfrom(sockfd, buffer, BUFSIZE-1, 0, (struct sockaddr *)&client, &len);
		if (n == -1)
		{
			perror("recvfrom() error: ");
			continue;
		}
	
		buffer[n] = '\0';
		printf("\tRicevuto messaggio:\n\t'%s'\n\tda: %s:%d\n", buffer, \
		inet_ntop(AF_INET, &(client.sin_addr), address, INET_ADDRSTRLEN), \
		ntohs(client.sin_port) );

		size = INET_ADDRSTRLEN + 1 + n;
		if (ftruncate(fd, size) == -1){
        	perror("ftruncate");
        	exit(-1);
		}
		addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (addr == MAP_FAILED){
            perror("mmap");
            exit(-1);
        }

        printf("copying %ld bytes\n", (long) len);
        char * first_row = strcat(address, "\n");
        printf("%s", first_row);
        int msg_len = 0;
        for (msg_len = 0; buffer[msg_len] != '\0'; msg_len++);
        char second_row[msg_len + 1];
    	strncpy(second_row, buffer, msg_len + 1);
    	printf("%s", second_row);
        char * shm_message = strcat(first_row, second_row);
        printf("%s", shm_message);
    	memcpy(addr, shm_message, size); 
    	printf("shm message: %s\n", shm_message);

		printf("Sending reply...\n");
		
		n = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client, len);
		if (n == -1)
		{
			perror("sendto() error: ");
			continue;
		}
	
	}
		
	close(sockfd);
	
return 0;
}