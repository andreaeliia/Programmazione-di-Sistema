/** 
@defgroup Group1 Manipolazione degli indirizzi Internet

@brief Gestire le strutture di indirizzo 

In questa sezione si presentano le API per la gestione degli indirizzi IP,
sia IPv4 che IPv6, 
ad opera delle funzioni <em>inet_ntop()</em> ed <em>inet_pton()</em>,
oltre alla gestione dei numeri di porta con le funzioni ntohs() ed htons()

IP e port numbers devono essere sempre memorizzati in Network Byte Order nelle strutture di indirizzo.

In particolare, ogni indirizzo espresso in forma testuale (pari ad un 
indirizzo IPv4 dotted decimal o IPv6 esadecimale) sarà convertito in valore numerico 
in <em>Network Byte Order</em> (ovvero Big Endian) e 
memorizzato nelle rispettive strutture di indirizzo.
@{
*/
/**
@file 	SocketAddresses.c
@author Catiuscia Melle

@brief 	Esempio d'uso delle funzioni per la manipolazione di indirizzi 
		IPv4/IPv6 inet_pton() e inet_ntop() e per la gestione della endianness
*/

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>


/**
@brief Funzione che illustra come inizializzare una struct sockaddr_in con un indirizzo nel dominio IPv4.

@param ipv4, la stringa dotted decimal che contiene l'indirizzo IPv4 da convertire in binario in Network Byte Order
@param port, il valore decimale di porta di servizio, in host-byte-order, da convertire in Network Byte Order
@param sa, ptr alla struct sockaddr_in da avvalorare

@return intero, stato dell'esecuzione
*/
int initSockaddr_in(char *ipv4, in_port_t port, struct sockaddr_in *sa){
 	
	int result = 0; 			/* valore di ritorno delle Sockets API e della funzione */
 	
	/*
	inizializziamo le strutture di indirizzo
	- poiché ogni implementazione le definisce arbitrariamente -
	*/
	memset(sa, 0, sizeof(struct sockaddr_in));
	
	//avvaloriamo il campo Address Family delle strutture di indirizzo
	sa->sin_family = AF_INET;
	
 	/*
 	aggiungo alla struct sockaddr_in l'informazione sul numero di porta
 	ATTENZIONE: anche questo dato deve essere memorizzato in Network Byte Order
 	*/
 	sa->sin_port = htons(port);
	
	
 	/*
 	Converte l'indirizzo IPv4 
 	espresso in forma dotted-decimal 
 	in un intero a 32 bit (in NETWORK BYTE ORDER), 
 	e lo salva in una struttura in_addr
 	*/
 	result = inet_pton(AF_INET, ipv4, &(sa->sin_addr));
 	if (result == 1)
 	{
 		printf("L'indirizzo inserito '%s' è una stringa dotted decimale valida per l'Address Family = %d\n", ipv4, AF_INET);	
 	} 
 	else if (result == 0)
 	{
		//address not parseable in the specified address family
 		printf("La stringa '%s' non rappresenta un indirizzo IPv4 valido\n", ipv4);
 	} 
 	else 
 	{
 		//result == -1
 		perror("inet_pton() error: ");
 	}
	
	return result;
}


/**
@brief Funzione che presenta il contenuto di una struttura di indirizzo IPv4
@param sa, ptr alla struct sockaddr_in da leggere
@return nulla 
*/
void printSockaddr_in(struct sockaddr_in *sa){
 	/* 
 	stringhe in cui verranno memorizzati gli indirizzi IP convertiti in stringa
  	(/usr/include/netinet/in.h)
 	 INET_ADDRSTRLEN                 16 
 	 INET6_ADDRSTRLEN                46
 	*/
 	char ipv4[INET_ADDRSTRLEN] = {0}; 
 	const char *ptr = NULL;

 	ptr = inet_ntop(AF_INET, &(sa->sin_addr), ipv4, INET_ADDRSTRLEN);
 	
 	if (ptr != NULL)
 	{
 		printf("\nIPv4 address = '%s'\n", ipv4);
 	}
 	else
 	{
 		perror("inet_ntop() error: ");
 	}
 	
	printf("Port number = %d\n\n", ntohs(sa->sin_port));
}



/**
@brief Funzione che illustra come inizializzare una struct sockaddr_in6 con l'indirizzo di un processo remoto nel dominio IPv6.

@param ipv6, la stringa hex che contiene l'indirizzo IPv6 da convertire in binario in Network Byte Order
@param port, il valore decimale di porta di servizio, in host-byte-order, da convertire in Network Byte Order
@param sa, ptr alla struct sockaddr_in6 da avvalorare

@return intero, stato dell'esecuzione
*/
int initSockaddr_in6(char *ipv6, in_port_t port, struct sockaddr_in6 *sa){
 	
	int result = 0; 			/* valore di ritorno delle Sockets API e della funzione */
	
	/*
	inizializziamo le strutture di indirizzo
	- poiché ogni implementazione le definisce arbitrariamente -
	*/
	memset(sa, 0, sizeof(struct sockaddr_in6));
	
	//avvaloriamo il campo Address Family delle strutture di indirizzo
	sa->sin6_family = AF_INET6;

 	/*
 	aggiungo alla struct sockaddr_in6 l'informazione sul numero di porta in Network Byte Order, 
	da host-byte-order in network byte order
 	*/
 	sa->sin6_port = htons(port);
	
	/*
	Converte l'indirizzo IPv6 espresso in hex in valore numerico in NETWORK BYTE ORDER 
	e lo salva in una struct in6_addr
	*/
	result = inet_pton(AF_INET6, ipv6, &(sa->sin6_addr));
	if (result == 1)
	{
		printf("L'indirizzo inserito '%s' è una stringa esadecimale valida per l'Address Family = %d\n", ipv6, AF_INET6);
	} 
	else if (result == 0)
	{
		printf("La stringa '%s' non rappresenta un indirizzo IPv6 valido\n", ipv6);
	} 
	else 
	{
		//result == -1
		perror("inet_pton() error: ");
	}

	return result;
}


/**
@brief Funzione che presenta il contenuto di una struttura di indirizzo IPv6
@param sa, ptr alla struct sockaddr_in6 da leggere
@return nulla 
*/
void printSockaddr_in6(struct sockaddr_in6 *sa){

	/* 
	stringhe in cui verranno memorizzati gli indirizzi IP convertiti in stringa
	(/usr/include/netinet/in.h)
	INET_ADDRSTRLEN                 16 
	INET6_ADDRSTRLEN                46
	*/

	char ipv6[INET6_ADDRSTRLEN] = {0};
	const char *ptr = NULL;
	
	ptr = inet_ntop(AF_INET6, &(sa->sin6_addr), ipv6, INET6_ADDRSTRLEN);
	if (ptr != NULL)
	{
		printf("\nIPv6 address = '%s'\n", ipv6);
	}
	else
	{
		perror("inet_ntop() error: ");
	}
	
	//attenzione:
	printf("la porta in network byte order è %d\n", (int)sa->sin6_port);
	printf("Port number = %d\n\n", ntohs(sa->sin6_port));
}



int main(){
	
	printf("\n\t\tIntroduzione alle funzioni di conversione degli indirizzi IPv4 ed IPv6\n\n");
	
	//indirizzi da convertire nella rappresentazione binaria in Network Byte Order
	char ipv4_str[] 		= "192.168.15.43"; 			/* IPv4 dotted-decimale */
	char ipv6_str[] 		= "2001:db8:63b3:1::3490"; 	/* IPv6 hex */
	char ipv4mapped_str[] 	= "::ffff:10.0.12.250"; 	/* IPv4-mapped-IPv6 */
	
	in_port_t portAddress = 33567; 						/* valore di porta TCP/UDP */

	struct sockaddr_in sa4;	 		/* struttura di indirizzo IPv4 */
	struct sockaddr_in6 sa6; 		/* struttura di indirizzo IPv6 */
	struct sockaddr_in6 samapped; 	/* struttura di indirizzo IPv6 */
	
	int result = 0; 			/* valore di ritorno delle Sockets API */
	
	//Conversione di un indirizzo IPv4
	result = initSockaddr_in(ipv4_str, portAddress, &sa4);
	if (result == 1) {
		printf("Struttura di indirizzo correttamente inizializzata...\nconversione inversa...\n");
		printSockaddr_in(&sa4);
	}
	
	//Conversione di un indirizzo IPv6 
	result = initSockaddr_in6(ipv6_str, portAddress, &sa6);
	if (result == 1) {
		printf("Struttura di indirizzo correttamente inizializzata...\nconversione inversa...\n");
		printSockaddr_in6(&sa6);
	}
	
	//Conversione di un indirizzo IPv4-mappato-IPv6 
	result = initSockaddr_in6(ipv4mapped_str, portAddress, &samapped);
	if (result == 1) {
		printf("Struttura di indirizzo correttamente inizializzata...\nconversione inversa...\n");
		printSockaddr_in6(&samapped);
	}

return 0;
}

/** @} */ 
