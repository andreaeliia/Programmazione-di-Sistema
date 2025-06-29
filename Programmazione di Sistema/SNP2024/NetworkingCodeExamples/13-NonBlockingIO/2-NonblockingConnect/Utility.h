/**
@addtogroup Group31
@{
*/
/**
@file 	Utility.h
@author Catiuscia Melle

@brief 	Funzioni di utilità condivise tra server e client
*/


#ifndef __UTILITY_H__
#define __UTILITY_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>

#include <errno.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <netdb.h>

#define FAILURE 1 /**< valore di ritorno in caso di errore */
#define SUCCESS 0 /**< valore di ritorno in caso di successo */

#define SRV_PORT "34567" /**< TCP server listening port */
#define BACKLOG 10 /**< dimensione della coda delle connessioni TCP del server*/

#define BUFSIZE 512 /**< application receive buffer */


/**
@brief Imposta la modalità non-blocking del socket descriptor 
		in input e ritorna un intero che indica successo o failure dell'operazione.
@param sockfd - socket descriptor su cui agire
@return intero, stato dell'esecuzione				
*/
int set_nonblock(int sockfd);



/**
@brief Ripristina la modalità blocking del socket descriptor in 
		input e ritorna un intero che indica successo o failure dell'operazione.

@param sockfd - socket descriptor su cui agire
@return intero, stato dell'esecuzione		
*/
int set_block(int sockfd);


/**
@brief Funzione per la visualizzazione dei dati del client connesso 
		(col socket descriptor sockfd).

@param sockfd - client socket connesso
@return nulla.
*/
void client_data(int sockfd);


/**
@brief Funzione che esegue la scrittura sul socket fino al totale invio di tutto il messaggio.

@param fd - socket descriptor su cui eseguire la scrittura
@param buf - buffer da cui prelevare i dati da inviare
@param count - lunghezza totale del messaggio da inviare

@return il numero di bytes ancora da inviare
*/
ssize_t FullWrite(int fd, const void *buf, size_t count);



/** 
@brief Funzione di lettura dei dati ritornati da getaddrinfo()
@param res - ptr alla linked list di risultati
@return nulla.
*/
void read_addrinfo(struct addrinfo *res);

#endif

/**@}*/