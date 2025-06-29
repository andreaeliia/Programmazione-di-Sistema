/**
@addtogroup Group11 
@{
*/
/**
@file 	FakeClient.c
@author Catiuscia Melle

@brief TCP Client che legge da stdin ed invia i messaggi al server.
	
Questo client fa due invii in cascata al server, senza interporre delle recv().		
Cosa succede se il client/sender non si accorge che il server/receiver 
ha chiuso la connessione? Il client scrive su una "broken pipe".

In questo esempio il client (sender peer) - poiché è bloccato nell'attesa che l'utente 
inserisca una linea di testo da stdin - non è in grado di rilevare la chiusura 
della connessione TCP del server (receiver peer) nel momento in cui arriva il segmento FIN.
-- La ricezione del FIN viene persa perché il client non effettua delle recv() sul socket --
-- RST e FIN sono consegnati nella RecvQ del socket connesso, per cui se l'applicazione 
effettua solo send() non è in grado di riceverli --

Ne segue che quando il client invia il primo messaggio sul socket, un segmento TCP RST viene 
ricevuto in risposta. 
Anche in questo caso, se il client non effettua recv() sul socket, o se il client non 
monitora gli errori pendenti sul socket, non è in grado di rilevare il 
<em>connection reset</em> del server.
In questo modo, se il client invia un secondo messaggio, un segnale SIGPIPE è generato 
e ciò determina la chiusura del client.

La gestione del SIGPIPE può essere intercettata (signal handler) per avviare 
le azioni opportune. 

*/


#include "Header.h"
#include "Utility.h"


/**
@brief Utility function
@param name, nome dell'eseguibile
*/
void usage(char *name){
	printf("Usage: %s <servername> <domain>\n", name);
	printf("\tdomain= 0 (UNSPEC), 4(INET), 6(INET6)\n"); 
}


/**
@brief Handler del segnale SIGPIPE: da specializzare
@param signalType - intero che rappresenta il signal number
@return nulla
*/
void SigHandler(int signalType){
	
	printf("\n\tRaised signal number: %d\n", signalType);
	
	if (signalType == SIGPIPE)
	{
		printf("\tSIGPIPE: Broken pipe error\n");
	 	exit(1);
	}
}




/**
@brief Implementa il duplice invio di un messaggio al server dal socket connesso.
@param sockfd, il socket TCP connesso
@return nulla

Eseguendo 2 send() in cascata, 
- senza interporvi una recv() 
- o senza verificare gli errori pendenti sul socket, 
- o senza l'utilizzo di una select(), 
il processo non è in grado di rilevare la "broken pipe" che genera il raise del SIGPIPE. 
*/
void fakeSender(int sockfd){
	
	char readline[BUFSIZE] = "";
	char sendline[BUFSIZE] = "";
	char replica[BUFSIZE] = "Secondo messaggio inviato back-to-back";
	
	ssize_t numbytes = 0;
	
	int rv = 0;
	
	int errCode = 0; //option value for the pending socket error
	socklen_t errlen = sizeof(errCode); //option len for the SO_ERROR option
		
	
	
	printf("Insert the message to send\n");
	
	while ( fgets(sendline, BUFSIZE, stdin) != NULL )
	{
		printf("preparing FIRST send ..");	
		
		numbytes = send(sockfd, sendline, strlen(sendline), 0);
		if (numbytes == -1)
		{
			perror("Error on sending the message: ");
 			close(sockfd);
  			exit(1);
		}
		printf(". sent %d bytes\n", (int)numbytes);
		
		/*
		Se a questo punto il peer remoto ha chiuso la connessione, un RST è già stato
		ricevuto in seguito alla prima send().
		Se faccio nuovamente una send(), non riesco a rilevare le condizioni del 
		socket.
		
		$ netstat -a -p tcp | grep 49152
		tcp4       0      0  localhost.49152        localhost.58392        FIN_WAIT_2 
		tcp4      38      0  localhost.58392        localhost.49152        CLOSE_WAIT
		
		Il primo socket (localhost.49152->localhost.58392, FIN_WAIT_2) è del server che 
		ha effettuato la active close: il socket è stato chiuso e l'ACK è stato ricevuto al FIN
		inviato: ora il socket sta aspettando di ricevere il FIN dal client.
		
		Il secondo socket (localhost.58392->localhost.49152, CLOSE_WAIT) è del client: il 
		TCP layer ha riscontrato il FIN, ma l'applicazione non si è accorta che la connessione 
		è stata chiusa.
		*/
		
		printf("sleep for 1 second ...\n");
		sleep(1);
		
		//Posso verificare gli errori pendenti sul socket prima di fare la send():
		rv = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &errCode, &errlen);
		if (rv == 0 && errCode != 0) {
			printf("\tpending error sul socket, error code %d\n", errCode);
			printf("\tthe error is %s\n", strerror(errCode));
		}
		
		
		printf("preparing SECOND send .");
		numbytes = send(sockfd, replica, strlen(replica), 0);//MSG_NOSIGNAL vs SO_NOSIGPIPE
		if (numbytes == -1)
		{
			perror("Error on sending the message: ");
			close(sockfd);
			exit(1);
		}
		printf(".. sent %d bytes\n", (int)numbytes);
		
		
		printf("Waiting echoed messages...\n");
		numbytes = recv(sockfd, readline, BUFSIZE-1, 0);
		if (numbytes > 0)
		{
		 	readline[numbytes] = '\0';
		 	printf("'%s'\n", readline);
		 	//fputs(readline, stdout); 
		 	printf("\nAgain?...[Ctrl+D per terminare]\n"); 
		 	continue;
		} 
		else if (numbytes == -1 )
		{
			printf("\nError on receiving the echo message\n");
			close(sockfd);
			exit(1);
		} 
		else 
		{//(numbytes == 0)
			printf("\nServer stopped running\n");
			close(sockfd);
			exit(1);
		}
	}//wend
}



int main(int argc, char *argv[]){

	if (argc != 3)
	{
		usage(argv[0]);
		return INVALID;
	}
	
	int family = getFamily(argv[2]); 
	int type = SOCK_STREAM;


	//open a TCP socket
	int sockfd = open_socket(family, argv[1], SERVICEPORT, type);
	if (sockfd == INSUCCESS)
	{
		printf("Errore nell'aprire il socket e stabilire la connessione al server TCP\n");
		return ERROR;
	}

	
	// Signal handler specification structure
	struct sigaction handler;
	
	// Set handler function
	handler.sa_handler = SigHandler;
	
	// Create mask that blocks all signals
	if (sigfillset(&handler.sa_mask) < 0)
	{
		perror("sigfillset() failed: ");
		close(sockfd);
		exit(1);
	}
	
	handler.sa_flags = 0; // No flags
	
	// Set signal handling for interrupt signal
	if (sigaction(SIGPIPE, &handler, 0) < 0)
	{
		perror("sigaction() failed for SIGPIPE: ");
		close(sockfd);
		exit(1);
	}
		
	fakeSender(sockfd);
	
	close(sockfd);

return 0;
}

/** @} */
