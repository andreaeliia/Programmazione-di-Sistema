#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define PORT 49154
#define BACKLOG 10 	/**< dimensione della coda di connessioni */

#define FAILURE 3 	/**< definizione del valore di errore di ritorno del processo 
						in caso di errori delle Sockets API */

const char *shared_file = "/Users/patryk/Desktop/universita/Magistrale/PrimoAnno/Sistemi/Esempi/apue.3e/Esempi/EserciziEsame/Esame50/sharedFile.txt";
const char *lock_file = "/Users/patryk/Desktop/universita/Magistrale/PrimoAnno/Sistemi/Esempi/apue.3e/Esempi/EserciziEsame/Esame50/lockFile.txt";
						
char* result;
key_t key;
int sharedMemId;
int fdLockFile;

void attachMem();
void detachMem();
void lockMem();
void unlockMem();
void createMem();

int main(int argc, char *argv[]){
	
	int res = 0; //valore di ritorno delle Sockets API
	
	int sockfd = 0; //connection socket: servirà per la comunicazione col server
	
	struct sockaddr_in server; //IPv4 server address, senza hostname resolution 
	socklen_t len = sizeof(server); //dimensione della struttura di indirizzo
	
	memset(&server, 0, sizeof(server)); //azzero la struttura dati
	server.sin_family = AF_INET; //specifico l'Address FAmily IPv4
	
	/*
	Specifico la porta del server da contattare, verso cui avviare il 3WHS
	*/
	
	
	
	
	server.sin_port = htons(PORT);
	
	if (argc == 1)
	{
		//non è stato indicato un indirizzo IPv4 per il server, usiamo localhost
		printf("\tTCP Client app connecting to localhost TCP server...\n");
		
		/* Specifico l'indirizzo del server usando 
		il wildcard address IPv4 INADDR_LOOPBACK per specificare 
		l'indirizzo di loopback 127.0.0.1 in Network Byte Order
		*/
		server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	} 
	else 
	{
		printf("\tTCP Client app connecting to TCP server at '%s'\n", argv[1]);
		
		/* Utilizzo inet_pton() per convertire l'indirizzo dotted decimal */
		res = inet_pton(AF_INET, argv[1], &(server.sin_addr));
		if (res == 1){
			printf("Memorizzato l'indirizzo IPv4 del server\n");
		}
		else if (res == -1)
		{
			perror("Errore inet_pton: ");
			return FAILURE;
		}
		else if (res == 0)
		{
			printf("The input value is not a valid IPv4 dotted-decimal string\n");
			return FAILURE;
		}
	}
	
	
	
	//ATTACH MEM AND OPEN A FILE TO LOCK
	createMem();
	fdLockFile = open(lock_file, O_RDWR, 0666);
	if (!fdLockFile) {
        printf("Unable to open for read/write\n");
    }
    
   
    //connessione al server
	/* open socket */
	while(1){
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		perror("socket error: ");
		return FAILURE;
	}
	
	
	//avvio il 3WHS
	lockMem();
	attachMem();
	res = connect(sockfd, (struct sockaddr *)&server, len);
	if (res != 0)
	{
		perror("connect() error: ");
		close(sockfd);
		return FAILURE;
	}
	
	
	//messaggio predefinito da inviare al server
	ssize_t n = 0;
	char msg[11];
	strncpy(msg,result,10);
	msg[10] = 0;
	printf("\n%s",msg);
	n = send(sockfd, "ciao amico",11, 0);
	if (n == -1) {
		perror("send() error: ");
		close(sockfd);
		return FAILURE;
	}
	detachMem();
	unlockMem();
		sleep(1);
	
	
	//4-way teardown
	close(sockfd);
	}
	detachMem();

return 0;
}








void createMem(){
key = ftok(shared_file,0);
	if(key == -1){
		printf("Error creation Key");
	}

	sharedMemId = shmget(key,0,0);
	}

void attachMem(){
	

	result = (char*)shmat(sharedMemId,NULL,0);
	if(result == (char*)(-1)){
		printf("Error attach Memory");
	}
}

void detachMem(){
	shmdt(result);

}

void lockMem(){
	struct flock region_to_lock;
    int res;

	
    
    region_to_lock.l_type = F_WRLCK;
    region_to_lock.l_whence = SEEK_SET;
    region_to_lock.l_start = 0;
    region_to_lock.l_len = 10;
    
    res = fcntl(fdLockFile, F_SETLK, &region_to_lock);
     if (res == -1) {
        printf("Process %d - failed to unlock region\n", getpid());
     }
    
}

void unlockMem(){
	
	struct flock region_to_lock;
    int res;
    
	region_to_lock.l_type = F_UNLCK;
    region_to_lock.l_whence = SEEK_SET;
    region_to_lock.l_start = 0;
    region_to_lock.l_len = 10;
    
    res = fcntl(fdLockFile, F_SETLK, &region_to_lock);
    if (res == -1) {
        printf("Process %d - failed to unlock region\n", getpid());
    } 
}




