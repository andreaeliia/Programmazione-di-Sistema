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
#include "apue.h"

/*=================== COSTANTI ===================*/
#define DAEMON_NAME "filemon_daemon"
#define PIDFILE "filemon_daemon.pid"   /*var/run/filedeamon.pid*/
#define LOCKMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)
#define CHECK_INTERVAL 30  /* Secondi tra ogni controllo */

/*=================== VARIBILI GLOBAL ===============*/

static char *monitor_directory = NULL;
static int lockfd = -1;
static char sig_atomic_t running = 1;
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

void daemonize(const char *cmd)
{
	int					i, fd0, fd1, fd2;
	pid_t				pid;
	struct rlimit		rl;
	struct sigaction	sa;

	/*
	 * Clear file creation mask.
	 */
	umask(0);

	/*
	 * Get maximum number of file descriptors.
	 */
	if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
		err_quit("%s: can't get file limit", cmd);

	/*
	 * Become a session leader to lose controlling TTY.
	 */
	if ((pid = fork()) < 0)
		err_quit("%s: can't fork", cmd);
	else if (pid != 0) /* parent */
		exit(0);
	setsid();

	/*
	 * Ensure future opens won't allocate controlling TTYs.
	 */
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIGHUP, &sa, NULL) < 0)
		err_quit("%s: can't ignore SIGHUP", cmd);
	if ((pid = fork()) < 0)
		err_quit("%s: can't fork", cmd);
	else if (pid != 0) /* parent */
		exit(0);

	/*
	 * Change the current working directory to the root so
	 * we won't prevent file systems from being unmounted.
	 */
	if (chdir("/") < 0)
		err_quit("%s: can't change directory to /", cmd);

	/*
	 * Close all open file descriptors.
	 */
	if (rl.rlim_max == RLIM_INFINITY)
		rl.rlim_max = 1024;
	for (i = 0; i < rl.rlim_max; i++)
		close(i);

	/*
	 * Attach file descriptors 0, 1, and 2 to /dev/null.
	 */
	fd0 = open("/dev/null", O_RDWR);
	fd1 = dup(0);
	fd2 = dup(0);

	/*
	 * Initialize the log file.
	 */
	openlog(cmd, LOG_CONS, LOG_DAEMON);
	if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
		syslog(LOG_ERR, "unexpected file descriptors %d %d %d",
		  fd0, fd1, fd2);
		exit(1);
	}
}

int create_lockfile(const char *lockfile){
    int fd;
    char buf[16];

    fp = open(lockfile, O_RDWR | O_CREATE, LOCKMODE);
    if(fd < 0){
        syslog(LOG_ERR, "Impossibile aprire lockfile %s: %s",lockfile,strerror(errno));
        return -1;
    }

    /* Acquisisce lock esclusivo sul file per prevenire race conditions tra processi
    Il lock viene rilasciato automaticamente se il daemon muore/crasha*/
    if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            syslog(LOG_ERR, "Daemon già in esecuzione");
            close(fd);
            return -1;
        }
        syslog(LOG_ERR, "Errore lock: %s", strerror(errno));
        close(fd);
        return -1;
    }

      if (ftruncate(fd, 0) < 0) {
        close(fd);
        return -1;
    }

    sprintf(buf, "%ld\n", (long)getpid());
    if (write(fd, buf, strlen(buf)) != strlen(buf)) {
        close(fd);
        return -1;
    }

    lockfd = fd;
    return 0;

}

void remove_lockfile(const char *lockfile) {
    if (lockfd >= 0) {
        close(lockfd);
        lockfd = -1;
    }
    unlink(lockfile);
}


void daemon_main_loop(){
    time_t last_check;
    time_t current_time; 
    
    last_check= time(NULL);
    syslog(LOG_INFO, "Avvio monitoraggio directory: %s", monitor_directory);
    syslog(LOG_INFO, "Intervallo controllo: %d secondi", CHECK_INTERVAL);
    
    while (running) 
    {
        current_time = time(NULL);

        /*Controlliamo se e' il momento di verificare i file*/
        if(current_time - last_check >= CHECK_INTERVAL){
            syslog(LOG_INFO,"Controllo modifiche alla directory\n");




            










            last_check = current_time;
        }
    }
    


/*=====================SEGNAlI ===================*/
void signal_handler(int sig) {
    syslog(LOG_INFO, "Ricevuto segnale %d, terminazione...", sig);
    running = 0;
}

/*====================CLEAN UP==================================*/

void cleanup_and_exit(int status) {
    syslog(LOG_INFO, "Terminazione daemon");
    remove_lockfile(PIDFILE);
    closelog();
    if (monitor_directory) {
        free(monitor_directory);
    }
    exit(status);
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

    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGPIPE, SIG_IGN); /*Viene ignorato il segnale SIGPIPE (se crash per esempi un client il daemon si chiude)*/
    

      if (create_lockfile(PIDFILE) < 0) {
        syslog(LOG_ERR, "Impossibile creare lockfile");
        cleanup_and_exit(1);
    }

        /* Log avvio */
    syslog(LOG_INFO, "Daemon %s avviato (PID: %d)", DAEMON_NAME, getpid());
    syslog(LOG_INFO, "Directory monitorata: %s", monitor_directory);
    
    /* Loop principale del daemon */
    daemon_main_loop();


     /* Cleanup e uscita */
    cleanup_and_exit(0);

}