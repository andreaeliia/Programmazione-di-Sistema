/* msg_process1.c - Processo 1 per versione code messaggi */
#include "token_ring.h"

/* Variabili globali per cleanup */
static int g_my_qid = -1;
static int g_next_qid = -1;

/* Gestore segnali per cleanup */
static void signal_handler(int sig) {
    cleanup_message_queues();
    exit(0);
}

/* Crea coda messaggi */
int create_message_queue(int process_id) {
    key_t key;
    int qid;
    
    key = MSG_KEY_BASE + process_id;
    qid = msgget(key, IPC_CREAT | 0666);
    if (qid == -1) {
        err_sys("msgget error");
    }
    
    return qid;
}

/* Ottiene coda messaggi esistente */
int get_message_queue(int process_id) {
    key_t key;
    int qid;
    
    key = MSG_KEY_BASE + process_id;
    qid = msgget(key, 0666);
    if (qid == -1) {
        err_sys("msgget error");
    }
    
    return qid;
}

/* Invia token alla coda successiva */
void send_token_msg(int qid, msg_token_t *token, int next_process) {
    token->mtype = MSG_TYPE;
    token->process_id = next_process;
    
    if (msgsnd(qid, token, sizeof(msg_token_t) - sizeof(long), 0) == -1) {
        err_sys("msgsnd error");
    }
}

/* Riceve token dalla propria coda */
int receive_token_msg(int qid, msg_token_t *token) {
    if (msgrcv(qid, token, sizeof(msg_token_t) - sizeof(long), MSG_TYPE, 0) == -1) {
        return -1;
    }
    return 0;
}

/* Cleanup code messaggi */
void cleanup_message_queues(void) {
    if (g_my_qid != -1) {
        msgctl(g_my_qid, IPC_RMID, NULL);
    }
}

/* Calcola differenza temporale */
double get_time_diff(struct timeval *start, struct timeval *end) {
    return (end->tv_sec - start->tv_sec) + 
           (end->tv_usec - start->tv_usec) / 1000000.0;
}

/* Stampa risultati benchmark */
void print_benchmark_results(int process_id, int tokens_processed, double elapsed_time) {
    double tokens_per_sec = tokens_processed / elapsed_time;
    double rotations_per_sec = tokens_per_sec / 3.0; /* 3 processi per rotazione */
    
    printf("Processo %d (MSG): %d token in %.2f sec = %.2f token/sec (%.2f rotazioni/sec)\n",
           process_id, tokens_processed, elapsed_time, tokens_per_sec, rotations_per_sec);
}

/* Funzione principale */
int main(void) {
    msg_token_t token;
    struct timeval start_time, current_time;
    int tokens_processed = 0;
    double elapsed_time;
    
    /* Installa gestore segnali */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("Processo 1 (MSG): Avvio...\n");
    
    /* Crea la propria coda messaggi */
    g_my_qid = create_message_queue(1);
    
    /* Attende che gli altri processi creino le loro code */
    printf("Processo 1 (MSG): Attendo altri processi...\n");
    sleep(2);
    
    /* Ottiene la coda del processo successivo */
    g_next_qid = get_message_queue(2);
    
    /* Inizializza token */
    memset(&token, 0, sizeof(token));
    token.counter = 0;
    token.process_id = 1;
    strcpy(token.data, "TOKEN_DATA_PAYLOAD_FOR_BENCHMARK");
    
    printf("Processo 1 (MSG): Avvio token ring per %d secondi...\n", TEST_DURATION);
    
    /* Invia token iniziale */
    send_token_msg(g_next_qid, &token, 2);
    tokens_processed++;
    
    /* Inizio benchmark */
    gettimeofday(&start_time, NULL);
    
    /* Loop principale */
    do {
        /* Riceve token */
        if (receive_token_msg(g_my_qid, &token) == 0) {
            token.counter++;
            tokens_processed++;
            
            /* Invia al processo successivo */
            send_token_msg(g_next_qid, &token, 2);
        }
        
        gettimeofday(&current_time, NULL);
        elapsed_time = get_time_diff(&start_time, &current_time);
        
    } while (elapsed_time < TEST_DURATION);
    
    /* Stampa risultati */
    print_benchmark_results(1, tokens_processed, elapsed_time);
    
    /* Cleanup */
    cleanup_message_queues();
    
    return 0;
}