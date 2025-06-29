/**
@addtogroup Group11 
@{
*/
/**
@file 	SelectClient.c
@author Catiuscia Melle

@brief Presentazione di un Client TCP/UDP che aggiunge il multiplexing su stdin e invia in loop dei messaggi al server.
		
Il client:
- apre un socket (TCP o UDP), 
- lo connette (anche per UDP), 
- ed invia messaggi al server.
*/


#include "Header.h"
#include "Utility.h"


/**
@brief Utility function
@param name, nome dell'eseguibile
*/
void usage(char *name){
	printf("Usage: %s <servername> <protocol> <domain>\n", name);
	printf("\tprotocol= TCP aut UDP;\n");
	printf("\tdomain= 0 (UNSPEC), 4(INET), 6(INET6)\n"); 
}


/**
@brief Esegue l'I/O multiplexing su stdin e socket connesso (UDP/TCP)
@param sock, il socket da monitorare
@param type, tipo di socket
@return nulla
*/
void multiplex(int sock, int type){
	
	printf("Insert messages for server\n");
	
	fd_set rset; 
	FD_ZERO(&rset);
	
	FD_SET(STDIN_FILENO, &rset);
	FD_SET(sock, &rset);
	
	int max = (sock > STDIN_FILENO)? sock: STDIN_FILENO;

	struct timeval timer;
	timer.tv_sec = 0;
	timer.tv_usec = 0;
	
	int result = 0;
	bool quit = false;
	
	//predispongo la comunicazione ed il file descriptor set da monitorare
	ssize_t n = 0;
	char msg[BUFSIZE] = "";
	
	while (!quit)
	{
		result = select(max+1, &rset, NULL, NULL, &timer);
		
		if (result == -1)
		{
			perror("select() error: ");
			break;
		}	
		
		if (result == 0)
		{
			//printf("select timeout\n");
		}
		
		if (result > 0)
		{
			if (FD_ISSET(STDIN_FILENO, &rset))
			{
				if (fgets(msg, BUFSIZE-1, stdin) != NULL)
				{
					/*
					fgets ritorna una stringa che termina con la sequenza
					'\n\0'
					il null-terminated non è contato da strlen,
					ma '\n' si.
					Per tagliare '\n' trasmetto (strlen(msg) - 1).
					
					@note: ATTENZIONE
					Ogni volta che premo "Return" (dò invio), fgets() ritorna una stringa è vuota.
					La stringa vuota viene letta da send che trasmette 0 bytes dal socket "sock".
					Dobbiamo distinguere 2 casi:
					- se sock è di tipo SOCK_STREAM, l'operazione non genera dati da trasmettere;
					- se sock è di tipo SOCK_DGRAM, l'operazione genera un datagram UDP vuoto, che viene inviato al server UDP
					
					Per eliminare questo scenario, possiamo effettuare send solo su stringhe non vuote.
					*/
					//if ( (strlen(msg) - 1) != 0){
						n = send(sock, msg, strlen(msg) - 1, 0);
						printf("sent %d bytes\n", (int)n);
					//}//fi
				}
				else
					break; //chiusura
			}
			
			
			if (FD_ISSET(sock, &rset))
			{
				n = recv(sock, msg, BUFSIZE-1, 0);
				if (n == -1)
				{
					perror("recv() error: ");
					close(sock);
					return; // ERROR;
				}
				else if (n > 0)
				{
					msg[n] = 0;
					printf("\tResponse %d bytes message '%s'\n", (int)n, msg);
				}
				else
				{	
					//n==0 over TCP: closed connection
					//if (type == SOCK_STREAM)
					//	break;
					
					/*
					In realtà non abbiamo necessità di usare il parametro type in input alla funzione,
					perché il tipo del socket può essere ottenuto leggendo le opzioni del socket:
					*/	
					int sockType = 0;
					socklen_t optlen = sizeof(sockType);
					if ( getsockopt(sock, SOL_SOCKET, SO_TYPE, &sockType, &optlen) == 0){
						if (sockType == SOCK_STREAM) {
							printf("This is a TCP socket that received a FIN segment\n");
							break;
						} else {
							printf("This is an UDP socket that received an empty datagram\n");
						}	
					}//fi getsockopt	
								
				}
			
				
						
			}//fi sock
		}//fi result

		FD_ZERO(&rset);
		FD_SET(STDIN_FILENO, &rset);
		FD_SET(sock, &rset);
		max = (sock > STDIN_FILENO)? sock: STDIN_FILENO;

		timer.tv_sec = 5;
		timer.tv_usec = 0;
	}//wend
	printf("Multiplex ended\n");
}



int main(int argc, char *argv[]){

	if (argc != 4)
	{
		usage(argv[0]);
		return INVALID;
	}
	
	int family = getFamily(argv[3]); 
	
	int type = 0;
	if ((strcmp(argv[2], "TCP") == 0) || (strcmp(argv[2],"tcp") == 0))
	{
		type = SOCK_STREAM;
	}
	else if (strcmp(argv[2], "UDP") == 0 || (strcmp(argv[2],"udp") == 0))
	{
		type = SOCK_DGRAM;
	}
	else
	{
		printf("Invalid service type\n");
		return FAILURE;
	}

	int sockfd = open_socket(family, argv[1], SERVICEPORT, type);
	if (sockfd == INSUCCESS)
	{
		printf("Errore nell'aprire il socket e stabilire la connessione al server (TCP only)\n");
		return ERROR;
	}

	//ho un socket CONNESSO verso la destinazione specificata:
	struct sockaddr destination;
	socklen_t len = sizeof(destination);
	int res = getpeername(sockfd, &destination, &len);
	if (res != 0) {
		close(sockfd);
		return FAILURE;
	}
	//visualizzo il remote address
	printf("Connected to remote address: ");
	printAddressInfo(&destination, len);
	printf("\n");

	
	multiplex(sockfd, type);

	printf("Closing the socket...\n");
	close(sockfd);

return 0;
}

/** @} */
