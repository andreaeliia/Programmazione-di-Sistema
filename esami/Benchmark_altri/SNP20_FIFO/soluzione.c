/*Quattro thread di un processo devono realizzare una catena di montaggio che svolga le
seguenti funzioni:
la thread 1 deve generare dei numeri interi positivi casuali e
posizionarli in una FIFO di dimensione massima fornita come parametro dalla linea di
comando;
la thread 2 deve scoprire, leggendoli dall'uscita della FIFO, se tali interi sono
primi; 
la thread 3 deve spostarli in un'altra FIFO se sono primi e rimuoverli se non lo
sono; 
la thread 4 deve dare allo standard output i fattori di ciascun numero non primo. Al
momento in cui il programma viene interrotto con ^C, deve stampare la lista dei numeri
primi trovati fino a quel momento.*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#define FIFO1_NAME "/tmp/pipeline_fifo1"
#define FIFO2_NAME "/tmp/pipeline_fifo2"
#define MAX_NUMBER_SIZE 16
#define MAX_PRIMES 1000
#define MAX_FACTORS 100

/* Struttura per passare numeri non primi a Thread4 */
typedef struct {
    int numbers[MAX_FACTORS];
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} non_prime_queue_t;

/* Lista globale dei numeri primi (per stampa finale) */
typedef struct {
    int primes[MAX_PRIMES];
    int count;
    pthread_mutex_t mutex;
} prime_list_t;

/*===============VARIABILI GLOBALI==========*/
pthread_t thread1, thread2, thread3, thread4;
volatile int running = 1;
int max_fifo_size = 1048576; /* Default 1MB */

non_prime_queue_t non_prime_queue;
prime_list_t prime_list;

/*=================UTILS===========================*/
int random_value() {
    return rand() % 10000 + 1; /* Evita zero */
}

int is_prime_number(int number) {
    int i;
    
    if (number < 2) return 0;
    if (number == 2) return 1;
    if (number % 2 == 0) return 0;
    
    for (i = 3; i * i <= number; i += 2) {
        if (number % i == 0) return 0;
    }
    return 1;
}

void find_factors(int number) {
    int i;
    int temp = number;
    
    printf("Fattori di %d: ", number);
    for (i = 2; i <= temp; i++) {
        while (temp % i == 0) {
            printf("%d ", i);
            temp /= i;
        }
    }
    printf("\n");
}

void add_to_prime_list(int prime) {
    pthread_mutex_lock(&prime_list.mutex);
    if (prime_list.count < MAX_PRIMES) {
        prime_list.primes[prime_list.count] = prime;
        prime_list.count++;
    }
    pthread_mutex_unlock(&prime_list.mutex);
}

void add_to_non_prime_queue(int number) {
    pthread_mutex_lock(&non_prime_queue.mutex);
    if (non_prime_queue.count < MAX_FACTORS) {
        non_prime_queue.numbers[non_prime_queue.count] = number;
        non_prime_queue.count++;
        pthread_cond_signal(&non_prime_queue.cond);
    }
    pthread_mutex_unlock(&non_prime_queue.mutex);
}

/*================FIFO MANAGEMENT=============*/
int create_fifo(const char* fifo_name) {
    int res;
    
    if (access(fifo_name, F_OK) != -1) {
        unlink(fifo_name); /* Rimuovi se esiste */
    }
    
    res = mkfifo(fifo_name, 0666);
    if (res != 0) {
        perror("mkfifo");
        return -1;
    }
    return 0;
}

void cleanup_fifos() {
    unlink(FIFO1_NAME);
    unlink(FIFO2_NAME);
}

/*===================THREADS==============*/

void* thread1_generator(void* arg) {
    int fd;
    int bytes_written;
    int number;
    char buffer[MAX_NUMBER_SIZE];
    int len;
    int res;
    
    printf("Thread1 (Generator): Avviato\n");
    
    /* Crea e apri FIFO1 per scrittura */
    if (create_fifo(FIFO1_NAME) != 0) {
        printf("Thread1: Errore creazione FIFO1\n");
        return NULL;
    }
    
    fd = open(FIFO1_NAME, O_WRONLY);
    if (fd == -1) {
        perror("Thread1: open FIFO1");
        return NULL;
    }
    
    bytes_written = 0;
    while (running && bytes_written < max_fifo_size) {
        number = random_value();
        len = snprintf(buffer, sizeof(buffer), "%d\n", number);
        
        res = write(fd, buffer, len);
        if (res == -1) {
            if (errno == EPIPE) break; /* Reader chiuso */
            perror("Thread1: write");
            break;
        }
        
        printf("Thread1: Generato %d\n", number);
        bytes_written += res;
        sleep(1); /* 100ms pausa */
    }
    
    close(fd);
    printf("Thread1: Terminato (%d bytes scritti)\n", bytes_written);
    return NULL;
}

void* thread2_prime_checker(void* arg) {
    int fd1, fd2;
    char buffer[MAX_NUMBER_SIZE];
    char line[MAX_NUMBER_SIZE];
    int line_pos;
    int res;
    char c;
    int number;
    char prime_buffer[MAX_NUMBER_SIZE];
    int len;
    
    printf("Thread2 (Prime Checker): Avviato\n");
    
    /* Apri FIFO1 per lettura */
    fd1 = open(FIFO1_NAME, O_RDONLY);
    if (fd1 == -1) {
        perror("Thread2: open FIFO1");
        return NULL;
    }
    
    /* Crea e apri FIFO2 per scrittura (numeri primi) */
    if (create_fifo(FIFO2_NAME) != 0) {
        printf("Thread2: Errore creazione FIFO2\n");
        close(fd1);
        return NULL;
    }
    
    fd2 = open(FIFO2_NAME, O_WRONLY);
    if (fd2 == -1) {
        perror("Thread2: open FIFO2");
        close(fd1);
        return NULL;
    }
    
    line_pos = 0;
    
    while (running) {
        res = read(fd1, buffer, 1); /* Leggi carattere per carattere */
        if (res <= 0) break;
        
        c = buffer[0];
        if (c == '\n') {
            line[line_pos] = '\0';
            number = atoi(line);
            
            if (number > 0) {
                if (is_prime_number(number)) {
                    printf("Thread2: %d e' primo\n", number);
                    /* Invia a Thread3 via FIFO2 */
                    len = snprintf(prime_buffer, sizeof(prime_buffer), "%d\n", number);
                    write(fd2, prime_buffer, len);
                } else {
                    printf("Thread2: %d NON e' primo\n", number);
                    /* Invia a Thread4 via coda condivisa */
                    add_to_non_prime_queue(number);
                }
            }
            line_pos = 0;
        } else if (line_pos < MAX_NUMBER_SIZE - 1) {
            line[line_pos] = c;
            line_pos++;
        }
    }
    
    close(fd1);
    close(fd2);
    printf("Thread2: Terminato\n");
    return NULL;
}

void* thread3_prime_mover(void* arg) {
    int fd;
    char buffer[MAX_NUMBER_SIZE];
    char line[MAX_NUMBER_SIZE];
    int line_pos;
    int res;
    char c;
    int prime;
    
    printf("Thread3 (Prime Mover): Avviato\n");
    
    /* Aspetta che FIFO2 sia disponibile */
    sleep(1);
    
    fd = open(FIFO2_NAME, O_RDONLY);
    if (fd == -1) {
        perror("Thread3: open FIFO2");
        return NULL;
    }
    
    line_pos = 0;
    
    while (running) {
        res = read(fd, buffer, 1);
        if (res <= 0) break;
        
        c = buffer[0];
        if (c == '\n') {
            line[line_pos] = '\0';
            prime = atoi(line);
            
            if (prime > 0) {
                printf("Thread3: Spostato primo %d nella lista\n", prime);
                add_to_prime_list(prime);
            }
            line_pos = 0;
        } else if (line_pos < MAX_NUMBER_SIZE - 1) {
            line[line_pos] = c;
            line_pos++;
        }
    }
    
    close(fd);
    printf("Thread3: Terminato\n");
    return NULL;
}

void* thread4_factor_printer(void* arg) {
    int number;
    int i;
    
    printf("Thread4 (Factor Printer): Avviato\n");
    
    while (running) {
        pthread_mutex_lock(&non_prime_queue.mutex);
        
        while (non_prime_queue.count == 0 && running) {
            pthread_cond_wait(&non_prime_queue.cond, &non_prime_queue.mutex);
        }
        
        if (!running) {
            pthread_mutex_unlock(&non_prime_queue.mutex);
            break;
        }
        
        number = non_prime_queue.numbers[0];
        /* Shift array */
        for (i = 0; i < non_prime_queue.count - 1; i++) {
            non_prime_queue.numbers[i] = non_prime_queue.numbers[i + 1];
        }
        non_prime_queue.count--;
        
        pthread_mutex_unlock(&non_prime_queue.mutex);
        
        printf("Thread4: ");
        find_factors(number);
    }
    
    printf("Thread4: Terminato\n");
    return NULL;
}

/*==============SIGNAL HANDLER===============*/
void signal_handler(int sig) {
    int i;
    
    printf("\n\n=== SIGNAL %d RICEVUTO ===\n", sig);
    running = 0;
    
    /* Sblocca thread4 se in attesa */
    pthread_cond_broadcast(&non_prime_queue.cond);
    
    /* Stampa lista numeri primi */
    pthread_mutex_lock(&prime_list.mutex);
    printf("\n=== NUMERI PRIMI TROVATI (%d) ===\n", prime_list.count);
    for (i = 0; i < prime_list.count; i++) {
        printf("%d ", prime_list.primes[i]);
        if ((i + 1) % 10 == 0) printf("\n");
    }
    printf("\n==============================\n");
    pthread_mutex_unlock(&prime_list.mutex);
}

void init_global_vars() {
    int i;
    
    /* Inizializza non_prime_queue */
    for (i = 0; i < MAX_FACTORS; i++) {
        non_prime_queue.numbers[i] = 0;
    }
    non_prime_queue.count = 0;
    pthread_mutex_init(&non_prime_queue.mutex, NULL);
    pthread_cond_init(&non_prime_queue.cond, NULL);
    
    /* Inizializza prime_list */
    for (i = 0; i < MAX_PRIMES; i++) {
        prime_list.primes[i] = 0;
    }
    prime_list.count = 0;
    pthread_mutex_init(&prime_list.mutex, NULL);
}

/*===========MAIN===================*/
int main(int argc, char* argv[]) {
    printf("=== CATENA DI MONTAGGIO AVVIATA ===\n");
    
    /* Parametro dimensione FIFO da linea di comando */
    if (argc > 1) {
        max_fifo_size = atoi(argv[1]);
        if (max_fifo_size <= 0) max_fifo_size = 1048576; /* 1MB default */
    }
    printf("Dimensione massima FIFO: %d bytes\n", max_fifo_size);
    
    /* Inizializza strutture globali */
    init_global_vars();
    
    /* Cleanup precedenti */
    cleanup_fifos();
    
    /* Setup signal handler */
    srand(time(NULL));
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* Crea i 4 thread */
    pthread_create(&thread1, NULL, thread1_generator, NULL);
    sleep(1); /* Da tempo al thread1 di creare FIFO1 */
    
    pthread_create(&thread2, NULL, thread2_prime_checker, NULL);
    sleep(1); /* Da tempo al thread2 di creare FIFO2 */
    
    pthread_create(&thread3, NULL, thread3_prime_mover, NULL);
    pthread_create(&thread4, NULL, thread4_factor_printer, NULL);
    
    /* Aspetta terminazione */
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);
    pthread_join(thread4, NULL);
    
    cleanup_fifos();
    printf("\n=== PROGRAMMA TERMINATO ===\n");
    return 0;
}