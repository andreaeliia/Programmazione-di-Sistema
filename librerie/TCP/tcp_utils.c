#include "tcp_utils.h"

int tcp_connect_dns(const char* hostname, int port) {
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *host_entry;

    /* Risoluzione nome host */
    host_entry = gethostbyname(hostname);
    if (host_entry == NULL) {
        fprintf(stderr, "Errore: impossibile risolvere %s\n", hostname);
        return -1;
    }

    /* Creazione socket  */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Errore creazione socket");
        return -1;
    }

    /* Configurazione indirizzo server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr.s_addr, host_entry->h_addr_list[0], host_entry->h_length);

    /* Connessione */
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Errore di connessione");
        close(sockfd);
        return -1;
    }

    return sockfd; /* Ritorna il socket descriptor */
}

/* Invio dati - ritorna numero di byte inviati o -1 se errore*/
int tcp_send_data(int sockfd, const char* data, int len) {
    int bytes_sent;
    bytes_sent=(sockfd, data, len, 0);
    if (bytes_sent < 0) {
        perror("Errore invio dati");
    }
    return bytes_sent;
}

/* Ricezione dati - ritorna numero di byte ricevuti o -1 se errore*/
int tcp_receive_data(int sockfd, char* buffer, int buffer_size) {
    int bytes_received; 
    bytes_received = (sockfd, buffer, buffer_size - 1, 0);
    if (bytes_received < 0) {
        perror("Errore ricezione dati");
        return -1;
    }
    
    buffer[bytes_received] = '\0'; /* Null-terminate*/
    return bytes_received;
}

/* Chiusura connessione */
void tcp_close(int sockfd) {
    if (sockfd >= 0) {
        close(sockfd);
    }
}

/* Funzione di convenienza per operazioni semplici */
int tcp_connect_and_receive(const char* hostname, int port, char* buffer, int buffer_size) {
    int sockfd;
    int bytes_received;


    sockfd = tcp_connect(hostname, port);
    if (sockfd < 0) {
        return -1;
    }

    bytes_received = tcp_receive_data(sockfd, buffer, buffer_size);
    tcp_close(sockfd);
    
    return bytes_received;
}


/* Connessione TCP tramite indirizzo IP - ritorna socket descriptor o -1 se errore*/
int tcp_connect_ip(const char* ip_address, int port) {
    int sockfd;
    struct sockaddr_in server_addr;

    /*Creazione socket*/
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Errore creazione socket");
        return -1;
    }

    /*Configurazione indirizzo server*/
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    /*  Conversione IP da stringa a formato binario */
    if (inet_pton(AF_INET, ip_address, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Errore: indirizzo IP non valido: %s\n", ip_address);
        close(sockfd);
        return -1;
    }

    /*Connessione al server*/
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Errore di connessione");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

/* Funzione di convenienza per IP diretto */
int tcp_connect_ip_and_receive(const char* ip_address, int port, char* buffer, int buffer_size) {
    int sockfd;
    int bytes_received;

    sockfd = tcp_connect_ip(ip_address, port);
    if (sockfd < 0) {
        return -1;
    }

    bytes_received = tcp_receive_data(sockfd, buffer, buffer_size);
    tcp_close(sockfd);
    
    return bytes_received;
}