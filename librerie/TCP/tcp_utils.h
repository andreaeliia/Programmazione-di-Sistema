#ifndef TCP_UTILS_H
#define TCP_UTILS_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>  // Per inet_addr() e inet_pton()
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_BUFFER_SIZE 1024

// Funzioni di connessione
int tcp_connect_dns(const char* hostname, int port);           // Connessione via hostname (DNS)
int tcp_connect_ip(const char* ip_address, int port);      // Connessione via IP diretto

// Funzioni per gestione dati
int tcp_send_data(int sockfd, const char* data, int len);
int tcp_receive_data(int sockfd, char* buffer, int buffer_size);
void tcp_close(int sockfd);

// Funzioni di convenienza
int tcp_connect_and_receive(const char* hostname, int port, char* buffer, int buffer_size);
int tcp_connect_ip_and_receive(const char* ip_address, int port, char* buffer, int buffer_size);

#endif