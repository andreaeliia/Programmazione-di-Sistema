/*Scrivere un programma C che prenda come argomento un comando e lo lanci
per 10 volte scrivendo al termine delle esecuzioni allo standard error
le medie dei tempi di orologio, dei tempi di utente e dei tempi di 
sistema consumati dal comando nei dieci lanci, con la precisione del
microsecondo.*/


#include "apue.h"
#include <sys/times.h>
#include <sys/time.h>    
#include <errno.h>
#include <sys/resource.h>

static void sum_times(double, struct rusage *, struct rusage *, int);
static void print_times();
static void	do_cmd(char *,int);

#define COMMAND_NUMBER 10
/*=================VARIABILI GLOBALI==========*/
double real; /*Tempo reale*/

/*Tempi di sistema*/

double child_user[10];
double child_sys[10];


/*==============UTILS==========*/
double media (double arr[], int size) {
    double totale;
    int i;

    totale = 0.0;
    for ( i = 0; i < size; i++) {
        totale += arr[i];
    }
    return totale / (double)size;
}


int
main(int argc, char *argv[])
{
	int		i;



    real = 0.0;
  
    
    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command>\n", argv[0]);
        exit(1);
    }

	setbuf(stdout, NULL);
	for (i = 0; i < COMMAND_NUMBER; i++){
		do_cmd(argv[1],i);
    }	

    print_times();
	exit(0);
}

static void
do_cmd(char *cmd, int i)
{
    struct timeval start_wall, end_wall;
    struct rusage rusage_before, rusage_after; 
    int status;

    fprintf(stderr,"\ncommand: %s\n", cmd);

    if (gettimeofday(&start_wall, NULL) == -1) 
        err_sys("gettimeofday error");

   
    if (getrusage(RUSAGE_CHILDREN, &rusage_before) == -1)
        err_sys("getrusage before error");

    if ((status = system(cmd)) < 0)
        err_sys("system() error");

    if (gettimeofday(&end_wall, NULL) == -1)  
        err_sys("gettimeofday error");

    
    if (getrusage(RUSAGE_CHILDREN, &rusage_after) == -1)
        err_sys("getrusage after error");

    double wall_time = (end_wall.tv_sec - start_wall.tv_sec) + 
                      (end_wall.tv_usec - start_wall.tv_usec) / 1000000.0;

    sum_times(wall_time, &rusage_before, &rusage_after, i);
}

static void sum_times(double wall_time, struct rusage *before, struct rusage *after, int i)
{
    real += wall_time;
    
   
    child_user[i] = (after->ru_utime.tv_sec - before->ru_utime.tv_sec) + 
                    (after->ru_utime.tv_usec - before->ru_utime.tv_usec) / 1000000.0;
    
    child_sys[i] = (after->ru_stime.tv_sec - before->ru_stime.tv_sec) + 
                   (after->ru_stime.tv_usec - before->ru_stime.tv_usec) / 1000000.0;
}
static void print_times(){

    double media_child_user;
    double media_child_sys;


    media_child_user = media(child_user,COMMAND_NUMBER);
    media_child_sys = media(child_sys,COMMAND_NUMBER);

    fprintf(stderr,"  real:  %7.6f\n", real /COMMAND_NUMBER);
	fprintf(stderr,"  child user:  %7.6f\n",media_child_user);
	fprintf(stderr,"  child sys:   %7.6f\n",media_child_sys);

}



