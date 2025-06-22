#include "apue.h"
#include <pthread.h>

int counter = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  // Provate a decommentare!

void* increment_thread(void* arg) {
    for (int i = 0; i < 100000; i++) {
        pthread_mutex_lock(&mutex);     // Provate a decommentare!
        counter++;
        pthread_mutex_unlock(&mutex);   // Provate a decommentare!
    }
    return NULL;
}

int main(void) {
    pthread_t thread1, thread2;
    
    pthread_create(&thread1, NULL, increment_thread, NULL);
    pthread_create(&thread2, NULL, increment_thread, NULL);
    
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    printf("Counter finale: %d (dovrebbe essere 200000)\n", counter);
    
    return 0;
}