#include "Header.h"
#include "apue.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
socklen_t len;

int main(){
	
	int			fdin,fdlk;
	void		*src;
	char 		*p;
	
	int sockfd = UDPInit();
	
		
	ssize_t n = 0;
	char buffer[BUFSIZE];
	
	struct sockaddr_in client;
	char address[INET_ADDRSTRLEN] = "";
	
	int quit = 0;
		
	
	
	while (!quit)
	{	
	
		if ((fdlk = open("/Users/patryk/Desktop/universita/Magistrale/PrimoAnno/Sistemi/Esempi/apue.3e/Esempi/EserciziEsame/Esame54/FileDiLock.txt", O_RDWR|O_CREAT,0666)) < 0)
			err_sys("can't open FileDiLock");
	 	
	 	writew_lock(fdlk, 0, SEEK_SET, 1);
	
		n = recvfrom(sockfd, buffer, BUFSIZE-1, 0, (struct sockaddr *)&client, &len);
		if (n == -1)
		{
			perror("recvfrom() error: ");
			continue;
// 			close(sockfd);
// 			return FAILURE;
		}
	
		/* creo l'area condivisa */
		if ((fdin = open("/Users/patryk/Desktop/universita/Magistrale/PrimoAnno/Sistemi/Esempi/apue.3e/Esempi/EserciziEsame/Esame54/FileCondiviso.txt", O_RDWR|O_CREAT|O_TRUNC,0666)) < 0)
			err_sys("can't open FileCondiviso for writing");
		
		if (lseek(fdin, BUFSIZE, SEEK_SET) == -1)
    	{
        	perror("Lseek function error.");
        	//return 1;
   		 }
   		 
   		write(fdin," ",1);
		
		writew_lock(fdin, 0, SEEK_SET, BUFSIZE * 2);
		
		if ((src = mmap(0, BUFSIZE * 2, PROT_WRITE, MAP_SHARED,fdin, 0)) == MAP_FAILED)
		err_sys("mmap error for input");
	    p=src;
		
		char stringaDaCopiare[2*BUFSIZE] = "";
		
		strcat (stringaDaCopiare, "IP: ");
		char* stringa1 = inet_ntop(AF_INET, &(client.sin_addr), address, INET_ADDRSTRLEN);
		strcat (stringaDaCopiare, stringa1);
		strcat (stringaDaCopiare, " PORT: ");
		
		char  stringa2 [10];
		sprintf(stringa2, " %d ", ntohs(client.sin_port));
		strcat (stringaDaCopiare, stringa2);
		strcat (stringaDaCopiare, "\n");
		strcat (stringaDaCopiare, buffer);
		stringaDaCopiare[2*BUFSIZE - 1] = '\0';
		
		
		for(int i = 0; i < BUFSIZE-1; i++)
			p[i] = stringaDaCopiare[i];
		
		
		un_lock(fdlk, 0, SEEK_SET, 1);
		un_lock(fdin, 0, SEEK_SET, BUFSIZE*2);
		
		
	
	}
		
	
	close(sockfd);
	
return 0;
}


