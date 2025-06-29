#include <stdio.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <signal.h>




static void del_semvalue(void);


static int sem_id;



int main(){
	
	pid_t	pid1,pid2,pid3,pid4;

	if ((pid1 = fork()) < 0) {
		printf("Errore fork 1");
			return 1;
	} else if (pid1 == 0) {	/* specify pathname, specify environment */
		if (execl("./coda1", "coda1",(char *)0) < 0){
			printf("Errore exec 1");
			return 1;
			}
	}
	
	if ((pid2 = fork()) < 0) {
		printf("Errore fork 2");
			return 1;
	} else if (pid2 == 0) {	/* specify pathname, specify environment */
		if (execl("./coda2", "coda2",(char *)0) < 0){
			printf("Errore exec 2");
			return 1;
			}
	}
	
	if ((pid3 = fork()) < 0) {
		printf("Errore fork 3");
			return 1;
	} else if (pid3 == 0) {	/* specify pathname, specify environment */
		if (execl("./coda3", "coda3",(char *)0) < 0){
			printf("Errore exec 3");
			return 1;
			}
	}
	
	if ((pid4 = fork()) < 0) {
		printf("Errore fork 4");
			return 1;
	} else if (pid3 == 0) {	/* specify pathname, specify environment */
		if (execl("./coda4", "coda4",(char *)0) < 0){
			printf("Errore exec 4");
			return 1;}
	}
	
	if(pid4 != 0 && pid1 !=0 && pid2 !=0 && pid3 !=0){
	sleep(100);
	kill(pid1,9);
	kill(pid2,9);
	kill(pid3,9);
	kill(pid4,9);
	
	sem_id = semget((key_t)1234, 1, 0666 | IPC_CREAT);
	del_semvalue();
    
	   
    return 0;
    }
}







/* The del_semvalue function has almost the same form, except the call to semctl uses
 the command IPC_RMID to remove the semaphore's ID. */

static void del_semvalue(void)
{
    union semun sem_union;
    
    if (semctl(sem_id, 0, IPC_RMID, sem_union) == -1)
        fprintf(stderr, "Failed to delete semaphore\n");
}





