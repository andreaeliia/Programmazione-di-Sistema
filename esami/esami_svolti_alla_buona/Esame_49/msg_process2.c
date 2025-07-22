/* msg_process2.c - Processo 2 per versione code messaggi */
#include "token_ring.h"

/* Variabili globali */
static int g_my_qid = -1;
static int g_next_qid = -1;

/* Gestore segnali */
static void signal_handler(int sig) {
    if (g_my_qid != -1) {
        msgctl(g_my_qid, IPC_RMID, NULL);
    }
    exit(0);
}

/* Funzione principale */
int main(void) {
    msg_token_t token;
    struct timeval start_time, current_time;
    int tokens_processed = 0;
    double elapsed_time;
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("Processo 2 (MSG): Avvio...\n");
    
    /* Crea propria coda */
    g_my_qid = create_message_queue(2);
    
    /* Attende altri processi */
    sleep(3);
    
    /* Ottiene coda processo successivo */
    g_next_qid = get_message_queue(3);
    
    printf("Processo 2 (MSG): Pronto per token ring...\n");
    
    /* Attende primo token per iniziare benchmark */
    receive_token_msg(g_my_qid, &token);
    gettimeofday(&start_time, NULL);
    
    do {
        token.counter++;
        tokens_processed++;
        
        /* Invia al processo 3 */
        send_token_msg(g_next_qid, &token, 3);
        
        /* Riceve token successivo */
        if (receive_token_msg(g_my_qid, &token) != 0) {
            break;
        }
        
        gettimeofday(&current_time, NULL);
        elapsed_time = get_time_diff(&start_time, &current_time);
        
    } while (elapsed_time < TEST_DURATION);
    
    print_benchmark_results(2, tokens_processed, elapsed_time);
    
    /* Cleanup */
    if (g_my_qid != -1) {
        msgctl(g_my_qid, IPC_RMID, NULL);
    }
    
    return 0;
}