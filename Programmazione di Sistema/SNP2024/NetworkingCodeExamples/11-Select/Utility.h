/**
@addtogroup Group11
@{
*/
/**
@file 	Utility.h
@author Catiuscia Melle

@brief 	Interfaccia del modulo di funzioni di utilità.

*/

#ifndef __UTILITY_H__
#define __UTILITY_H__


#include "Header.h"


/**
@brief Utility function per la visualizzazione dell'indirizzo associato ad un socket,
protocol-independent
@param addr, ptr alla struct sockaddr da leggere
@param len, dimensione della struttura puntata da addr
@return nulla
*/
void printAddressInfo(struct sockaddr * addr, socklen_t len);

/**
@brief Utility per l'apertura di un connection socket TCP IPv6
@param family - ipv4 o ipv6 protocol family domain...
@param hostname - stringa contenente l'hostname per cui è invocato il resolver 
@param servicename - stringa contenente il service name per cui è invocato il resolver
@param type - intero pari al tipo di socket SOCK_STREAM o SOCK_DGRAM
@return intero, pari al socket descriptor creato (-1 in caso di errore).

Attenzione, restituisce sempre un socket connesso (UDP e TCP)
*/
int open_socket(int family, char *hostname, char *servicename, int type);


/**
@brief Legge il valore di param e ritorna l'Address Family corrispondente
@param param, contiene il valore di argv[] corrispondente al dominio inserito.
@return il valore di Address Family corrispondente al dominio specificato
*/
int getFamily(char *param);








/**
@brief Funzione per la hostname resolution che ritorna il socket descriptor per un server.

@param nodename -  hostname da risolvere
@param servicename - indirizzo del servizio
@param socktype - tipo di sock (SOCK_STREAM, SOCK_DGRAM)
@return intero pari al socket descriptor allocato o -1 in caso di errore

*/
int passiveOpen(int family, char *nodename, char *servicename, int socktype);

/**
@brief Funzione che gestisce l'arrivo di messaggi sul socket UDP in ascolto
@param sockU - socket UDP in ascolto
@return nulla

Effettua ricezione ed invio al client UDP. Usa <em>getnameinfo()</em> per recuperare 
l'indirizzo del client UDP. 

*/
void handleUDPMessage(int sockU);


/**
@brief Funzione che gestisce l'arrivo di nuovi messaggi su connessioni TCP.
@param sock - socket TCP della connessione client pronta.
@return intero:
-# pari a -1 in caso di errore;
-# pari a 0 se la connessione è stata chiusa;
-# pari a 1 se il messaggio è stato elaborato con successo.
*/
int handleTCPClient(int sock);

#endif
/** @} */