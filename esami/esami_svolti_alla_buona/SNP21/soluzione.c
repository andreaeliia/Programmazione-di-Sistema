/*
 * Programma per confrontare le prestazioni di scambio dati tra processi
 * utilizzando memory mapping e pipe di vario tipo.
 * 
 * Implementa test di performance per:
 * - Memory Mapping: mmap shared, anonymous, file-backed
 * - Pipe: anonime, named pipe (FIFO), socket pairs
 *
 * Compatibile Linux/macOS con libreria APUE
 */

#include "apue.h"
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>

/* Costanti per i test */
#define TEST_DATA_SIZE (1024 * 1024)  /* 1MB di dati per test */
#define NUM_ITERATIONS 100             /* Numero di iterazioni per test */
#define FIFO_NAME "/tmp/benchmark_fifo"
#define MMAP_FILE "/tmp/benchmark_mmap"

/* Struttura per risultati timing */
typedef struct {
    double elapsed_time;
    double throughput_mbps;
    const char* method_name;
} benchmark_result_t;

/* Prototipi funzioni */
static double get_time_diff(struct timeval *start, struct timeval *end);
static void print_results(benchmark_result_t *results, int num_results);
static void cleanup_files(void);

/* Test Memory Mapping */
static benchmark_result_t test_mmap_shared_anonymous(void);
static benchmark_result_t test_mmap_file_backed(void);

/* Test Pipe */
static benchmark_result_t test_pipe_anonymous(void);
static benchmark_result_t test_pipe_named(void);
static benchmark_result_t test_socket_pair(void);

/* Utility */
static void fill_test_data(char *buffer, size_t size);
static int verify_test_data(const char *buffer, size_t size);

/*
 * Calcola differenza di tempo in millisecondi
 */
static double get_time_diff(struct timeval *start, struct timeval *end)
{
    return (end->tv_sec - start->tv_sec) * 1000.0 + 
           (end->tv_usec - start->tv_usec) / 1000.0;
}

/*
 * Riempie buffer con dati di test riconoscibili
 */
static void fill_test_data(char *buffer, size_t size)
{
    size_t i;
    for (i = 0; i < size; i++) {
        buffer[i] = (char)(i % 256);
    }
}

/*
 * Verifica integrità dati ricevuti
 */
static int verify_test_data(const char *buffer, size_t size)
{
    size_t i;
    for (i = 0; i < size; i++) {
        if (buffer[i] != (char)(i % 256)) {
            return 0; /* Errore nei dati */
        }
    }
    return 1; /* Dati corretti */
}

/*
 * Test Memory Mapping Anonymous Shared
 */
static benchmark_result_t test_mmap_shared_anonymous(void)
{
    benchmark_result_t result = {0.0, 0.0, "Memory Mapping (Anonymous Shared)"};
    struct timeval start, end;
    void *shared_mem;
    pid_t pid;
    int i;
    char *test_data;
    
    printf("Testing Memory Mapping (Anonymous Shared)...\n");
    
    /* Alloca memoria condivisa anonima */
    shared_mem = mmap(NULL, TEST_DATA_SIZE, PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shared_mem == MAP_FAILED) {
        err_sys("mmap anonymous failed");
    }
    
    /* Alloca buffer per dati di test */
    test_data = malloc(TEST_DATA_SIZE);
    if (test_data == NULL) {
        err_sys("malloc failed");
    }
    fill_test_data(test_data, TEST_DATA_SIZE);
    
    gettimeofday(&start, NULL);
    
    /* Fork processo figlio */
    if ((pid = fork()) < 0) {
        err_sys("fork failed");
    } else if (pid == 0) {
        /* Processo figlio - legge dati */
        for (i = 0; i < NUM_ITERATIONS; i++) {
            /* Attendi che il padre scriva (sincronizzazione semplice) */
            while (((volatile char*)shared_mem)[0] != (char)(i % 256)) {
                usleep(1);
            }
            
            /* Verifica dati ricevuti */
            if (!verify_test_data((char*)shared_mem, TEST_DATA_SIZE)) {
                fprintf(stderr, "Data verification failed at iteration %d\n", i);
            }
            
            /* Segnala al padre che ha finito */
            ((volatile char*)shared_mem)[TEST_DATA_SIZE-1] = (char)(i % 256);
        }
        exit(0);
    } else {
        /* Processo padre - scrive dati */
        for (i = 0; i < NUM_ITERATIONS; i++) {
            /* Modifica primo byte per segnalare nuovi dati */
            test_data[0] = (char)(i % 256);
            
            /* Copia dati in memoria condivisa */
            memcpy(shared_mem, test_data, TEST_DATA_SIZE);
            
            /* Attendi che figlio finisca di leggere */
            while (((volatile char*)shared_mem)[TEST_DATA_SIZE-1] != (char)(i % 256)) {
                usleep(1);
            }
        }
        
        wait(NULL); /* Attendi terminazione figlio */
        gettimeofday(&end, NULL);
        
        result.elapsed_time = get_time_diff(&start, &end);
        result.throughput_mbps = (TEST_DATA_SIZE * NUM_ITERATIONS) / 
                                (result.elapsed_time * 1024.0);
    }
    
    /* Cleanup */
    munmap(shared_mem, TEST_DATA_SIZE);
    free(test_data);
    
    return result;
}

/*
 * Test Memory Mapping File-Backed
 */
static benchmark_result_t test_mmap_file_backed(void)
{
    benchmark_result_t result = {0.0, 0.0, "Memory Mapping (File-Backed)"};
    struct timeval start, end;
    void *shared_mem;
    pid_t pid;
    int fd, i;
    char *test_data;
    
    printf("Testing Memory Mapping (File-Backed)...\n");
    
    /* Crea file temporaneo */
    if ((fd = open(MMAP_FILE, O_CREAT | O_RDWR | O_TRUNC, 0666)) < 0) {
        err_sys("open failed");
    }
    
    /* Ridimensiona file */
    if (lseek(fd, TEST_DATA_SIZE - 1, SEEK_SET) == -1) {
        err_sys("lseek failed");
    }
    if (write(fd, "", 1) != 1) {
        err_sys("write failed");
    }
    
    /* Mappa file in memoria */
    shared_mem = mmap(NULL, TEST_DATA_SIZE, PROT_READ | PROT_WRITE,
                      MAP_SHARED, fd, 0);
    if (shared_mem == MAP_FAILED) {
        err_sys("mmap file failed");
    }
    
    test_data = malloc(TEST_DATA_SIZE);
    if (test_data == NULL) {
        err_sys("malloc failed");
    }
    fill_test_data(test_data, TEST_DATA_SIZE);
    
    gettimeofday(&start, NULL);
    
    if ((pid = fork()) < 0) {
        err_sys("fork failed");
    } else if (pid == 0) {
        /* Processo figlio */
        for (i = 0; i < NUM_ITERATIONS; i++) {
            while (((volatile char*)shared_mem)[0] != (char)(i % 256)) {
                usleep(1);
            }
            
            if (!verify_test_data((char*)shared_mem, TEST_DATA_SIZE)) {
                fprintf(stderr, "Data verification failed at iteration %d\n", i);
            }
            
            ((volatile char*)shared_mem)[TEST_DATA_SIZE-1] = (char)(i % 256);
        }
        exit(0);
    } else {
        /* Processo padre */
        for (i = 0; i < NUM_ITERATIONS; i++) {
            test_data[0] = (char)(i % 256);
            memcpy(shared_mem, test_data, TEST_DATA_SIZE);
            
            while (((volatile char*)shared_mem)[TEST_DATA_SIZE-1] != (char)(i % 256)) {
                usleep(1);
            }
        }
        
        wait(NULL);
        gettimeofday(&end, NULL);
        
        result.elapsed_time = get_time_diff(&start, &end);
        result.throughput_mbps = (TEST_DATA_SIZE * NUM_ITERATIONS) / 
                                (result.elapsed_time * 1024.0);
    }
    
    /* Cleanup */
    munmap(shared_mem, TEST_DATA_SIZE);
    close(fd);
    free(test_data);
    
    return result;
}

/*
 * Test Pipe Anonime
 */
static benchmark_result_t test_pipe_anonymous(void)
{
    benchmark_result_t result = {0.0, 0.0, "Pipe (Anonymous)"};
    struct timeval start, end;
    int pipefd[2];
    pid_t pid;
    int i;
    char *test_data;
    char *read_buffer;
    
    printf("Testing Pipe (Anonymous)...\n");
    
    if (pipe(pipefd) == -1) {
        err_sys("pipe failed");
    }
    
    test_data = malloc(TEST_DATA_SIZE);
    read_buffer = malloc(TEST_DATA_SIZE);
    if (test_data == NULL || read_buffer == NULL) {
        err_sys("malloc failed");
    }
    fill_test_data(test_data, TEST_DATA_SIZE);
    
    gettimeofday(&start, NULL);
    
    if ((pid = fork()) < 0) {
        err_sys("fork failed");
    } else if (pid == 0) {
        /* Processo figlio - legge */
        close(pipefd[1]); /* Chiude write end */
        
        for (i = 0; i < NUM_ITERATIONS; i++) {
            if (read(pipefd[0], read_buffer, TEST_DATA_SIZE) != TEST_DATA_SIZE) {
                err_sys("read failed");
            }
            
            if (!verify_test_data(read_buffer, TEST_DATA_SIZE)) {
                fprintf(stderr, "Data verification failed at iteration %d\n", i);
            }
        }
        
        close(pipefd[0]);
        exit(0);
    } else {
        /* Processo padre - scrive */
        close(pipefd[0]); /* Chiude read end */
        
        for (i = 0; i < NUM_ITERATIONS; i++) {
            test_data[0] = (char)(i % 256);
            if (write(pipefd[1], test_data, TEST_DATA_SIZE) != TEST_DATA_SIZE) {
                err_sys("write failed");
            }
        }
        
        close(pipefd[1]);
        wait(NULL);
        gettimeofday(&end, NULL);
        
        result.elapsed_time = get_time_diff(&start, &end);
        result.throughput_mbps = (TEST_DATA_SIZE * NUM_ITERATIONS) / 
                                (result.elapsed_time * 1024.0);
    }
    
    free(test_data);
    free(read_buffer);
    
    return result;
}

/*
 * Test Named Pipe (FIFO)
 */
static benchmark_result_t test_pipe_named(void)
{
    benchmark_result_t result = {0.0, 0.0, "Named Pipe (FIFO)"};
    struct timeval start, end;
    pid_t pid;
    int fd_read, fd_write;
    int i;
    char *test_data;
    char *read_buffer;
    
    printf("Testing Named Pipe (FIFO)...\n");
    
    /* Crea FIFO */
    unlink(FIFO_NAME); /* Rimuovi se esiste */
    if (mkfifo(FIFO_NAME, 0666) == -1) {
        err_sys("mkfifo failed");
    }
    
    test_data = malloc(TEST_DATA_SIZE);
    read_buffer = malloc(TEST_DATA_SIZE);
    if (test_data == NULL || read_buffer == NULL) {
        err_sys("malloc failed");
    }
    fill_test_data(test_data, TEST_DATA_SIZE);
    
    gettimeofday(&start, NULL);
    
    if ((pid = fork()) < 0) {
        err_sys("fork failed");
    } else if (pid == 0) {
        /* Processo figlio - legge */
        if ((fd_read = open(FIFO_NAME, O_RDONLY)) < 0) {
            err_sys("open FIFO for read failed");
        }
        
        for (i = 0; i < NUM_ITERATIONS; i++) {
            if (read(fd_read, read_buffer, TEST_DATA_SIZE) != TEST_DATA_SIZE) {
                err_sys("read from FIFO failed");
            }
            
            if (!verify_test_data(read_buffer, TEST_DATA_SIZE)) {
                fprintf(stderr, "Data verification failed at iteration %d\n", i);
            }
        }
        
        close(fd_read);
        exit(0);
    } else {
        /* Processo padre - scrive */
        if ((fd_write = open(FIFO_NAME, O_WRONLY)) < 0) {
            err_sys("open FIFO for write failed");
        }
        
        for (i = 0; i < NUM_ITERATIONS; i++) {
            test_data[0] = (char)(i % 256);
            if (write(fd_write, test_data, TEST_DATA_SIZE) != TEST_DATA_SIZE) {
                err_sys("write to FIFO failed");
            }
        }
        
        close(fd_write);
        wait(NULL);
        gettimeofday(&end, NULL);
        
        result.elapsed_time = get_time_diff(&start, &end);
        result.throughput_mbps = (TEST_DATA_SIZE * NUM_ITERATIONS) / 
                                (result.elapsed_time * 1024.0);
    }
    
    free(test_data);
    free(read_buffer);
    
    return result;
}

/*
 * Test Socket Pair
 */
static benchmark_result_t test_socket_pair(void)
{
    benchmark_result_t result = {0.0, 0.0, "Socket Pair (UNIX Domain)"};
    struct timeval start, end;
    int sockfd[2];
    pid_t pid;
    int i;
    char *test_data;
    char *read_buffer;
    
    printf("Testing Socket Pair (UNIX Domain)...\n");
    
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockfd) == -1) {
        err_sys("socketpair failed");
    }
    
    test_data = malloc(TEST_DATA_SIZE);
    read_buffer = malloc(TEST_DATA_SIZE);
    if (test_data == NULL || read_buffer == NULL) {
        err_sys("malloc failed");
    }
    fill_test_data(test_data, TEST_DATA_SIZE);
    
    gettimeofday(&start, NULL);
    
    if ((pid = fork()) < 0) {
        err_sys("fork failed");
    } else if (pid == 0) {
        /* Processo figlio - legge */
        close(sockfd[1]);
        
        for (i = 0; i < NUM_ITERATIONS; i++) {
            if (read(sockfd[0], read_buffer, TEST_DATA_SIZE) != TEST_DATA_SIZE) {
                err_sys("read from socket failed");
            }
            
            if (!verify_test_data(read_buffer, TEST_DATA_SIZE)) {
                fprintf(stderr, "Data verification failed at iteration %d\n", i);
            }
        }
        
        close(sockfd[0]);
        exit(0);
    } else {
        /* Processo padre - scrive */
        close(sockfd[0]);
        
        for (i = 0; i < NUM_ITERATIONS; i++) {
            test_data[0] = (char)(i % 256);
            if (write(sockfd[1], test_data, TEST_DATA_SIZE) != TEST_DATA_SIZE) {
                err_sys("write to socket failed");
            }
        }
        
        close(sockfd[1]);
        wait(NULL);
        gettimeofday(&end, NULL);
        
        result.elapsed_time = get_time_diff(&start, &end);
        result.throughput_mbps = (TEST_DATA_SIZE * NUM_ITERATIONS) / 
                                (result.elapsed_time * 1024.0);
    }
    
    free(test_data);
    free(read_buffer);
    
    return result;
}

/*
 * Stampa risultati benchmark
 */
static void print_results(benchmark_result_t *results, int num_results)
{
    int i;
    
    printf("\n=== RISULTATI BENCHMARK IPC ===\n");
    printf("Test Data Size: %.2f MB\n", TEST_DATA_SIZE / (1024.0 * 1024.0));
    printf("Iterations: %d\n", NUM_ITERATIONS);
    printf("Total Data Transferred per Test: %.2f MB\n\n", 
           (TEST_DATA_SIZE * NUM_ITERATIONS) / (1024.0 * 1024.0));
    
    printf("%-30s %12s %15s\n", "Method", "Time (ms)", "Throughput (KB/s)");
    printf("%-30s %12s %15s\n", "------", "---------", "----------------");
    
    for (i = 0; i < num_results; i++) {
        if (results[i].elapsed_time > 0) {
            printf("%-30s %12.2f %15.2f\n", 
                   results[i].method_name,
                   results[i].elapsed_time,
                   results[i].throughput_mbps);
        } else {
            printf("%-30s %12s %15s\n", 
                   results[i].method_name, "FAILED", "N/A");
        }
    }
    
    printf("\n");
}

/*
 * Cleanup file temporanei
 */
static void cleanup_files(void)
{
    unlink(FIFO_NAME);
    unlink(MMAP_FILE);
}

/*
 * Funzione principale
 */
int main(void)
{
    benchmark_result_t results[5];
    int num_tests = 0;
    
    printf("=== BENCHMARK IPC: MEMORY MAPPING vs PIPE ===\n");
    printf("Testando diverse tecniche di comunicazione inter-processo...\n\n");
    
    /* Cleanup file precedenti */
    cleanup_files();
    
    /* Esegui test Memory Mapping */
    results[num_tests++] = test_mmap_shared_anonymous();
    results[num_tests++] = test_mmap_file_backed();
    
    /* Esegui test Pipe */
    results[num_tests++] = test_pipe_anonymous();
    results[num_tests++] = test_pipe_named();
    results[num_tests++] = test_socket_pair();
    
    /* Stampa risultati */
    print_results(results, num_tests);
    
    /* Cleanup finale */
    cleanup_files();
    
    printf("Benchmark completato.\n");
    return 0;
}