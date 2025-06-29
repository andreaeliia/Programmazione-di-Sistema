/**
@addtogroup Group14
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
#include <stdbool.h>

#define PORTNUMBER 		49152 		/**< TCP listening port, decimal */
#define SERVICEPORT 	"49152" 	/**< TCP listening port, name */
#define PORT_STRLEN 6 /**< lunghezza di una stringa rappresentativa di un indirizzo di trasporto */


#define FREQ 3 			/**< Frequenza (in secs) di segnalazione del sender */
#define THRESHOLD 15 	/**< Limite di messaggi broadcast inviati dal sender */

#define QUIT "QuitMessage" 	/**< Messaggio di chiusura della trasmissione multicast */


#define BACKLOG 10 	/**< dimensione della coda di connessioni */

#define SIZE 15 /**< dimensione della coda di connessioni concorrenti gestite dal server */

#define BUFSIZE 512 /**< dimensione del buffer di messaggio */

#define FAILURE 3 	/**< definizione del valore di errore di ritorno del processo in caso di errori delle Sockets API */

#define INVALID 5 	/**< se i due processi non sono avviati col giusto numero di parametri */

#define ERROR 1 /**<  valore di errore */

#define INSUCCESS -1 /**< valore di ritorno non valido */


#endif /* __HEADER_H__ */

/** @} */