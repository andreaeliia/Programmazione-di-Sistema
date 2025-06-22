#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(void) {
    struct sockaddr_in server_addr;
    
    // TODO: Configurare per IPv4
    server_addr.sin_family = AF_WANPIPE ;
    
    // TODO: Porta 9999 in formato rete
    server_addr.sin_port = htons(9999);
    
    // TODO: Accetta da qualsiasi IP
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    printf("Indirizzo configurato!\n");
    printf("Famiglia: %d\n", server_addr.sin_family);
    printf("Porta: %d\n", ntohs(server_addr.sin_port));
    
    return 0;
}