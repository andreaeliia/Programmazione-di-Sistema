/**
@addtogroup Group7 
@{
*/
/**
@file 	Header.h
@author Catiuscia Melle

@brief 	Definizione di un header comune al client e server dell'esempio.
*/

#ifndef __HEADER_H__
#define __HEADER_H__

#include <stdio.h>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <errno.h>


#define FAILURE 3 		/**< definizione del valore di errore di ritorno del processo 
						 in caso di errori delle Socket APIs */
//#define PORT 4915 		/**< UDP server port */
#define PORT 49152 		/**< UDP server port */

#define COUNT 10 		/**< Dimensione della sequenza di messaggi inviati dal client */

#define BUFSIZE 256 	/**< Dimensione del buffer applicativo */

#define CONNECTMSG "Connect to me" /**< messaggio che richiede connessione del socket UDP */

#define DISCONNECTMSG "Disconnect to me" /**< messaggio di disconnessione del socket UDP */

#endif /* __HEADER_H__ */

/** @} */
