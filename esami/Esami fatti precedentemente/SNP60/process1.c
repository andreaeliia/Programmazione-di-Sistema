/*
 * Esercizio: Tre processi che leggono parole da file e le scrivono
 * in memoria condivisa, sincronizzati per massimizzare la velocità.
 * Gestione segnale ^C per stampare il contenuto.
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
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#define SHARED_MEM_SIZE 4096
#define MAX_WORD_LEN 100
#define SHM_NAME "/word_buffer"
#define SEM_MUTEX "/mutex_sem"
#define SEM_TURN "/turn_sem"
#define FILENAME "italian.txt"
#define NUM_PROCESSES 3

// Struttura per la memoria condivisa
typedef struct {
    char buffer[SHARED_MEM_SIZE];
    int current_pos;
    int word_count;
    int turn;  // Per la rotazione a turno (0, 1, 2)
    pid_t processes[NUM_PROCESSES];
} SharedData;

// Variabili globali per cleanup
SharedData* shared_data = NULL;
sem_t* mutex_sem = NULL;
sem_t* turn_sem = NULL;
int process_id = -1;

// Array per memorizzare le parole del file (caricato una sola volta)
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
    printf("Caricate %d parole dal file %s\n", count, filename);
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
    printf("\n\n=== SEGNALE ^C RICEVUTO ===\n");
    
    if (shared_data != NULL) {
        printf("Contenuto della memoria condivisa:\n");
        printf("Parole scritte: %d\n", shared_data->word_count);
        printf("Posizione corrente: %d\n", shared_data->current_pos);
        printf("Contenuto:\n");
        printf("----------------------------------------\n");
        
        if (shared_data->current_pos > 0) {
            // Assicuriamoci che sia null-terminated
            char temp_buffer[SHARED_MEM_SIZE];
            memcpy(temp_buffer, shared_data->buffer, shared_data->current_pos);
            temp_buffer[shared_data->current_pos] = '\0';
            printf("%s\n", temp_buffer);
        } else {
            printf("(Nessun contenuto)\n");
        }
        printf("----------------------------------------\n");
    }
    
    // Cleanup e uscita
    if (mutex_sem) sem_close(mutex_sem);
    if (turn_sem) sem_close(turn_sem);
    if (shared_data) munmap(shared_data, sizeof(SharedData));
    
    exit(0);
}

// Funzione principale del processo worker
void process_worker(int id) {
    process_id = id;
    
    // Installa handler per SIGINT
    signal(SIGINT, sigint_handler);
    
    // Seed diverso per ogni processo
    srand(time(NULL) + getpid());
    
    printf("Processo %d (PID %d) avviato\n", id, getpid());
    
    while (1) {
        // Attende il proprio turno
        sem_wait(turn_sem);
        
        // Controlla se è davvero il suo turno
        sem_wait(mutex_sem);
        if (shared_data->turn != id) {
            sem_post(mutex_sem);
            sem_post(turn_sem);
            usleep(1000); // Piccola pausa per evitare busy waiting
            continue;
        }
        
        // È il suo turno, processa una parola
        char* word = get_random_word();
        if (word == NULL) {
            printf("Processo %d: errore nel leggere parola\n", id);
            shared_data->turn = (shared_data->turn + 1) % NUM_PROCESSES;
            sem_post(mutex_sem);
            sem_post(turn_sem);
            continue;
        }
        
        int word_len = strlen(word);
        
        // Controlla se c'è spazio sufficiente (parola + spazio)
        if (shared_data->current_pos + word_len + 1 >= SHARED_MEM_SIZE) {
            printf("Processo %d: buffer pieno, termino\n", id);
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
        
        printf("Processo %d: scritta parola '%s' (totale: %d parole)\n", 
               id, word, shared_data->word_count);
        
        // Passa il turno al processo successivo
        shared_data->turn = (shared_data->turn + 1) % NUM_PROCESSES;
        
        sem_post(mutex_sem);
        sem_post(turn_sem);
        
        // Pausa breve per rendere visibile la rotazione
        usleep(500000); // 0.5 secondi
    }
    
    printf("Processo %d terminato\n", id);
}

int main() {
    printf("=== SISTEMA TRE PROCESSI CON ROTAZIONE ===\n\n");
    
    // Carica le parole dal file
    total_words = load_words_from_file(FILENAME);
    if (total_words == 0) {
        fprintf(stderr, "Impossibile caricare parole dal file\n");
        exit(EXIT_FAILURE);
    }
    
    // Cleanup di eventuali risorse precedenti
    shm_unlink(SHM_NAME);
    sem_unlink(SEM_MUTEX);
    sem_unlink(SEM_TURN);
    
    // Crea memoria condivisa
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    
    if (ftruncate(shm_fd, sizeof(SharedData)) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }
    
    shared_data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE,
                       MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    
    close(shm_fd);
    
    // Inizializza struttura condivisa
    memset(shared_data, 0, sizeof(SharedData));
    shared_data->current_pos = 0;
    shared_data->word_count = 0;
    shared_data->turn = 0;
    
    // Crea semafori
    mutex_sem = sem_open(SEM_MUTEX, O_CREAT | O_EXCL, 0644, 1);
    if (mutex_sem == SEM_FAILED) {
        perror("sem_open mutex");
        exit(EXIT_FAILURE);
    }
    
    turn_sem = sem_open(SEM_TURN, O_CREAT | O_EXCL, 0644, 1);
    if (turn_sem == SEM_FAILED) {
        perror("sem_open turn");
        exit(EXIT_FAILURE);
    }
    
    printf("Memoria condivisa e semafori creati\n");
    
    // Installa handler per SIGINT nel processo padre
    signal(SIGINT, sigint_handler);
    
    // Crea i tre processi
    pid_t children[NUM_PROCESSES];
    
    for (int i = 0; i < NUM_PROCESSES; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0) {
            // Processo figlio
            process_worker(i);
            exit(0);
        }
        else {
            // Processo padre
            children[i] = pid;
            shared_data->processes[i] = pid;
            printf("Creato processo %d con PID %d\n", i, pid);
        }
    }
    
    printf("\nTutti i processi avviati. Premi ^C per vedere il contenuto.\n");
    printf("I processi scriveranno a turno...\n\n");
    
    // Il padre aspetta i figli
    for (int i = 0; i < NUM_PROCESSES; i++) {
        int status;
        wait(&status);
        printf("Processo figlio terminato\n");
    }
    
    // Cleanup finale
    printf("\n=== CONTENUTO FINALE ===\n");
    if (shared_data->current_pos > 0) {
        shared_data->buffer[shared_data->current_pos] = '\0';
        printf("Parole totali: %d\n", shared_data->word_count);
        printf("Contenuto: %s\n", shared_data->buffer);
    }
    
    sem_close(mutex_sem);
    sem_close(turn_sem);
    sem_unlink(SEM_MUTEX);
    sem_unlink(SEM_TURN);
    munmap(shared_data, sizeof(SharedData));
    shm_unlink(SHM_NAME);
    
    printf("\nProgramma terminato\n");
    return 0;
}