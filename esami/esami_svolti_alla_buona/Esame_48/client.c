/* client.c - Client di test */
#include "shared_mem.h"

/* Crea connessione TCP al server */
int connect_to_server(int port) {
    int sockfd;
    struct sockaddr_in server_addr;
    
    /* Crea socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        err_sys("socket error");
    }
    
    /* Configura indirizzo server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        err_sys("inet_pton error");
    }
    
    /* Connetti */
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        err_sys("connect error");
    }
    
    return sockfd;
}

/* Invia messaggio al server */
void send_message(int sockfd, const char* message) {
    char buffer[MAX_STRING_LEN];
    
    snprintf(buffer, sizeof(buffer), "%s\n", message);
    
    if (write(sockfd, buffer, strlen(buffer)) == -1) {
        err_sys("write error");
    }
}

/* Funzione principale */
int main(int argc, char *argv[]) {
    int sockfd;
    int port;
    char message[MAX_STRING_LEN];
    
    /* Verifica argomenti */
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <porta>\n", argv[0]);
        fprintf(stderr, "Porte disponibili: %d, %d\n", PORT1, PORT2);
        exit(1);
    }
    
    port = atoi(argv[1]);
    if (port != PORT1 && port != PORT2) {
        fprintf(stderr, "Porta non valida. Usa %d o %d\n", PORT1, PORT2);
        exit(1);
    }
    
    /* Connetti al server */
    sockfd = connect_to_server(port);
    printf("Connesso al server sulla porta %d\n", port);
    printf("Inserisci messaggi (CTRL+D per terminare):\n");
    
    /* Loop input utente */
    while (fgets(message, sizeof(message), stdin) != NULL) {
        /* Rimuovi newline se presente */
        message[strcspn(message, "\n")] = '\0';
        
        /* Invia messaggio */
        send_message(sockfd, message);
        printf("Messaggio inviato: %s\n", message);
    }
    
    close(sockfd);
    printf("Connessione chiusa\n");
    
    return 0;
}