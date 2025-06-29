/** 
@addtogroup Group6
@{
*/
/**
@file 	shut-client.c
@author Catiuscia Melle

@brief 	Studio della chiusura di una connessione TCP

In questo esempio, un client TCP invia una sequenza di messaggi al server e
al termine può:
- chiudere la connessione TCP con close();
- usare shutdown per effettuare l'half-close della connessione TCP

Se il server è programmato per l'invio di un messaggio finale, alla ricezione dell'EOF 
sulla connessione:
- nel primo caso riceverà un RST in risposta all'invio del messaggio,
- nel secondo caso effettuerà l'invio e la chiusura della connessione senza problemi.


*/

#include "Header.h"


/**
@brief stampa il local address del socket connesso
@param sockfd, il socket connesso di cui si vuole conoscere l'indirizzo locale
@return il numero di porta locale, in host byte order
*/
int printMyAddress(int sockfd){
	struct sockaddr_in addr; 
	socklen_t len = sizeof(addr); 
	char myIP[INET_ADDRSTRLEN] = "";
	
	getsockname(sockfd, (struct sockaddr *)&addr, &len);
	
	printf("client connected with local address '%s:%d'\n", inet_ntop(AF_INET, &(addr.sin_addr), myIP, INET_ADDRSTRLEN), ntohs(addr.sin_port));
	
	return ntohs(addr.sin_port);
}


/**
@brief Esegue l'active Open per il client e ritorna il socket connesso
@param serverIP, l'indirizzo dotted decimal fornito in input al programma
@return -1 in caso di errore, il socket connesso in caso di successo
*/
int activeOpen(char *serverIP) {
	int res = 0;
	int sockfd = 0;
	
	struct sockaddr_in server; //IPv4 server address, senza hostname resolution 
	socklen_t len = sizeof(server); //dimensione della struttura di indirizzo
	memset(&server, 0, sizeof(server)); //azzero la struttura dati
	server.sin_family = AF_INET; //specifico l'Address FAmily IPv4
	
	/*
	Specifico la porta del server da contattare, verso cui avviare il 3WHS
	*/
	server.sin_port = htons(PORT);
	
	printf("\tTCP Client app connecting to TCP server at '%s'\n", serverIP);
	
	/* Utilizzo inet_pton() per convertire l'indirizzo dotted decimal */
	res = inet_pton(AF_INET, serverIP, &(server.sin_addr));
	if (res == 1){
		printf("Memorizzato l'indirizzo IPv4 del server\n");
	}
	else if (res == -1)
	{
		perror("Errore inet_pton: ");
		return -1;
	}
	else if (res == 0)
	{
		printf("The input value is not a valid IPv4 dotted-decimal string\n");
		return -1;
	}
	
	
	//connessione al server
	/* open socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		perror("socket error: ");
		return sockfd;
	}
	
	//avvio il 3WHS
	res = connect(sockfd, (struct sockaddr *)&server, len);
	if (res != 0)
	{
		perror("connect() error: ");
		close(sockfd);
		return res;
	}
	
return sockfd;
}



/**
@brief Funzione che implementa l'<em>half-close</em> di una connessione TCP.
@param sockfd, sockfd relativo alla connessione TCP da terminare.
@return nulla.
*/
void shutConnection(int sockfd){
	char msg[BUFSIZE] = "";
	ssize_t numbytes = 0;
	int quit = 0;
	
	
	printf("Half-Close del socket... prepare for only reading\n");
	
	/*
	Terminato l'invio, 
	effettua l'half-close della connessione in scrittura e attende una risposta.
	*/
	shutdown(sockfd, SHUT_WR);
	
	while (!quit)
	{
		numbytes = recv(sockfd, msg, BUFSIZE-1, 0);
		if (numbytes > 0)
		{
			msg[numbytes] = 0;	
			printf("Received Echo message: '%s'\n", msg);
		}
		else if (numbytes == 0)
		{
			printf("Server closed connection ..\n");
			quit = 1;
		}
		else
		{
			perror("recv() error: ");
		}
	}//wend
	
	/*
	quindi chiude il socket anche in lettura.
	*/
	shutdown(sockfd, SHUT_RD);

}



int main(int argc, char *argv[]){

	if ( argc != 3)
	{
		printf("Usage: %s <servername> <option>\n", argv[0]);
		printf("\twith option=1 to close the connection\n\toption=2 to half-close the connection\n");
		return 0;
	}
	
	int halfClose = 0;
	
	if (atoi(argv[2]) == 2) {
		halfClose = 1;
	}
	
	
	int sockfd = activeOpen(argv[1]);
	if (sockfd == -1 ){
		printf("Connection failed\n");
		exit(1);
	}
	
	int myPort = printMyAddress(sockfd);
	
	
	//char msg[5000] = "";
	//memset(msg, 'A', 5000);
	
	char msg[BUFSIZE] = ""; 
	ssize_t numbytes = 0;
	
	
	/*
	Stiamo simulando un'applicazione che implementa un client TCP il quale,
	dopo aver trasmesso i suoi dati al server, chiude la connessione.
	*/
	int i = 0;
	while ( i < 20)
	{
		snprintf(msg, BUFSIZE-1, "Shut Client %d - Hello %d", myPort, ++i);
		numbytes = send(sockfd, msg, strlen(msg), 0);
		
		if (numbytes == -1)
		{
			perror("send() error: ");
			close(sockfd);
			return FAILURE;
		}
		else
		{
			printf("Sent message %d\n", i);
		}
	
	}//wend
	
	
	/*
	numbytes = send(sockfd, msg, 5000, 0);
	
	if (numbytes == -1)
	{
		perror("send() error: ");
		close(sockfd);
		return FAILURE;
	}
	else
	{
		printf("Sent %d bytes message\n", (int)numbytes);
	}
	
	numbytes = recv(sockfd, msg, 4999, 0);
	if (numbytes > 0){
		printf("Received response %d bytes\n", (int)numbytes);
	} else if (numbytes == 0) {
		printf("connection closed by server\n");
	} else {
		perror("recv error:");
	}
	*/
	
	
	/*
	Terminato l'invio, 
	effettua l'half-close della connessione in scrittura e attende una risposta.
	*/
	if (halfClose){
		shutConnection(sockfd);
	}
	
	
	/*
	quindi dealloco le strutture interne al kernel
	*/
	close(sockfd);
	
return 0;
}

/** @} */
