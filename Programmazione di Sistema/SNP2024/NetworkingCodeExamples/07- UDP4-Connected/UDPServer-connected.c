/**
@defgroup Group7 UDP4 examples
@brief UDP/IPv4 Client and Server

Il codice implementa un semplice esempio di UDP Echo server e client.
Il server è programmato in modo che possa distingure due messaggi speciali:
- il primo fa scattare l'esecuzione della connect sul socket UDP del server
- il secondo fa scattare la disconnessione del socket UDP.

Attenzione: la disconnessione di un socket UDP non è portabile.
Inoltre, mentre su Mac OS X mantiene il binding della porta, su Linux il binding di porta si perde.

Mentre il server ha il socket connesso ad un client, non potrà ricevere traffico da parte di altri peer. 

Sono presenti inoltre due diversi UDP client:
- il ClientConnected esegue una connect del suo socket UDP indicando l'indirizzo del server UDP con cui vuole comunicare
- il codice in UDPClient.c invece semplicemente invia i messaggi.

Attenzione: se questo secondo client è mandato in esecuzione mentre il processo server UDP non è running, un ICMP viene ricevuto dall'host client che però no è notificato all'applicazione che rimane bloccata nella recvfrom. 
@{
*/
/**
@file 	UDPServer-connected.c
@author Catiuscia Melle

@brief 	Presentazione di un UDP Echo Server IPv4 che utilizza connect() sul 
		socket utilizzato per ricevere i messaggi dei client.

Il server connette il socket all'indirizzo del primo client che lo contatta.

Se altri client UDP cercano di contattarlo (usando la versione UDPClient.c) 
rimarranno appesi nella recvfrom...
*/

#include "Header.h"



/**
@brief Disconnessione di un socket UDP connesso
@param sockfd - socket UDP connesso
@return nulla
*/
void disconnect(int sockfd){

	struct sockaddr_in none;
	memset(&none, 0, sizeof(none));
	//none.sin_family = AF_UNSPEC;
	none.sin_family = AF_INET;
	
	int res = connect(sockfd, (struct sockaddr *)&none, sizeof(none));
	
	if (res == -1)
	//if (res != 0 && errno == EAFNOSUPPORT)
	{
		perror("connect() error: ");
	}
	else
	{
		printf("\n\tConnect to null address successful\n\n");
	}
	
return;
}



int main(int argc, char *argv[]){
	
	printf("\tIterative UDP4 echo server\n");
		
	int res = 0; //valore di ritorno delle APIs
	int sockfd = 0; //socket: servirà per la comunicazione col server
	
	//1- open socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == -1)
	{
		perror("socket error: ");
		return FAILURE;
	}
	
	
	//2- indirizzo IPv4 del server, senza hostname resolution
	struct sockaddr_in server;
	socklen_t len = sizeof(server);
	
	memset(&server, 0, sizeof(server)); //azzero la struttura dati
	server.sin_family = AF_INET; //specifico l'Address Family IPv4
	server.sin_addr.s_addr = htonl(INADDR_ANY);//IP: qualsiasi interfaccia
	server.sin_port = htons(PORT);//Specifico la well-known port
	
	//bind to the well-known server address
	res = bind(sockfd, (struct sockaddr *)&server, len);
	if (res == -1)
	{
		perror("Bind error: ");
		close(sockfd);
		exit(1);
	}//fi
	
		
	ssize_t n = 0;
	char buffer[BUFSIZE] = "";
	struct sockaddr_in client;
	char address[INET_ADDRSTRLEN] = "";
	
	bool isConnected = false;
	
	printf("Waiting messages from my peers...\n");
	
	while (1)
	{
	
		n = recvfrom(sockfd, buffer, BUFSIZE-1, 0, (struct sockaddr *)&client, &len);
		if (n == -1)
		{
			perror("recvfrom() error: ");
			continue;
// 			close(sockfd);
// 			return FAILURE;
		}
	
		buffer[n] = '\0';
		printf("\t-'%s'\n\tda: %s:%d\n", buffer, \
		inet_ntop(AF_INET, &(client.sin_addr), address, INET_ADDRSTRLEN), \
		ntohs(client.sin_port) );
		
		if (strncmp(buffer, CONNECTMSG, strlen(CONNECTMSG)) == 0){	
			printf("\n\tConnecting the UDP socket to the peer\n\n");
			res = connect(sockfd, (struct sockaddr *)&client, len);
			if (res == -1)
			{
				perror("Error on UDP connect(): ");
				close(sockfd);
				exit(1);
			}//fi
			
			isConnected = true;
		} 
		
		if (isConnected && strncmp(buffer, DISCONNECTMSG, strlen(DISCONNECTMSG)) == 0) {
			disconnect(sockfd);
			isConnected = false;
		}
		
		if (isConnected){
			n = send(sockfd, buffer, strlen(buffer), 0);
			if (n == -1)
			{
				perror("send() error: ");
				close(sockfd);
				return FAILURE;
			}
		}
		else
		{
			n = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client, len);
			if (n == -1)
			{
				perror("sendto() error: ");
				close(sockfd);
				return FAILURE;
			}
		}
		
		printf("Sent %d bytes reply...\n", (int)n);
	
	}//wend
		
	close(sockfd);
	
return 0;
}

/** @} */

