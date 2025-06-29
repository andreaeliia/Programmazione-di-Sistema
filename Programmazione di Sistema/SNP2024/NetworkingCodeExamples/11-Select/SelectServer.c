/**
@defgroup Group11 I/O multiplexing with BSD Sockets

@brief Utilizzo di I/O multiplexing con i socket
@{
*/
/**
@file 	SelectServer.c
@author Catiuscia Melle

@brief 	Presentazione di un Server che implementa I/O multiplexing, in grado di monitorare "contemporaneamente":
		-# listening TCP sockets 
		-# accepted TCP sockets (established connections)
		-# UDP sockets

Il server aggiunge il socket connesso TCP al set di descriptor da monitorare.
Il codice è protocol-indipendent (utilizzo di <em>getnameinfo()</em> per la lettura dei dati del client da una <em>struct sockaddr_storage</em>). 

*/

#include "Header.h"
#include "Utility.h"

/**
variabili globale client sockets array
rappresenta l'array di socket descriptor connessi ai client TCP
*/
int conn_set[SIZE]; 



/**
@brief aggiunge la nuova connessione all'array di client connessi
@param sockfd, il socket della nuova connessione
@param conn_set, ptr all'array delle connessioni
@param dim, dimensione dell'array
@return true se la nuova connessione è stata registrata, false se la coda è piena
*/
bool add_client(int sockfd, int *conn_set, int dim){
	
	int i = 0;
	bool is_registered = false;
	for (i = 0; i < dim; i++)
	{
		if (conn_set[i] == -1)
		{
			conn_set[i] = sockfd;
			is_registered = true;
			break;
		}
	}

	return is_registered;
}


/**
@brief Funzione che gestisce l'arrivo di nuove connessioni sul listening socket TCP.
@param sockT - listening TCP socket del server
@return intero:
-# pari a -1 se la connessione è stata rifiutata.
-# pari al socket descriptor della nuova connessione (se correttamente inserita in elenco)
*/
int handleNewConn(int sockT){

	struct sockaddr_storage client; 
	socklen_t sslen = sizeof(client);

	int conn = accept(sockT, (struct sockaddr *)&client, &sslen);
	if (conn == -1)
	{
		perror("accept() error: ");
		return -1;
	}

	printf("TCP listening socket accepted new connection from ");
	printAddressInfo((struct sockaddr *)&client, sslen);
	printf("\n");

	bool is_added = add_client(conn, conn_set, SIZE);
	if (is_added == false)
	{
		printf("max client queue\n");
		close(conn);
		return -1;
	}

return conn;
}


/** 
@brief Inizializza l'array dei socket connessi
@param conn_set, ptr all'arrayd di interi
@param dim, dimensione dell'array
*/
void setConnSet(int *conn_set, int dim){

	int i = 0;
	for (i = 0; i < dim; i++)
	{
		conn_set[i] = -1;
	}
}


/**
@brief Funzione di utilità
@param name, nome dell'eseguibile
*/
void usage(char *name){
	printf("Usage: %s <domain>\n", name);
	printf("\tdomain: 0 (UNSPEC), 4 (INET), 6 (INET6)\n");
} 



int main(int argc, char *argv[]){

	if (argc != 2) 
	{
		usage(argv[0]);
		return INVALID;
	}
	
	int family = getFamily(argv[1]);
	
	
	int sockT = 0; //TCP listening socket
	int sockU = 0; //UDP listening socket
	
	sockT = passiveOpen(family, NULL, SERVICEPORT, SOCK_STREAM); 
	sockU = passiveOpen(family, NULL, SERVICEPORT, SOCK_DGRAM); 
	
	
	if ((sockT == -1) || (sockU == -1))
	{
		printf("Errore nell'avviare i socket\n");
		return ERROR;
	} 
	
	/* su Mac OS X: 
	$ netstat -a | grep 49152
	tcp4       0      0  *.49152                *.*                    LISTEN     
	udp4       0      0  *.49152                *.*                            
	*/
	printf("TCP Server and UDP server running on port %s\n", SERVICEPORT);

	//inizializzo l'array
	setConnSet(conn_set, SIZE); 

	//handle delle nuove connessioni (socket connessi)
	int conn = 0; 

	//protocol-independent management of connections
	struct sockaddr_storage client; 
	socklen_t sslen = sizeof(client);


	/*
	definiamo il set dei file descriptor da monitorare:
	il server sarà in attesa di:
	- connessioni 
	- messaggi, 
	- e tutte condizioni monitorabili in lettura
	*/

	//dichiaro 2 read file descriptor set
	fd_set rset, allset; 

	FD_ZERO(&rset); //azzero set descrittori
	FD_ZERO(&allset); //azzero set descrittori copia

	FD_SET(sockT, &rset); //aggiungo socket TCP
	FD_SET(sockU, &rset); //aggiungo socket UDP

	//prendo il max tra i 2, più 1
	int max = (sockT > sockU)?sockT+1: sockU+1; 

	//duplico il set - allset contiene sempre i 2 socket TCP ed UDP
	allset = rset; 
	
	int result = 0;
	int quit = 0;
	while (!quit)
	{
		/*
		rfds è un value-result argument, modificato da select():
		dobbiamo re-inizializzarlo
		*/
		FD_ZERO(&rset);
		rset = allset; 

		//wait for select forever
		result = select(max, &rset, NULL, NULL, NULL);

		if (result == -1)
		{
			perror("select() error: ");
			continue;
		}
		else if (result == 0)
		{ 
			//timeout su select (non specificato, non dovrei mai arrivarci)
			continue;	
		} 
		else 
		{
			//result è pari al numero di descriptor pronti in lettura...
			printf("%d socket ready for reading\n", result);

			//verifichiamo chi ha dati da leggere...
			if ( FD_ISSET(sockU, &rset))
			{
				printf("Received a datagram...\n");
				handleUDPMessage(sockU);
				//printf("decrement result...\n");
				result--;
			}

			if ( FD_ISSET(sockT, &rset))
			{
				//new TCP Connection 
				conn = handleNewConn(sockT);
			
				if (conn != -1)
				{ 
					//printf("add new conn to allset\n");
					FD_SET(conn, &allset);
				
					if ( (conn + 1) > max)
						max = conn+1;
				}
				result--;
			}

			if (result == 0)
			{
				//skip the rest
				continue;
			}

			//a questo punto devo controllare tutte le connessioni attive dei client
			int j = 0; //indice dell'array di connessioni TCP client
			while (result > 0)
			{
				if ( (conn_set[j] != -1)  && FD_ISSET(conn_set[j], &rset))
				{
					printf("Client Connection number %d ready for reading ...\n", j);
					int status = handleTCPClient(conn_set[j]);

					//decremento result
					result--;

					if (status == 0)
					{ 
						//client ha chiuso la connessione
						close(conn_set[j]);

						//rimuovo il socket dal set da monitorare
						FD_CLR(conn_set[j], &allset);

						//libero la sua posizione nell'array
						conn_set[j] = -1;
					}//fi status
				}//fi FD_ISSET
				
				//passo al prossimo elemento dell'array di connessioni TCP 
				j++;
			}//wend client connection

		}//fi result

	}//wend
	
	//never here
	close(sockT);
	close(sockU);
	
return 0;
}

/** @} */
