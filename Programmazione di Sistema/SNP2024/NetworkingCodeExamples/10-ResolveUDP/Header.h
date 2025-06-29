/**
@addtogroup Group10
@{
*/
/**
@file 	Header.h
@author Catiuscia Melle

@brief 	Header comune al client e server dell'esempio

L'header definisce costanti comuni al client e server.
*/

#ifndef __HEADER_H__
#define __HEADER_H__

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> //resolver related functions

#include <sys/types.h> //necessario su Mac
#include <stdlib.h>

#include <ctype.h> //toupper()


#define PORTNUMBER 		49152 		/**< TCP listening port, decimal */
#define SERVICEPORT 	"49152" 	/**< TCP listening port, name */

#define BACKLOG 10 	/**< dimensione della coda di connessioni */
#define BUFSIZE 256 /**< dimensione del buffer di messaggio */
#define PORT_STRLEN 6 /**< lunghezza di una stringa rappresentativa di un indirizzo di trasporto */

#define FAILURE 3 	/**< definizione del valore di errore di ritorno del processo in caso di errori delle Sockets API */

#define INVALID 5 	/**< se i due processi non sono avviati col giusto numero di parametri */

#endif /* __HEADER_H__ */

/** @} */