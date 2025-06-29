#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>



#define PORT 8080
#define BUFFER_SIZE 1024



#define DIRECTORY "/mnt/cartella_test"

/*
 Il programma B trova tutti i file presenti nei volumi montati nella
macchina in cui gira la cui dimensione in bytes è superiore al numero 
ricevuto e per ciascuno di essi invia il percorso assoluto in un 
datagramma UDP al programma A, concludendo la lista con un datagramma
contenente la scritta "end".
*/




char buffer_path[BUFFER_SIZE];
        

// Struttura per raccogliere informazioni sui file
typedef struct {
    char **file_paths;
    int file_count;
    int capacity;
    long total_size;
} FileCollection;

// Inizializza collezione file
FileCollection* init_file_collection() {
    FileCollection* fc = malloc(sizeof(FileCollection));
    fc->file_paths = malloc(3000 * sizeof(char*));
    fc->file_count = 0;
    fc->capacity = 3000;
    fc->total_size = 0;
    return fc;
}


void free_file_collection(FileCollection* fc) {
    for (int i = 0; i < fc->file_count; i++) {
        free(fc->file_paths[i]);
    }
    free(fc->file_paths);
    free(fc);
}
// Aggiungi file alla collezione
void add_file_to_collection(FileCollection* fc, const char* filepath, long size) {
    if (fc->file_count >= fc->capacity) {
        fc->capacity *= 2;
        fc->file_paths = realloc(fc->file_paths, fc->capacity * sizeof(char*));
    }
    
    fc->file_paths[fc->file_count] = malloc(strlen(filepath) + 1);
    strcpy(fc->file_paths[fc->file_count], filepath);
    fc->file_count++;
    fc->total_size += size;
}
// FUNZIONE PRINCIPALE: Raccoglie TUTTI i file da una directory ricorsivamente
int collect_all_files_recursive(const char* directory_path,long long size,FileCollection* fc) {
    DIR* dir;
    struct dirent* entry;
    struct stat entry_stats;
    char full_path[1024];
    
    //printf("Esplorando directory: %s\n", directory_path);
    
    dir = opendir(directory_path);
    if (dir == NULL) {
        printf("Errore apertura directory %s: %s\n", directory_path, strerror(errno));
        return -1;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        // Salta . e ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Costruisci path completo
        snprintf(full_path, sizeof(full_path), "%s/%s", directory_path, entry->d_name);
        
        // Ottieni informazioni sul file/directory
        if (stat(full_path, &entry_stats) != 0) {
            printf("Errore stat su %s: %s\n", full_path, strerror(errno));
            continue;
        }
        
        if (S_ISREG(entry_stats.st_mode)) {
            // È un file regolare -
            //Controllo se la dimensione del file e' maggiore dell'entry dell'utente
            if((long long)entry_stats.st_size > size){
                add_file_to_collection(fc, full_path, entry_stats.st_size);
                printf("File trovato: %s (%ld bytes)\n", full_path, entry_stats.st_size);
                
              
            }

                //1.Converso i bytes in long long
            //printf("File trovato: %s (%ld bytes)\n", full_path, entry_stats.st_size);
            
        } else if (S_ISDIR(entry_stats.st_mode)) {
            // È una directory - ricorsione
            //printf("Directory trovata: %s (esplorando...)\n", full_path);
            collect_all_files_recursive(full_path, size,fc);
            
        } else {
            // Altri tipi di file (device, pipe, etc.)
            printf("File speciale trovato: %s\n", full_path);
        }
    }
    
    closedir(dir);
    return 0;
}



int main() {
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];


    
    
    // 1. Crea socket UDP
    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(1);
    }
    
    // 2. Configura indirizzo server
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // 3. Bind socket
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(1);
    }
    
    printf("Server UDP in ascolto sulla porta %d\n", PORT);
    
    // 4. Loop principale
    while (1) {

        FileCollection* fc;
        fc=init_file_collection();


        // Ricevi messaggio
        int bytes_received = recvfrom(server_fd, buffer, BUFFER_SIZE - 1, 0,
                                     (struct sockaddr*)&client_addr, &client_len);
        
        if (bytes_received < 0) {
            perror("recvfrom failed");
            continue;
        }
        




        buffer[bytes_received] = '\0';
        long long size_limits = atoll(buffer);
        printf("NUMERO : %lld\n",size_limits);


        collect_all_files_recursive(DIRECTORY,size_limits,fc);



        printf("Ricevuto da %s:%d: %s\n", 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port), 
               buffer);
        


        


        //collect_all_files_recursive(DIRECTORY,size_limits,fc);
        
        
        
        printf("======INVIO AL SERVER=====\n");
        for (int i = 0; i < fc->file_count; i++)
        {
            char response[BUFFER_SIZE];
            snprintf(response,BUFFER_SIZE,"Path: %s" ,fc->file_paths[i]);
            sendto(server_fd, response, strlen(response), 0,
               (struct sockaddr*)&client_addr, client_len);

            sleep(2);
        }


        printf("=====PATH MANDATI======\n");


        char end[BUFFER_SIZE] = "end";
        sendto(server_fd, end, strlen(end), 0,
               (struct sockaddr*)&client_addr, client_len);
        free_file_collection(fc);
        
    }
    
    close(server_fd);
    return 0;
}