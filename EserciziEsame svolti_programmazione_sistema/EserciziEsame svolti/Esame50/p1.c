#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include <arpa/inet.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <pthread.h>
#include <signal.h>
#include <time.h>

#define BILLION  1000000000L;
#define PORTA 49152
#define PORTB 49154

void genera (char array[]);

struct timespec start, stop;
double accum;

void * t1Server(void *arg)

{	char stringaCasuale[10];
	int serverSocket, newSocket;
	struct sockaddr_in serverAddr;
	socklen_t addr_size;
  	struct sockaddr_storage serverStorage;
  	
	
	//Create the socket. 
  	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  	
  	
  	// Configure settings of the server address struct
  	// Address family = Internet 
 	serverAddr.sin_family = AF_INET;
 	
 	//Set port number, using htons function to use proper byte order 
  	serverAddr.sin_port = htons(PORTA);
  	
  	//Set IP address 
  	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  	
  	//Set all bits of the padding field to 0 
  	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
  	
  	//Bind the address struct to the socket 
  	bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
  	
  	//Listening
  	if(listen(serverSocket,10)==0)
    printf("\nListening T1\n");
  	else
    printf("Error\n");
    
    
    
    while(1){
	//Accept call creates a new socket for the incoming connection
    addr_size = sizeof serverStorage;
    newSocket = accept(serverSocket, (struct sockaddr *) &serverStorage, &addr_size);
    sleep(1);
    
    
    //Start measure time for the loop
    if( clock_gettime( CLOCK_REALTIME, &start) == -1 ) {
      perror( "clock gettime" );
    }
    
    //generate a random String
	genera(stringaCasuale);
	
	int n = send(newSocket, stringaCasuale, strlen(stringaCasuale), 0);
	
		if(n == -1){
			printf("Error during trasmission");
		}
	
	close(newSocket);
	
	}
	
  pthread_exit(NULL);

}


void * t2Server(void *arg)

{	char *stringaRicevuta;
	int serverSocket, newSocket;
	struct sockaddr_in serverAddr;
	socklen_t addr_size;
  	struct sockaddr_storage serverStorage;
  	
	
	//Create the socket. 
  	serverSocket = socket(PF_INET, SOCK_STREAM, 0);
  	
  	// Configure settings of the server address struct
  	// Address family = Internet 
 	serverAddr.sin_family = AF_INET;
 	
 	//Set port number, using htons function to use proper byte order 
  	serverAddr.sin_port = htons(PORTB);
  	
  	//Set IP address 
  	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  	
  	//Set all bits of the padding field to 0 
  	memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);
  	
  	//Bind the address struct to the socket 
  	bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
  	
  	//Listening
  	if(listen(serverSocket,40)==0)
    printf("\nListening T2\n");
  	else
    printf("Error\n");
    
    
    while(1){
    
	//Accept call creates a new socket for the incoming connection
    addr_size = sizeof serverStorage;
    sleep(1);
    newSocket = accept(serverSocket, (struct sockaddr *) &serverStorage, &addr_size);
	
	
	int n = recv(newSocket, stringaRicevuta, 100, 0);
			if (n == -1)
			{
				perror("recv() error: ");
			}
			
	printf("\nHo ricevuto la stringa %s\n",stringaRicevuta);
	
	//Calculate Stop time
	if( clock_gettime( CLOCK_REALTIME, &stop) == -1 ) {
      perror( "clock gettime" );
    }
    
    //Print total Time
    accum = ( stop.tv_sec - start.tv_sec ) + (double)( stop.tv_nsec - start.tv_nsec ) / (double)BILLION;
    printf( "\nIl numero di nanosecondi trascorsi è %lf\n", accum );
    
	close(newSocket);
	
	
	
	}
	
  pthread_exit(NULL);

}

int main(){
	pthread_t t1,t2;
	int s;
	
	s = pthread_create(&t1, NULL, t1Server, NULL);
    if (s != 0){
        printf("Error pthread1_create");
        return 1;
	}
	
	s = pthread_create(&t2, NULL, t2Server, NULL);
    if (s != 0){
        printf("Error pthread2_create");
        return 1;
	}
	
	
	
	s = pthread_join(t1, NULL);
    if (s != 0)
        printf("Error pthread1_join");
    
    s = pthread_join(t2, NULL);
    if (s != 0)
        printf("Error pthread1_join");


}


void genera (char array[]) //j è il contatore del ciclo che chiama genera()
{
int i;
srand (time(NULL));
for (i=0; i<10; )
array[i++] = (char) ((rand() % 26)+ 65);
}
