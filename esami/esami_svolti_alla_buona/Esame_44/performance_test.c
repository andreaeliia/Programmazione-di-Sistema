/*
 * Test automatico delle prestazioni
 * Confronta server thread-based vs process-based
 */

#include "apue.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>

#define MAXLINE 4096
#define THREAD_PORT 8080
#define PROCESS_PORT 8081
#define MAX_CLIENTS 200

/* Struttura per risultati del test */
struct test_result {
    int num_clients;
    double total_time;
    double avg_time;
    double clients_per_sec;
};

/* Prototipi delle funzioni */
static pid_t start_server(const char *server_prog, int port);
static void stop_server(pid_t pid);
static struct test_result run_client_test(int port, int num_clients);
static int connect_and_request(int port);
static double get_time_diff(struct timeval start, struct timeval end);
static void print_results(const char *server_type, struct test_result *results, int num_tests);

/*
 * Funzione principale del test
 */
int main(void)
{
    pid_t thread_server_pid, process_server_pid;
    struct test_result thread_results[4], process_results[4];
    int test_clients[] = {10, 50, 100, 200};
    int i, num_tests = 4;
    
    printf("=== TCP Server Performance Comparison ===\n\n");
    
    /* Avvia entrambi i server */
    printf("Starting thread-based server on port %d...\n", THREAD_PORT);
    thread_server_pid = start_server("./server_thread", THREAD_PORT);
    
    printf("Starting process-based server on port %d...\n", PROCESS_PORT);
    process_server_pid = start_server("./server_process", PROCESS_PORT);
    
    /* Attendi che i server siano pronti */
    sleep(2);
    
    /* Esegui test con diversi numeri di client */
    for (i = 0; i < num_tests; i++) {
        printf("\n--- Testing with %d clients ---\n", test_clients[i]);
        
        /* Test server thread-based */
        printf("Testing thread-based server...\n");
        thread_results[i] = run_client_test(THREAD_PORT, test_clients[i]);
        
        /* Pausa tra test */
        sleep(1);
        
        /* Test server process-based */
        printf("Testing process-based server...\n");
        process_results[i] = run_client_test(PROCESS_PORT, test_clients[i]);
        
        /* Pausa tra test */
        sleep(1);
    }
    
    /* Stampa risultati */
    printf("\n=== RESULTS SUMMARY ===\n");
    print_results("Thread-based", thread_results, num_tests);
    print_results("Process-based", process_results, num_tests);
    
    /* Confronto finale */
    printf("\n=== PERFORMANCE COMPARISON ===\n");
    printf("Clients  | Thread (req/s) | Process (req/s) | Winner\n");
    printf("---------|---------------|----------------|--------\n");
    
    for (i = 0; i < num_tests; i++) {
        const char *winner = thread_results[i].clients_per_sec > 
                           process_results[i].clients_per_sec ? "Thread" : "Process";
        
        printf("%8d | %13.1f | %14.1f | %s\n",
               test_clients[i],
               thread_results[i].clients_per_sec,
               process_results[i].clients_per_sec,
               winner);
    }
    
    /* Termina i server */
    printf("\nStopping servers...\n");
    stop_server(thread_server_pid);
    stop_server(process_server_pid);
    
    return 0;
}

/*
 * Avvia un server e restituisce il PID
 */
static pid_t start_server(const char *server_prog, int port)
{
    pid_t pid;
    char port_str[16];
    
    snprintf(port_str, sizeof(port_str), "%d", port);
    
    if ((pid = fork()) == 0) {
        /* Processo figlio - esegui server */
        execl(server_prog, server_prog, port_str, (char *)NULL);
        err_sys("execl error");
    } else if (pid < 0) {
        err_sys("fork error");
    }
    
    return pid;
}

/*
 * Termina un server
 */
static void stop_server(pid_t pid)
{
    if (kill(pid, SIGTERM) == 0) {
        waitpid(pid, NULL, 0);
    }
}

/*
 * Esegue test con N client concorrenti
 */
static struct test_result run_client_test(int port, int num_clients)
{
    struct test_result result;
    struct timeval start, end;
    pid_t *client_pids;
    int i, status;
    
    result.num_clients = num_clients;
    
    /* Alloca array per PID client */
    client_pids = malloc(num_clients * sizeof(pid_t));
    if (client_pids == NULL) {
        err_sys("malloc error");
    }
    
    /* Avvia cronometro */
    gettimeofday(&start, NULL);
    
    /* Lancia client concorrenti */
    for (i = 0; i < num_clients; i++) {
        if ((client_pids[i] = fork()) == 0) {
            /* Processo figlio - client */
            exit(connect_and_request(port) ? 0 : 1);
        } else if (client_pids[i] < 0) {
            err_sys("fork error");
        }
    }
    
    /* Attendi terminazione di tutti i client */
    for (i = 0; i < num_clients; i++) {
        waitpid(client_pids[i], &status, 0);
    }
    
    /* Ferma cronometro */
    gettimeofday(&end, NULL);
    
    /* Calcola risultati */
    result.total_time = get_time_diff(start, end);
    result.avg_time = result.total_time / num_clients;
    result.clients_per_sec = num_clients / result.total_time;
    
    free(client_pids);
    return result;
}

/*
 * Connette a un server ed esegue una richiesta
 */
static int connect_and_request(int port)
{
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[MAXLINE];
    ssize_t n;
    
    /* Crea socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return 0;
    }
    
    /* Configura indirizzo server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(port);
    
    /* Connetti */
    if (connect(sockfd, (SA *) &server_addr, sizeof(server_addr)) < 0) {
        close(sockfd);
        return 0;
    }
    
    /* Invia richiesta */
    snprintf(buffer, MAXLINE, "Performance test request");
    if (write(sockfd, buffer, strlen(buffer)) < 0) {
        close(sockfd);
        return 0;
    }
    
    /* Ricevi risposta */
    if ((n = read(sockfd, buffer, MAXLINE-1)) <= 0) {
        close(sockfd);
        return 0;
    }
    
    close(sockfd);
    return 1;
}

/*
 * Calcola differenza di tempo in secondi
 */
static double get_time_diff(struct timeval start, struct timeval end)
{
    return (end.tv_sec - start.tv_sec) + 
           (end.tv_usec - start.tv_usec) / 1000000.0;
}

/*
 * Stampa risultati formattati
 */
static void print_results(const char *server_type, struct test_result *results, int num_tests)
{
    int i;
    
    printf("\n%s Server Results:\n", server_type);
    printf("Clients | Total Time (s) | Avg Time (ms) | Clients/sec\n");
    printf("--------|----------------|---------------|------------\n");
    
    for (i = 0; i < num_tests; i++) {
        printf("%7d | %13.3f | %12.1f | %10.1f\n",
               results[i].num_clients,
               results[i].total_time,
               results[i].avg_time * 1000,
               results[i].clients_per_sec);
    }
}