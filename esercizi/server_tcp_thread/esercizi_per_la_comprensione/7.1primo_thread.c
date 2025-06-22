#include "apue.h"
#include <pthread.h>


void* worked_thread(void* arg){
    int thread_num = *((int*)arg);

    for (int i = 0; i < 5; i++)
    {
        printf("Thread %d: interazione %d\n",thread_num,i);
        sleep(1);
    }

    return NULL;
    
}

int main(void){
    pthread_t thread1,thread2;
    int id1 =1;
    int id2 = 2;


    printf("Creazione thread....\n");
    //TODO: Creare thread 1 con id1

    pthread_create(&thread1,NULL,worked_thread,&id1);
    pthread_create(&thread2,NULL,worked_thread,&id2);

    //TODO: Aspettare che entrambi finiscono 

    pthread_join(thread1,NULL);
    pthread_join(thread2,NULL);


    printf("Tutti i thread sono finiti \n");

    return 0 ;
    
}