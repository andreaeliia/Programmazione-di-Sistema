/*
 * server.c - Server per la ricerca di file JPEG e PNG
 * 
 * Il server riceve richieste dai client e cerca ricorsivamente
 * file JPEG o PNG nella home directory dell'utente.
 */

#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define SERVER_PORT 8080
#define BUFFER_SIZE 1024
#define MAX_PATH 4096

/* Magic numbers per identificazione file */
static const unsigned char JPEG_MAGIC[] = {0xFF, 0xD8};
static const unsigned char PNG_MAGIC[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};

/*
 * Funzione per verificare se un file è di tipo JPEG
 * filepath: percorso del file da verificare
 * Ritorna 1 se è JPEG, 0 altrimenti
 */
int is_jpeg_file(const char *filepath) {
    int fd;
    unsigned char buffer[2];
    ssize_t bytes_read;
    
    fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        return 0;
    }
    
    bytes_read = read(fd, buffer, sizeof(buffer));
    close(fd);
    
    if (bytes_read != sizeof(buffer)) {
        return 0;
    }
    
    return (memcmp(buffer, JPEG_MAGIC, sizeof(JPEG_MAGIC)) == 0);
}

/*
 * Funzione per verificare se un file è di tipo PNG
 * filepath: percorso del file da verificare
 * Ritorna 1 se è PNG, 0 altrimenti
 */
int is_png_file(const char *filepath) {
    int fd;
    unsigned char buffer[8];
    ssize_t bytes_read;
    
    fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        return 0;
    }
    
    bytes_read = read(fd, buffer, sizeof(buffer));
    close(fd);
    
    if (bytes_read != sizeof(buffer)) {
        return 0;
    }
    
    return (memcmp(buffer, PNG_MAGIC, sizeof(PNG_MAGIC)) == 0);
}

/*
 * Funzione ricorsiva per la ricerca di file
 * dirpath: percorso della directory da esplorare
 * file_type: tipo di file da cercare ("JPEG" o "PNG")
 * client_fd: socket del client per inviare i risultati
 */
void search_files_recursive(const char *dirpath, const char *file_type, int client_fd) {
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    char full_path[MAX_PATH];
    char result_buffer[MAX_PATH + 10];
    
    dir = opendir(dirpath);
    if (dir == NULL) {
        return;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        /* Salta le directory . e .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        /* Costruisce il percorso completo */
        snprintf(full_path, sizeof(full_path), "%s/%s", dirpath, entry->d_name);
        
        if (stat(full_path, &statbuf) == 0) {
            if (S_ISDIR(statbuf.st_mode)) {
                /* Se è una directory, ricerca ricorsivamente */
                search_files_recursive(full_path, file_type, client_fd);
            } else if (S_ISREG(statbuf.st_mode)) {
                /* Se è un file regolare, verifica il tipo */
                if (strcmp(file_type, "JPEG") == 0 && is_jpeg_file(full_path)) {
                    snprintf(result_buffer, sizeof(result_buffer), "%s\n", full_path);
                    send(client_fd, result_buffer, strlen(result_buffer), 0);
                } else if (strcmp(file_type, "PNG") == 0 && is_png_file(full_path)) {
                    snprintf(result_buffer, sizeof(result_buffer), "%s\n", full_path);
                    send(client_fd, result_buffer, strlen(result_buffer), 0);
                }
            }
        }
    }
    
    closedir(dir);
}

/*
 * Funzione per gestire le richieste del client
 * client_fd: socket descriptor del client
 */
void handle_client_request(int client_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    char *home_dir;
    
    /* Riceve la richiesta dal client */
    bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received <= 0) {
        return;
    }
    
    buffer[bytes_received] = '\0';
    
    /* Ottiene la home directory */
    home_dir = getenv("HOME");
    if (home_dir == NULL) {
        const char *error_msg = "Errore: impossibile ottenere la home directory\n";
        send(client_fd, error_msg, strlen(error_msg), 0);
        return;
    }
    
    /* Verifica il tipo di richiesta e avvia la ricerca */
    if (strcmp(buffer, "JPEG") == 0) {
        printf("Ricerca file JPEG in corso...\n");
        search_files_recursive(home_dir, "JPEG", client_fd);
        printf("Ricerca JPEG completata.\n");
    } else if (strcmp(buffer, "PNG") == 0) {
        printf("Ricerca file PNG in corso...\n");
        search_files_recursive(home_dir, "PNG", client_fd);
        printf("Ricerca PNG completata.\n");
    } else {
        const char *error_msg = "Richiesta non valida. Usa 'JPEG' o 'PNG'\n";
        send(client_fd, error_msg, strlen(error_msg), 0);
    }
}

/*
 * Funzione per creare e configurare il socket server
 * Ritorna il file descriptor del socket
 */
int create_server_socket(void) {
    int sockfd, optval = 1;
    struct sockaddr_in server_addr;
    
    /* Crea il socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        err_sys("socket error");
    }
    
    /* Imposta l'opzione SO_REUSEADDR */
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        err_sys("setsockopt error");
    }
    
    /* Configura l'indirizzo del server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);
    
    /* Bind del socket */
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        err_sys("bind error");
    }
    
    /* Listen per le connessioni */
    if (listen(sockfd, 5) < 0) {
        err_sys("listen error");
    }
    
    return sockfd;
}

/*
 * Funzione principale del server
 */
int main(void) {
    int server_fd, client_fd;
    struct sockaddr_in client_addr;
    socklen_t client_len;
    pid_t pid;
    
    printf("Server avviato sulla porta %d\n", SERVER_PORT);
    printf("In attesa di connessioni...\n");
    
    /* Crea il socket server */
    server_fd = create_server_socket();
    
    while (1) {
        client_len = sizeof(client_addr);
        
        /* Accetta una connessione */
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            err_sys("accept error");
        }
        
        printf("Connessione accettata da un client\n");
        
        /* Crea un processo figlio per gestire il client */
        if ((pid = fork()) == 0) {
            /* Processo figlio */
            close(server_fd);
            handle_client_request(client_fd);
            close(client_fd);
            exit(0);
        } else if (pid > 0) {
            /* Processo padre */
            close(client_fd);
        } else {
            err_sys("fork error");
        }
    }
    
    close(server_fd);
    return 0;
}