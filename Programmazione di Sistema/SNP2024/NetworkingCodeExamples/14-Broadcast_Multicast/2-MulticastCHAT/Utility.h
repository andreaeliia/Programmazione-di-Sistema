/**
@addtogroup Group14
@{
*/
/**
@file 	Utility.h
@author Catiuscia Melle

@brief File che contiene le implementazioni di alcune utility e costanti comuni al sender
ed al receiver multicast.
*/


#ifndef __UTILITY_H__
#define __UTILITY_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define FAILURE -1 		/**< Valore di ritorno in caso di errore */
#define SUCCESS 3 		/**< Valore di ritorno in caso di successo */

#define BUFSIZE 512 	/**< dimensione dei messaggi */
#define FREQ 2 			/**< Frequenza (in secs) di segnalazione del sender */
#define THRESHOLD 15 	/**< Limite di messaggi multicast inviati dal sender */


#define QUIT "QuitMessage" 	/**< Messaggio di chiusura della trasmissione multicast */

#define MGROUP "239.255.255.150"	/**< indirizzo IPv4 del gruppo multicast */
#define MPORT "45678" 			/**< indirizzo UDP del gruppo multicast */


/**
@brief Funzione che invoca getnameinfo per ottenere l'indirizzo IP e la porta 
	memorizzati in una struct sockaddr
@param addr - ptr alla struct di indirizzo
@param addrlen - dimensione della struttura 
@param result - ptr alla stringa da avvalorare
@param size - dimensione della stringa
@return nulla
*/
void getAddress(struct sockaddr *addr, socklen_t addrlen, char *result, int size);



/**
@brief Funzione che legge i campi di tutte le strutture di indirizzo ritornate da getaddrinfo().

@param res, ptr alla lista concatenata di addrinfo ritornata da getaddrinfo();
@return nulla.
*/
void read_addrinfo(struct addrinfo *res);



/**
@brief	Gestione delle comunicazioni multicast
@param	sockfd - socket su cui si è eseguito il join al gruppo multicast
@return nulla
*/
void handle_multicast_session(int sockfd);

/**
@brief Funzione che invia messaggi al gruppo multicast
@param sockfd - socket da cui eseguire l'invio
@param msg - messaggio da inviare
@param MulticastDest - ptr ad una struct sockaddr che contiene l'indirizzo multicast
@param destLen, dimensione della struttura di indirizzo
@return nulla

*/
void emitMulticast(int sockfd, char *msg, struct sockaddr *MulticastDest, socklen_t destLen);



/**
@brief Funzione di confronto tra due stringhe
@param msg - stringa da confrontare col contenuto di QUIT
@return booleano, true se le due stringhe coincidono.
*/
bool isMsgQuit(char *msg);




/**
@brief Funzione per effettuare il join ad un gruppo multicast.
@param sockfd, socket descriptor per cui abilitare la ricezione multicast
@param multicastAddr, socket address che specifica l'indirizzo del gruppo multicast 
@param addrlen, dimensione del socket address 
@return intero, stato dell'esecuzione

Attenzione: la gestione delle opzioni multicast è Protocol-Dependent
*/
int join_group(int sockfd, struct sockaddr *multicastAddr, socklen_t addrlen);



/**
@brief Funzione per esplicitare il leave da un gruppo multicast.
@param sockfd, socket descriptor per cui interrompere la ricezione multicast
@param multicastAddr, socket address contenente l'indirizzo multicast da abbandonare
@param addrlen, dimensione del socket address 
@return intero, valore di ritorno della funzione setsockopt() dopo l'esecuzione del LEAVE.
*/
int leave_group(int sockfd, struct sockaddr *multicastAddr, socklen_t addrlen);


/**
@brief Specifica il TTL per i pacchetti multicast.
@param sockfd - socket per cui settare il TTL
@param TTL - valore da settare;
@return intero, stato dell'esecuzione.
 
L'opzione TTL richiede codice address-family-specific perché:
- IPv6 richiede l'opzione IPV6_MULTICAST_HOPS di tipo <em>int</em>;
- IPv4 richiede l'opzione IP_MULTICAST_TTL di tipo <em>u_char</em>;
*/
int setTTL(int sockfd, int TTL, int family);



/**
@brief Funzione per disabilitare la ricezione multicast sul nodo
@param family - AF domain;
@param sockfd - socket su cui agire
@return valore di ritorno di setsockopt().

*/
int disableLoop(int family, int sockfd);


/**
@brief Funzione che produce un output in rosso
@param text, messaggio da visualizzare.
@param form, indirizzo del mittente il messaggio.
@return nulla.
*/
void color_print(char *msg, char *from);

#endif
/** @} */
