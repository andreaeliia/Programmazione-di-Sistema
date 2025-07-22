#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>

#define MAX_NUM 1000

// Nodo per le code FIFO
typedef struct Node {
    int value;
    struct Node* next;
} Node;

typedef struct {
    Node* head;
    Node* tail;
    int size;
    int max_size;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
} FIFO;

FIFO fifo1, fifo2, non_primes;
volatile sig_atomic_t running = 1;

int* prime_list = NULL;
int prime_count = 0;
pthread_mutex_t prime_mutex = PTHREAD_MUTEX_INITIALIZER;

// --- FIFO Functions ---
void fifo_init(FIFO* f, int max_size) {
    f->head = f->tail = NULL;
    f->size = 0;
    f->max_size = max_size;
    pthread_mutex_init(&f->mutex, NULL);
    pthread_cond_init(&f->not_full, NULL);
    pthread_cond_init(&f->not_empty, NULL);
}

void fifo_push(FIFO* f, int value) {
    pthread_mutex_lock(&f->mutex);
    while (f->size >= f->max_size && running)
        pthread_cond_wait(&f->not_full, &f->mutex);

    Node* new_node = malloc(sizeof(Node));
    new_node->value = value;
    new_node->next = NULL;

    if (f->tail)
        f->tail->next = new_node;
    else
        f->head = new_node;
    f->tail = new_node;
    f->size++;

    pthread_cond_signal(&f->not_empty);
    pthread_mutex_unlock(&f->mutex);
}

int fifo_pop(FIFO* f) {
    pthread_mutex_lock(&f->mutex);
    while (f->size == 0 && running)
        pthread_cond_wait(&f->not_empty, &f->mutex);

    if (!running && f->size == 0) {
        pthread_mutex_unlock(&f->mutex);
        return -1;
    }

    Node* temp = f->head;
    int value = temp->value;
    f->head = temp->next;
    if (f->head == NULL) f->tail = NULL;
    free(temp);
    f->size--;

    pthread_cond_signal(&f->not_full);
    pthread_mutex_unlock(&f->mutex);
    return value;
}

// --- Utility ---
bool is_prime(int n) {
    if (n <= 1) return false;
    for (int i = 2; i*i <= n; i++)
        if (n % i == 0) return false;
    return true;
}

void factorize(int n) {
    printf("Fattori di %d: ", n);
    for (int i = 2; i <= n; i++) {
        while (n % i == 0) {
            printf("%d ", i);
            n /= i;
        }
    }
    printf("\n");
}

// --- Threads ---
void* generator_thread(void* arg) {
    while (running) {
        int num = rand() % MAX_NUM + 1;
        fifo_push(&fifo1, num);
        usleep(100000); // 0.1 sec
    }
    return NULL;
}

void* prime_checker_thread(void* arg) {
    while (running) {
        int num = fifo_pop(&fifo1);
        if (num == -1) break;
        bool prime = is_prime(num);
        if (prime) {
            pthread_mutex_lock(&prime_mutex);
            prime_list = realloc(prime_list, sizeof(int)*(prime_count+1));
            prime_list[prime_count++] = num;
            pthread_mutex_unlock(&prime_mutex);
            fifo_push(&fifo2, num);
        } else {
            fifo_push(&non_primes, num);
        }
    }
    return NULL;
}

void* consumer_thread(void* arg) {
    while (running) {
        int num = fifo_pop(&fifo2);
        if (num == -1) break;
        // Il numero è primo → potresti anche loggarlo o ignorarlo qui.
        // Per ora, non facciamo nulla.
    }
    return NULL;
}

void* factorizer_thread(void* arg) {
    while (running) {
        int num = fifo_pop(&non_primes);
        if (num == -1) break;
        factorize(num);
    }
    return NULL;
}

// --- Signal Handler ---
void handle_sigint(int sig) {
    running = 0;
    // Sbloccare eventuali thread bloccati
    pthread_cond_broadcast(&fifo1.not_empty);
    pthread_cond_broadcast(&fifo2.not_empty);
    pthread_cond_broadcast(&non_primes.not_empty);
    pthread_cond_broadcast(&fifo1.not_full);
    pthread_cond_broadcast(&fifo2.not_full);
    pthread_cond_broadcast(&non_primes.not_full);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <dimensione_fifo>\n", argv[0]);
        return 1;
    }

    int fifo_size = atoi(argv[1]);
    srand(time(NULL));

    fifo_init(&fifo1, fifo_size);
    fifo_init(&fifo2, fifo_size);
    fifo_init(&non_primes, fifo_size);

    signal(SIGINT, handle_sigint);

    pthread_t t1, t2, t3, t4;
    pthread_create(&t1, NULL, generator_thread, NULL);
    pthread_create(&t2, NULL, prime_checker_thread, NULL);
    pthread_create(&t3, NULL, consumer_thread, NULL);
    pthread_create(&t4, NULL, factorizer_thread, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    pthread_join(t4, NULL);

    // Stampa finali
    printf("\nNumeri primi trovati:\n");
    pthread_mutex_lock(&prime_mutex);
    for (int i = 0; i < prime_count; i++) {
        printf("%d ", prime_list[i]);
    }
    printf("\n");
    pthread_mutex_unlock(&prime_mutex);

    free(prime_list);
    return 0;
}