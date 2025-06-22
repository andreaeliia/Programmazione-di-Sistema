#include "apue.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h>

#define SHM_NAME "/char_buffer"
#define BUFFER_SIZE 100



typedef struct {
    char buffer[BUFFER_SIZE];
    int write_index;
    int process_a_can_write;
    int process_b_can_write;
    pid_t process_a_pid;
    pid_t process_b_pid;

}shared_data_t;


shared_data_t *shared_data;

int init_shared_memory(){
    int shm_fd;

    //creazione ed apertura memoria condivisa
    shm_fd = shm_open(SHM_NAME,O_CREAT | O_RDWR, 0666);
    if(shm_fd == -1){
        perror("shm_open");
        return -1;
    }

    //Impostare la dimensione
    if(ftruncate(shm_fd,sizeof(shared_data_t))== - 1){
        perror("ftruncate");
        return -1;
    }

    //Mappare la memoria
    shared_data = mmap(NULL,sizeof(shared_data_t),PROT_READ | PROT_WRITE, MAP_SHARED,shm_fd,0);
    if(shared_data == MAP_FAILED){
        perror("mmap");
        return -1;
    }

    close(shm_fd);
    return 0;

}

int main(void){
    if(init_shared_memory<0){
        exit(1);
    }

    //Iniziallizzazione della struttura
    shared_data->write_index =0;
    shared_data->process_a_can_write = 1;
    shared_data->process_b_can_write = 0;
    shared_data ->process_a_pid =0;
    shared_data -> process_b_pid = 0;

    printf("Memoria condivisa inizializzata");
    printf("Buffer size : %d\n",BUFFER_SIZE);

        // Pulizia
    munmap(shared_data, sizeof(shared_data_t));
    shm_unlink(SHM_NAME);
}