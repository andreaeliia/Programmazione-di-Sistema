/**
@addtogroup Group41
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

#endif

/** @} */
