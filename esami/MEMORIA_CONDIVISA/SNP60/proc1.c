/*Tre processi leggono a turno una parola scelta a caso dal file
italian.txt e la depositano in una memoria condivisa separandola con uno
spazio dalle parole precedenti.

I processi sono sincronizzati in modo da massimizzare la velocità di
rotazione e da evitare che le loro scritture si intersechino.

Quando l'utente preme i tasti ^C il contenuto della memoria scritto fino
a quel momento viene stampato al terminale da uno dei processi. */




#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#define MAX_RECORD 200

typedef struct 
{
    record array[MAX_RECORD];
    int items;
}shared_memory_t;

/*======================SHARED_MEMORY=================*/

int init_shared_memory(){
    int shm_fd;

    /*Creiamo e apriamo la memoria condivisa POSIX*/
    shm_fd = shm_open("/shared_data", O_CREAT | O_RDWR,0644);
    if(shm_fd == -1){
        perror("shm_open");
        return -1;
    }

    /*Impostiamo la dimensione*/
    if(ftruncate(shm_fd , sizeof(shared_memory_t))== -1){
        perror("ftruncate");
        return -1;
    }
    /*Mappiamo il file in memoria*/
    shared_memory = mmap(NULL,sizeof(shared_memory_t),
                                    PROT_READ | PROT_WRITE,
                                    MAP_SHARED,shm_fd,0);

    if(shared_memory == MAP_FAILED){
        perror("mmap");
        close(shm_fd);
        return -1;
    }

    close(shm_fd);
    printf(" File mappato in memoria\n");
    return 0;
}


int main(){

}