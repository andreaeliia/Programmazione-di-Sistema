#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080

int main(void) {
    int server_fd;
    struct sockaddr_in address;
    
    // 1. Creare socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        err_sys("socket failed");
    }
    
    // 2. Configurare indirizzo
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // 3. TODO: Fare bind del socket all'indirizzo
    if (bind(server_fd, (struct sockaddr_in*)&address,sizeof(address)) < 0) {
        err_sys("bind failed");
    }
    
    // 4. TODO: Iniziare ad ascoltare (max 3 in coda)
    if (listen(server_fd, 3) < 0) {
        err_sys("listen failed");
    }
    
    printf("Server in ascolto su porta %d\n", PORT);
    printf("Premi Ctrl+C per fermare\n");
    
    // Aspetta indefinitamente
    while(1) {
        sleep(1);
    }
    
    close(server_fd);
    return 0;
}