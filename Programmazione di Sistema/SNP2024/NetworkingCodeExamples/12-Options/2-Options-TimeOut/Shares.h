/**
@addtogroup Group12 
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
#include <time.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define PORTNUMBER 		49152 		/**< well-known UDP port del server, decimal */
#define SERVICEPORT 	"49152" 	/**< well-known UDP port del server */

#define PORT_STRLEN 6 /**< lunghezza di una stringa rappresentativa di un indirizzo di trasporto */


#define FAILURE 1 					/**< Valore di ritorno in caso di errore  */

#define MICRO_FACTOR 1000000 		/**< fattore di conversione da microsecondi 
										a secondi e viceversa */

#define BUFSIZE 512 				/**< dimensione massima di un application message */

#define JOKE "Are you kidding me?" 	/**< special application message */


#endif /* __SHARES_H__ */

/**@}*/
