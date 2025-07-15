/*
Creare un daemon che periodicamente effettui una verifica di tutti i file
annidati all'interno di una directory passata come argomento al suo lancio
e verifichi se qualcuno dei file è stato modificato rispetto al momento del
suo lancio, avvisando l'utente se ciò è accaduto.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#include <sys/file.h>
#include <time.h>

/*=================== COSTANTI ===================*/
#define DAEMON_NAME "filemon_daemon"
#define PIDFILE "filemon_daemon.pid"   /*var/run/filedeamon.pid*/
#define LOCKMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)
#define CHECK_INTERVAL 30  /* Secondi tra ogni controllo */

/*=================== VARIBILI GLOBAL ===============*/

static char *monitor_directory = NULL;

/*========================DAEMON================*/
int process_exists(pid_t pid){
    return (kill(pid,0) == 0);
}

int is_already_running(const char *lockfile){
    FILE *fp;
    pid_t pid;

    fp = fopen(lockfile,"r");
    if(fp == NULL){
        /*Il file non esiste*/
        return 0;
    }
    
    if(fscanf(fp , "%d",&pid)!= 1){
        fclose(fp);
        return 0 ; /*file corrotto*/
    }

    fclose(fp);
    return process_exists(pid);

}

void daemonize(const char *cmd){
    

    /*Clear file creation mask*/
    umask(0);  /*con umask specifico che i permessi che voglio dare li devo esplicitare io*/


    /* Become a session leader to lose controlling TTY */
    if ((pid = fork()) < 0) {
        exit(1);
    } else if (pid != 0) { /* parent */
        exit(0);
    }
}





int main(int argc, char *argc[]){

    struct stat st;

        /* Verifica argomenti */
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <directory_da_monitorare>\n", argv[0]);
        exit(1);
    }
    
    monitor_directory = strdup(argv[1]);
    if (!monitor_directory) {
        perror("strdup");
        exit(1);
    }


    /*Verifichiamo che la directory esiste*/
    if(stat(monitor_directory,&st)!= 0 ){
        fprintf(stderr, "Errore: impossibile accedere a %s: %s\n", 
                monitor_directory, strerror(errno));
        exit(1);
    }

    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Errore: %s non è una directory\n", monitor_directory);
        exit(1);
    }


    /*Verifichiamo che il daemon e' gia in esecuzione*/
    if(is_already_running(PIDFILE)){
        fprintf(stderr, "Errore, daemon gia in esecuzione\n");
        exit(1);
    }


    daemonize(DAEMON_NAME);
    



}