/**
@defgroup Group8 Risoluzione degli indirizzi Internet

@brief Risoluzione degli indirizzi Internet 

In 	questo modulo si mostra l'hostname e service name resolution per mezzo di <em>getaddrinfo()</em>.
@{
*/
/**
@file 	HostnameResolution.c
@author Catiuscia Melle
@brief 	Studio della funzione di risoluzione degli hostname in indirizzi IPv4/IPv6.

Dato un hostname in input, l'applicazione invoca il resolver e stampa i risultati ottenuti.
*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#define FAILURE 1 	/**< Valore di ritorno del processo in caso di insuccesso */
#define ERROR 2 	/**< Valore di ritorno del processo in caso di errore */



int main(int argc, char *argv[]){

	/*
	Controllo degli input: 
	l'applicazione richiede un parametro di input, pari all'hostname da risolvere
	*/
	if (argc != 3) 
	{
	    fprintf(stdout, "Usage: %s <hostname> [<service name>]\n", argv[0]);
	    return FAILURE;
	} 

	
	printf("\tHostname Resolution for %s:%s\n", argv[1], argv[2]);
	
	int g_err; //valore di ritorno della getaddrinfo
	
	char ipstr[INET6_ADDRSTRLEN]; //stringa atta a contenere un indirizzo IPv4 e/o IPv6
	
	/*
	hints contiene i dati di input per il resolver 
	(suggerimenti per le operazioni di risoluzione)
	*/
	struct addrinfo hints; 
	
	/*
	value-result-argument della getaddrinfo(): 
	punta ad una lista concatenata di struct addrinfo in cui ognuna contiene un indirizzo
	valido (risultato della resolution)
	*/
	struct addrinfo *res; 
	struct addrinfo *p; //ptr addizionale
	
	
	
	//azzeriamo la struttura hints
	memset(&hints, 0, sizeof hints);
	
	//specifichiamo i suggerimenti per il resolver
	hints.ai_family = AF_UNSPEC; 	//sia struct sockaddr_in che sockaddr_in6
	
	//hints.ai_family = AF_INET; 	//solo struct sockaddr_in 
	//hints.ai_family = AF_INET6; 	//solo struct sockaddr_in6
	hints.ai_socktype = 0; //sia TCP che UDP service

	
	//solo se specifico AF_INET6, questo flag è preso in carica
	//e restituisce gli indirizzi IPv4 mappati IPv6 per l'hostname indicato
	hints.ai_flags |= AI_V4MAPPED; 
	
	//invochiamo l'hostname resolution per il valore passato in input
	g_err = getaddrinfo(argv[1], argv[2], &hints, &res);
	if (g_err != 0) 
	{
		fprintf(stdout, "getaddrinfo Error (%d): %s\n", g_err, gai_strerror(g_err)); 
		return ERROR;
	}


	/*
	scorriamo la linked list per leggere tutti i risultati ottenuti:
	*/
	for (p = res; p != NULL; p = p->ai_next) 
	{
		printf("AF=%d, sock type=%s\n", p->ai_family, (p->ai_socktype == SOCK_STREAM) ? "TCP":"UDP");
		/*
		per ogni elemento della lista
		il campo ai_family identifica l'Address Family dell'indirizzo
		Questo ci consente di specializzare l'elaborazione
		*/
		if (p->ai_family == AF_INET) 
		{ 
			/* IPv4
			eseguo il cast della struct sockaddr puntata da p->ai_addr
			ad una struct sockaddr_in per poterne interpretare i campi di indirizzo IPv4 e porta
			*/
			struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
			
			printf("\tIPv4 Address: %s:%d\n",
							inet_ntop(p->ai_family, &(ipv4->sin_addr), ipstr, sizeof(ipstr)), ntohs(ipv4->sin_port) );
		} 
		else 
		{ 
			/* IPv6
			eseguo il cast della struct sockaddr puntata da p->ai_addr
			ad una struct sockaddr_in6 per poterne interpretare i campi di indirizzo IPv6 e porta
			*/
			struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
			printf("\tIPv6 Address: %s:%d\n", 
						inet_ntop(p->ai_family, &(ipv6->sin6_addr), ipstr, sizeof(ipstr)), 
						ntohs(ipv6->sin6_port));
		}
	}//for

	
	/*
	 free the linked list
	Questa funzione deve essere sempre richiamata per poter deallocare le strutture dati
	usate da getaddrinfo()
	*/
	freeaddrinfo(res); 

return 0;
}

/** @} */