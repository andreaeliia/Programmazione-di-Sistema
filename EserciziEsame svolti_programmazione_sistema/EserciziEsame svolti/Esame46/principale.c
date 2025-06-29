#include "apue.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>

int		fd;
char previus;
char toWrite = '0';
char current;
int counter = 0;
static pthread_mutex_t mtx;
bool exit1 = false;


static void *                   /* Loop 'arg' times incrementing 'glob' */
threadFunc(void *arg)
{	int s;
	int n;
	int pos;
	
	while(exit1 == false){
		
		s = pthread_mutex_lock(&mtx);
        if (s != 0){
            printf("pthread_mutex_lock");
            break;
            }
            
            n = read(fd,&current,1);
            if(n <= 0){
            	exit1 = 1;
            	break;
            	}
            if ((pos = lseek(fd, -1, SEEK_CUR)) == -1)
			err_sys("lseek error");
			write(fd,&toWrite,1);
            if(previus == current){
            counter ++;
            }
            else{
            	counter = 0;
            	}
            previus = current;
            if(counter >= 2){
            	printf("Trovate 3 simboli uguali nella posizione %d \n",(pos + 1));
            }	
        
        s = pthread_mutex_unlock(&mtx);
        if (s != 0){
            printf("pthread_mutex_unlock");
            break;
			  
        }
	}  
	pthread_exit(NULL); 
}

int main(void)
{	int s;
	pthread_t t1, t2;
	
	if ((fd = open("/Users/patryk/Desktop/universita/Magistrale/PrimoAnno/Sistemi/Esempi/apue.3e/Esempi/EserciziEsame/Esame46/dump.txt",O_RDWR, FILE_MODE)) < 0)
		err_sys("creat error");
		
	read(fd,&previus,1);
	
	if (lseek(fd, -1, SEEK_CUR) == -1)
		err_sys("lseek error");
	write(fd,&toWrite,1);
	
	s = pthread_mutex_init(&mtx,NULL);
	if (s != 0){
        printf("mutex_create");
        return 1;
        }
	
	s = pthread_create(&t1, NULL, threadFunc,NULL);
    if (s != 0){
        printf("pthread_create1");
        return 1;
        }
    s = pthread_create(&t2, NULL, threadFunc,NULL);
    if (s != 0) {
    	printf("pthread_create2");
        return 1;
        }
	
	while(exit1 == false){
	
	}
	
	
	

	

	printf("\n\n\nFINE\n\n\n");

	return 0;
}
