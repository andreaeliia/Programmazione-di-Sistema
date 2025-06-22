#include "apue.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/wait.h>

#define SHM_NAME "/shared_counter"

//Programma per la creazione di due processi che condividono un numero
int main(void){
    int shm_fd;
    int *shared_counter;
    pid_t pid;


    //Creazione segmento memoria condivisa
    shm_fd = shm_open(SHM_NAME,O_CREAT | O_RDWR,0666);
    if(shm_fd == -1){
        err_sys("shm_open failed");
    }


    //Impostare la dimensione
    if(ftruncate(shm_fd,sizeof(int))== -1){
        err_sys("ftruncate failed");
    }


    //Mappare la memoria
    shared_counter = mmap(NULL,sizeof(int), PROT_READ | PROT_WRITE , MAP_SHARED, shm_fd,0);
    if(shared_counter == MAP_FAILED){
        err_sys("mmap failed");
    }

    //inizializza a 0 
    *shared_counter = 0 ;


    pid = fork();

    if(pid == 0){
        //processo figlio
        for (int i = 0; i < 5; i++)
        {
            (*shared_counter)++;
            printf("Figlio counter = %d\n",*shared_counter);
            sleep(1);
        }
        exit(0);
    }else if (pid>0)
    {
        for (int i = 0; i <5; i++)
        {
            
            (*shared_counter) +=10;
            printf("Padre: counter = %d\n",*shared_counter);
            sleep(1);
        }

        wait(NULL); //Comaqndo per aspettare che un  processo figlio muoia

        printf("Valore finale = %d\n",*shared_counter);

        //PULIZIA
        munmap(shared_counter,sizeof(int));
        close(shm_fd);
        shm_unlink(SHM_NAME);
    }else{
        err_sys("fork failed");
    }


    return 0 ;

}