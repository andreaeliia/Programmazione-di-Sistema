/*
 * Confronto prestazioni IPC: Pipe, TCP, Shared Memory
 * Trasferimento di 1 milione di numeri interi casuali
 * Standard C90 compatibile
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>

#define NUM_COUNT 1000000
#define TCP_PORT 12345
#define SHM_NAME "/ipc_test_shm"
#define CHUNK_SIZE 1024  /* Numeri da inviare per volta */

/* Struttura per memoria condivisa */
typedef struct {
    int numbers[NUM_COUNT];
    int ready;
    int completed;
} SharedData;

/* Funzione per ottenere timestamp in microsecondi */
long long get_time_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000 + tv.tv_usec;
}

/* Genera array di numeri casuali */
void generate_numbers(int *numbers, int count) {
    int i;
    srand(42); /* Seed fisso per riproducibilità */
    for (i = 0; i < count; i++) {
        numbers[i] = rand();
    }
}

/* Verifica correttezza trasferimento */
int verify_numbers(int *original, int *received, int count) {
    int i;
    for (i = 0; i < count; i++) {
        if (original[i] != received[i]) {
            return 0; /* Errore */
        }
    }
    return 1; /* OK */
}

/* Test 1: Unnamed Pipe */
double test_unnamed_pipe() {
    int pipefd[2];
    pid_t pid;
    int *numbers;
    int *received;
    long long start_time, end_time;
    int i, bytes_read, total_read;
    
    printf("Test 1: Unnamed Pipe\n");
    
    /* Alloca memoria per i numeri */
    numbers = malloc(NUM_COUNT * sizeof(int));
    received = malloc(NUM_COUNT * sizeof(int));
    if (!numbers || !received) {
        perror("malloc");
        return -1;
    }
    
    /* Genera numeri casuali */
    generate_numbers(numbers, NUM_COUNT);
    
    /* Crea pipe */
    if (pipe(pipefd) == -1) {
        perror("pipe");
        free(numbers);
        free(received);
        return -1;
    }
    
    start_time = get_time_us();
    
    pid = fork();
    if (pid == -1) {
        perror("fork");
        free(numbers);
        free(received);
        return -1;
    }
    
    if (pid == 0) {
        /* Processo figlio - scrive nella pipe */
        close(pipefd[0]); /* Chiude lettura */
        
        /* Scrive tutti i numeri */
        if (write(pipefd[1], numbers, NUM_COUNT * sizeof(int)) == -1) {
            perror("write");
        }
        
        close(pipefd[1]);
        free(numbers);
        free(received);
        exit(0);
    } else {
        /* Processo padre - legge dalla pipe */
        close(pipefd[1]); /* Chiude scrittura */
        
        /* Legge tutti i numeri */
        total_read = 0;
        while (total_read < NUM_COUNT * sizeof(int)) {
            bytes_read = read(pipefd[0], 
                            ((char*)received) + total_read,
                            NUM_COUNT * sizeof(int) - total_read);
            if (bytes_read <= 0) break;
            total_read += bytes_read;
        }
        
        close(pipefd[0]);
        wait(NULL);
        
        end_time = get_time_us();
        
        /* Verifica correttezza */
        if (verify_numbers(numbers, received, NUM_COUNT)) {
            printf("  Trasferimento completato correttamente\n");
        } else {
            printf("  ERRORE: Dati corrotti!\n");
        }
    }
    
    free(numbers);
    free(received);
    
    return (double)(end_time - start_time) / 1000.0; /* Ritorna millisecondi */
}

/* Test 2: TCP Locale */
double test_tcp_local() {
    int server_fd, client_fd, conn_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    pid_t pid;
    int *numbers;
    int *received;
    long long start_time, end_time;
    int i, bytes_sent, bytes_read, total_sent, total_read;
    int opt = 1;
    
    printf("Test 2: TCP Locale\n");
    
    /* Alloca memoria */
    numbers = malloc(NUM_COUNT * sizeof(int));
    received = malloc(NUM_COUNT * sizeof(int));
    if (!numbers || !received) {
        perror("malloc");
        return -1;
    }
    
    /* Genera numeri */
    generate_numbers(numbers, NUM_COUNT);
    
    /* Crea socket server */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        free(numbers);
        free(received);
        return -1;
    }
    
    /* Riusa indirizzo */
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    /* Configura indirizzo server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(TCP_PORT);
    
    /* Bind */
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        free(numbers);
        free(received);
        return -1;
    }
    
    /* Listen */
    if (listen(server_fd, 1) == -1) {
        perror("listen");
        close(server_fd);
        free(numbers);
        free(received);
        return -1;
    }
    
    start_time = get_time_us();
    
    pid = fork();
    if (pid == -1) {
        perror("fork");
        close(server_fd);
        free(numbers);
        free(received);
        return -1;
    }
    
    if (pid == 0) {
        /* Processo figlio - client */
        close(server_fd);
        sleep(1); /* Attende che il server sia pronto */
        
        /* Crea socket client */
        client_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (client_fd == -1) {
            perror("client socket");
            exit(1);
        }
        
        /* Connessione al server */
        if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
            perror("connect");
            close(client_fd);
            exit(1);
        }
        
        /* Invia tutti i numeri */
        total_sent = 0;
        while (total_sent < NUM_COUNT * sizeof(int)) {
            bytes_sent = send(client_fd,
                            ((char*)numbers) + total_sent,
                            NUM_COUNT * sizeof(int) - total_sent,
                            0);
            if (bytes_sent <= 0) break;
            total_sent += bytes_sent;
        }
        
        close(client_fd);
        free(numbers);
        free(received);
        exit(0);
    } else {
        /* Processo padre - server */
        client_len = sizeof(client_addr);
        conn_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (conn_fd == -1) {
            perror("accept");
            close(server_fd);
            free(numbers);
            free(received);
            return -1;
        }
        
        /* Riceve tutti i numeri */
        total_read = 0;
        while (total_read < NUM_COUNT * sizeof(int)) {
            bytes_read = recv(conn_fd,
                            ((char*)received) + total_read,
                            NUM_COUNT * sizeof(int) - total_read,
                            0);
            if (bytes_read <= 0) break;
            total_read += bytes_read;
        }
        
        close(conn_fd);
        close(server_fd);
        wait(NULL);
        
        end_time = get_time_us();
        
        /* Verifica correttezza */
        if (verify_numbers(numbers, received, NUM_COUNT)) {
            printf("  Trasferimento completato correttamente\n");
        } else {
            printf("  ERRORE: Dati corrotti!\n");
        }
    }
    
    free(numbers);
    free(received);
    
    return (double)(end_time - start_time) / 1000.0;
}

/* Test 3: Memoria Condivisa */
double test_shared_memory() {
    int shm_fd;
    SharedData *shared_data;
    pid_t pid;
    int *numbers;
    long long start_time, end_time;
    int i;
    
    printf("Test 3: Memoria Condivisa\n");
    
    /* Alloca memoria per numeri originali */
    numbers = malloc(NUM_COUNT * sizeof(int));
    if (!numbers) {
        perror("malloc");
        return -1;
    }
    
    /* Genera numeri */
    generate_numbers(numbers, NUM_COUNT);
    
    /* Cleanup precedente */
    shm_unlink(SHM_NAME);
    
    /* Crea memoria condivisa */
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        free(numbers);
        return -1;
    }
    
    if (ftruncate(shm_fd, sizeof(SharedData)) == -1) {
        perror("ftruncate");
        close(shm_fd);
        free(numbers);
        return -1;
    }
    
    /* Mappa memoria */
    shared_data = mmap(NULL, sizeof(SharedData), PROT_READ | PROT_WRITE,
                      MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        free(numbers);
        return -1;
    }
    
    close(shm_fd);
    
    /* Inizializza */
    shared_data->ready = 0;
    shared_data->completed = 0;
    
    start_time = get_time_us();
    
    pid = fork();
    if (pid == -1) {
        perror("fork");
        munmap(shared_data, sizeof(SharedData));
        free(numbers);
        return -1;
    }
    
    if (pid == 0) {
        /* Processo figlio - scrive in memoria condivisa */
        
        /* Copia tutti i numeri */
        for (i = 0; i < NUM_COUNT; i++) {
            shared_data->numbers[i] = numbers[i];
        }
        
        shared_data->ready = 1;
        
        munmap(shared_data, sizeof(SharedData));
        free(numbers);
        exit(0);
    } else {
        /* Processo padre - legge da memoria condivisa */
        
        /* Attende che i dati siano pronti */
        while (!shared_data->ready) {
            usleep(1000); /* Attende 1ms */
        }
        
        /* I dati sono già disponibili in memoria condivisa */
        shared_data->completed = 1;
        
        wait(NULL);
        end_time = get_time_us();
        
        /* Verifica correttezza */
        if (verify_numbers(numbers, shared_data->numbers, NUM_COUNT)) {
            printf("  Trasferimento completato correttamente\n");
        } else {
            printf("  ERRORE: Dati corrotti!\n");
        }
        
        munmap(shared_data, sizeof(SharedData));
        shm_unlink(SHM_NAME);
    }
    
    free(numbers);
    
    return (double)(end_time - start_time) / 1000.0;
}

/* Funzione principale */
int main() {
    double time_pipe, time_tcp, time_shm;
    
    printf("=== CONFRONTO PRESTAZIONI IPC ===\n");
    printf("Trasferimento di %d numeri interi casuali\n\n", NUM_COUNT);
    
    /* Test 1: Unnamed Pipe */
    time_pipe = test_unnamed_pipe();
    printf("  Tempo: %.2f ms\n\n", time_pipe);
    
    /* Test 2: TCP Locale */
    time_tcp = test_tcp_local();
    printf("  Tempo: %.2f ms\n\n", time_tcp);
    
    /* Test 3: Memoria Condivisa */
    time_shm = test_shared_memory();
    printf("  Tempo: %.2f ms\n\n", time_shm);
    
    /* Riepilogo risultati */
    printf("=== RIEPILOGO RISULTATI ===\n");
    printf("Unnamed Pipe:     %.2f ms\n", time_pipe);
    printf("TCP Locale:       %.2f ms\n", time_tcp);
    printf("Memoria Condivisa: %.2f ms\n", time_shm);
    
    printf("\n=== CLASSIFICA (dal più veloce) ===\n");
    if (time_shm <= time_pipe && time_shm <= time_tcp) {
        printf("1. Memoria Condivisa (%.2f ms)\n", time_shm);
        if (time_pipe <= time_tcp) {
            printf("2. Unnamed Pipe (%.2f ms)\n", time_pipe);
            printf("3. TCP Locale (%.2f ms)\n", time_tcp);
        } else {
            printf("2. TCP Locale (%.2f ms)\n", time_tcp);
            printf("3. Unnamed Pipe (%.2f ms)\n", time_pipe);
        }
    } else if (time_pipe <= time_shm && time_pipe <= time_tcp) {
        printf("1. Unnamed Pipe (%.2f ms)\n", time_pipe);
        if (time_shm <= time_tcp) {
            printf("2. Memoria Condivisa (%.2f ms)\n", time_shm);
            printf("3. TCP Locale (%.2f ms)\n", time_tcp);
        } else {
            printf("2. TCP Locale (%.2f ms)\n", time_tcp);
            printf("3. Memoria Condivisa (%.2f ms)\n", time_shm);
        }
    } else {
        printf("1. TCP Locale (%.2f ms)\n", time_tcp);
        if (time_shm <= time_pipe) {
            printf("2. Memoria Condivisa (%.2f ms)\n", time_shm);
            printf("3. Unnamed Pipe (%.2f ms)\n", time_pipe);
        } else {
            printf("2. Unnamed Pipe (%.2f ms)\n", time_pipe);
            printf("3. Memoria Condivisa (%.2f ms)\n", time_shm);
        }
    }
    
    printf("\nTest completato.\n");
    return 0;
}