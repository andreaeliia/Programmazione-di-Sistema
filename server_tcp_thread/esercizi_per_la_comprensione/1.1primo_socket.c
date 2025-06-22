#include "apue.h"
#include <sys/socket.h>

int main(void) {
    int my_socket;
    
    // TODO: Creare un socket TCP IPv4
    my_socket = socket(AF_INET, SOCK_STREAM, 0);
    
    if (my_socket == -1) {
        err_sys("Errore creazione socket");
    }
    
    printf("Ho creato il socket numero: %d\n", my_socket);
    
    // TODO: Chiudere il socket
    close(my_socket);
    
    return 0;
}