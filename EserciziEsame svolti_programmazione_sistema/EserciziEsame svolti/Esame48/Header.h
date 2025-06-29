/**
@addtogroup Group2
@{
*/
/**
@file 	Header.h
@author Catiuscia Melle

@brief 	Header comune al client e server dell'esempio

*/

#ifndef __HEADER_H__
#define __HEADER_H__

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORTA 49152 	/**< TCP listening port */
#define PORTB 49153
#define BACKLOG 10 	/**< dimensione della coda di connessioni */

#define BUFSIZE 512 /**< dimensione del buffer di messaggio */

#define FAILURE 3 	/**< definizione del valore di errore di ritorno del processo 
						in caso di errori delle Sockets API */



#endif /* __HEADER_H__ */

/** @} */
