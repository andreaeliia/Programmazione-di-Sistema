#include "apue.h"
#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
int turn = 0;  // 0 = turno PING, 1 = turno PONG

void* ping_thread(void* arg) {
    for (int i = 0; i < 5; i++) {
        pthread_mutex_lock(&mutex);
        
        // TODO: Aspettare il proprio turno (turn == 0)
        while (turn != 0) {
            pthread_cond_wait(&condition, &mutex);
        }
        
        printf("PING\n");
        
        // TODO: Passare il turno a PONG
        turn = 1;
        pthread_cond_signal(&condition);
        
        pthread_mutex_unlock(&mutex);
        sleep(1);
    }
    return NULL;
}

void* pong_thread(void* arg) {
    for (int i = 0; i < 5; i++) {
        pthread_mutex_lock(&mutex);
        
        // TODO: Aspettare il proprio turno (turn == 1)
        while (turn != 1) {
            pthread_cond_wait(&condition, &mutex);
        }
        
        printf("PONG\n");
        
        // TODO: Passare il turno a PING
        turn = 0;
        pthread_cond_signal(&condition);
        
        pthread_mutex_unlock(&mutex);
        sleep(3);
    }
    return NULL;
}

int main(void) {
    pthread_t ping, pong;
    
    pthread_create(&ping, NULL, ping_thread, NULL);
    pthread_create(&pong, NULL, pong_thread, NULL);
    
    pthread_join(ping, NULL);
    pthread_join(pong, NULL);
    
    return 0;
}