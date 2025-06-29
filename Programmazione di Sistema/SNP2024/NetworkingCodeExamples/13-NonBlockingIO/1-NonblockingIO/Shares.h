/**
@addtogroup Group30
@{
*/
/**
@file 	Shares.h
@author Catiuscia Melle

@brief 	Raccolta di info comuni al client e server UDP.

*/

#ifndef __SHARES_H__
#define __SHARES_H__


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>

#include <ctype.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <fcntl.h> //nonblocking flag

#define FAILURE 1 /**< error return value */
#define SUCCESS 0 /**< success return value */

#define NOPORT 0 /**< Valore di ritorno in caso di failure di get_port() */
#define MICRO_FACTOR 1000000 /**< fattore di conversione da microsecondi a secondi e viceversa */

#define ECHO_PORT "34567" /**< well-known UDP port del server */

#define PORT_STRLEN 6 /**< lunghezza di una stringa decimale */
#define BUFSIZE 512 /**< application buffer size */

#define JOKE "Are you kidding me?"

/**
@brief Funzione per la stampa di indirizzi sockaddr_in/in6

@param addr - ptr alla struct sockaddr generica da cui recuperare l'IP e porta
param len - dimensione di addr
@return nulla
*/
void printAddressInfo(struct sockaddr * addr, socklen_t len);



/**
@brief Imposta la modalità non-blocking del socket descriptor in input 
		e ritorna un intero che indica successo o failure dell'operazione.
		
@param sockfd - socket descriptor su cui settare la modalità bloccante.
@return stato dell'esecuzione
*/
int set_nonblock(int sockfd);


/**
@brief Ripristina la modalità blocking del socket descriptor 
		in input e ritorna un intero che indica successo o failure dell'operazione.

@param sockfd - socket descriptor su cui settare la modalità bloccante.
@return stato dell'esecuzione
*/
int set_block(int sockfd);



#endif
/**@}*/


