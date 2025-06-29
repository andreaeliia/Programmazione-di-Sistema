/*
 * CLIENT.C - Programma client che si connette alla memoria condivisa
 * e scrive parole a turno
 * 
 * Uso: ./client <id_processo>
 * dove id_processo è 0, 1 o 2
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include <signal.h>
#include <errno.h>

#define SHARED_MEM_SIZE 4096
#define MAX_WORD_LEN 100
#define SHM_NAME "/word_buffer_v2"
#define SEM_MUTEX "/mutex_sem_v2"
#define SEM_TURN "/turn_sem_v2"
#define FILENAME "italian.txt"
#define NUM_PROCESSES 3

// Struttura per la memoria condivisa (deve essere identica al server)
typedef struct {
    char buffer[SHARED_MEM_SIZE];
    int current_pos;
    int word_count;
    int turn;
    int active_clients;
    int shutdown_requested;
} SharedData;

// Variabili globali
SharedData* shared_data = NULL;
sem_t* mutex_sem = NULL;
sem_t* turn_sem = NULL;
int client_id = -1;
int running = 1;

// Array per memorizzare le parole del file
char words[1000][MAX_WORD_LEN];
int total_words = 0;

// Funzione per caricare tutte le parole dal file
int load_words_from_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Errore apertura file italian.txt");
        return 0;
    }
    
    char word[MAX_WORD_LEN];
    int count = 0;
    
    while (fscanf(file, "%99s", word) == 1 && count < 1000) {
        strcpy(words[count], word);
        count++;
    }
    
    fclose(file);
    return count;
}

// Funzione per ottenere una parola casuale
char* get_random_word() {
    if (total_words == 0) return NULL;
    int index = rand() % total_words;
    return words[index];
}

// Gestore del segnale SIGINT (^C)
void sigint_handler(int sig) {
    printf("\nClient %d: ricevuto segnale di terminazione\n", client_id);
    running = 0;
}

// Funzione per registrare il client
void register_client() {
    sem_wait(mutex_sem);
    shared_data->active_clients++;
    printf("Client %d registrato. Client attivi: %d\n", 
           client_id, shared_data->active_clients);
    sem_post(mutex_sem);
}

// Funzione per deregistrare il client
void unregister_client() {
    sem_wait(mutex_sem);
    shared_data->active_clients--;
    printf("Client %d deregistrato. Client attivi: %d\n", 
           client_id, shared_data->active_clients);
    sem_post(mutex_sem);
}

int main(int argc, char* argv[]) {
    // Controlla argomenti
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <id_client>\n", argv[0]);
        fprintf(stderr, "dove id_client è 0, 1 o 2\n");
        exit(EXIT_FAILURE);
    }
    
    client_id = atoi(argv[1]);
    if (client_id < 0 || client_id >= NUM_PROCESSES) {
        fprintf(stderr, "ID client deve essere 0, 1 o 2\n");
        exit(EXIT_FAILURE);
    }
    
    printf("=== CLIENT %d AVVIATO (PID %d) ===\n", client_id, getpid());
    
    // Installa handler per SIGINT
    signal(SIGINT, sigint_handler);
    
    // Seed diverso per ogni client
    srand(time(NULL) + getpid());
    
    // Carica le parole dal file
    total_words = load_words_from_file(FILENAME);
    if (total_words == 0) {
        fprintf(stderr, "Client %d: impossibile caricare parole dal file\n", client_id);
        exit(EXIT_FAILURE);
    }
    printf("Client %d: caricate %d parole dal file\n", client_id, total_words);
    
    // Connessione alla memoria condivisa
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0);
    if (shm_fd == -1) {
        perror("shm_open - il server è in esecuzione?");
        exit(EXIT_FAILURE);
    }
    
    shared_data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE,
                       MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    close(shm_fd);
    
    // Apertura semafori
    mutex_sem = sem_open(SEM_MUTEX, 0);
    if (mutex_sem == SEM_FAILED) {
        perror("sem_open mutex");
        exit(EXIT_FAILURE);
    }
    
    turn_sem = sem_open(SEM_TURN, 0);
    if (turn_sem == SEM_FAILED) {
        perror("sem_open turn");
        exit(EXIT_FAILURE);
    }
    
    printf("Client %d: connesso alla memoria condivisa\n", client_id);
    
    // Registra il client
    register_client();
    
    // Loop principale
    while (running && !shared_data->shutdown_requested) {
        // Attende il proprio turno
        sem_wait(turn_sem);
        
        // Controlla se è davvero il suo turno
        sem_wait(mutex_sem);
        
        // Se richiesta terminazione, esce
        if (shared_data->shutdown_requested) {
            sem_post(mutex_sem);
            sem_post(turn_sem);
            break;
        }
        
        if (shared_data->turn != client_id) {
            sem_post(mutex_sem);
            sem_post(turn_sem);
            usleep(10000); // 10ms
            continue;
        }
        
        // È il suo turno, processa una parola
        char* word = get_random_word();
        if (word == NULL) {
            printf("Client %d: errore nel leggere parola\n", client_id);
            shared_data->turn = (shared_data->turn + 1) % NUM_PROCESSES;
            sem_post(mutex_sem);
            sem_post(turn_sem);
            continue;
        }
        
        int word_len = strlen(word);
        
        // Controlla se c'è spazio sufficiente (parola + spazio)
        if (shared_data->current_pos + word_len + 1 >= SHARED_MEM_SIZE) {
            printf("Client %d: buffer pieno, termino\n", client_id);
            shared_data->shutdown_requested = 1;
            sem_post(mutex_sem);
            sem_post(turn_sem);
            break;
        }
        
        // Scrive la parola nel buffer
        if (shared_data->current_pos > 0) {
            shared_data->buffer[shared_data->current_pos] = ' ';
            shared_data->current_pos++;
        }
        
        memcpy(shared_data->buffer + shared_data->current_pos, word, word_len);
        shared_data->current_pos += word_len;
        shared_data->word_count++;
        
        printf("Client %d: scritta parola '%s' (totale: %d parole, pos: %d)\n", 
               client_id, word, shared_data->word_count, shared_data->current_pos);
        
        // Passa il turno al processo successivo
        shared_data->turn = (shared_data->turn + 1) % NUM_PROCESSES;
        
        sem_post(mutex_sem);
        sem_post(turn_sem);
        
        // Pausa per rendere visibile la rotazione
        usleep(500000); // 0.5 secondi
    }
    
    printf("Client %d: terminazione in corso...\n", client_id);
    
    // Deregistra il client
    unregister_client();
    
    // Cleanup
    sem_close(mutex_sem);
    sem_close(turn_sem);
    munmap(shared_data, sizeof(SharedData));
    
    printf("Client %d terminato\n", client_id);
    return 0;
}