#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"

int server_socket;  // Socket globale condiviso

void* client_thread(void* arg) {
    int thread_id = *((int*)arg);
    char received_char;
    char dummy = 'X';
    
    for (int i = 0; i < 3; i++) {
        // TODO: Inviare carattere dummy al server
        send(server_socket, &dummy, 1, 0);
        
        // TODO: Ricevere carattere dal server
        recv(server_socket, &received_char, 1, 0);
        
        printf("Thread %d ricevuto: '%c'\n", thread_id, received_char);
        sleep(1);
    }
    
    return NULL;
}

int main(void) {
    struct sockaddr_in serv_addr;
    pthread_t thread1, thread2;
    int id1 = 1, id2 = 2;
    
    // TODO: Creare socket e connettersi (come Lezione 5)
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr);
    connect(server_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    
    printf("Connesso al server!\n");
    
    // TODO: Creare i thread
    pthread_create(&thread1, NULL, client_thread, &id1);
    pthread_create(&thread2, NULL, client_thread, &id2);
    
    // TODO: Aspettare i thread
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    close(server_socket);
    return 0;
}