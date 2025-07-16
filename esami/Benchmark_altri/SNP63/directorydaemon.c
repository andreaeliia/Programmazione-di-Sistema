/*
Creare   ppppp un daemon che periodicamente effettui una verifica di tutti i file
annidati all'interno di una directory passata come argomento al suo lancio
e verifichi se qualcuno dei file è stato modificato rispetto al momento del
suo lancio, avvisando l'utente se ciò è accaduto.
*/
#include "apue.h"
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
#include <dirent.h>       
#include <limits.h>       
#include <signal.h>       
#include <string.h>       
#include <time.h>         
#include <unistd.h>       
#include <sys/wait.h>     

/*=================== COSTANTI ===================*/
#define DAEMON_NAME "filemon_daemon"
#define PIDFILE "/mnt/c/Users/recre/Programmazione-di-Sistema/esami/Benchmark_altri/SNP63/filemon_daemon.pid"   /*var/run/filedeamon.pid*/
#define LOCKMODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)
#define CHECK_INTERVAL 30  /* Secondi tra ogni controllo */
#define PATH_SIZE 1024;


typedef struct file_info{
    off_t st_size; /*size in bytes*/
    char path_name[1024];
    time_t last_modified; /*last_modified*/
    struct file_info* next;
} FileInfo;

FileInfo* all_file;
/*=================== VARIBILI GLOBAL ===============*/

static char *monitor_directory = NULL;
static int lockfd = -1;
static volatile sig_atomic_t running = 1;
static FileInfo* file_list = NULL;


/*===============LINKED_LIST===================*/
void add_file_to_list(const char* path, off_t size, time_t mtime) {
    FileInfo* new_file = (FileInfo*)malloc(sizeof(FileInfo));
    if (new_file == NULL) {
        err_sys("malloc failed");
        return;
    }
    
    strncpy(new_file->path_name, path, sizeof(new_file->path_name) - 1);
    new_file->path_name[sizeof(new_file->path_name) - 1] = '\0';
    new_file->st_size = size;
    new_file->last_modified = mtime;
    new_file->next = file_list;
    
    file_list = new_file;
}

int check_file_modified(const char* path) {
    struct stat st;
    FileInfo* current = file_list;
    
    /* Cerca il file nella lista */
    while (current != NULL) {
        if (strcmp(current->path_name, path) == 0) {
            /* File trovato, controlla se modificato */
            if (stat(path, &st) == 0) {
                if (st.st_mtime != current->last_modified || 
                    st.st_size != current->st_size) {
                    return 1;  /* File modificato */
                }
            }
            return 0;  /* File non modificato */
        }
        current = current->next;
    }
    return -1;  /* File non trovato nella lista */
}


void initialize_file_list(const char* directory) {
    /* Qui useresti la funzione myftw del tuo codice */
    /* per attraversare ricorsivamente la directory */
    
    /* Esempio semplificato: */
    struct stat st;
    if (stat(directory, &st) == 0) {
        add_file_to_list(directory, st.st_size, st.st_mtime);
    }
}

void check_all_files() {
    FileInfo* current = file_list;
    int modified_count = 0;
    
    while (current != NULL) {
        int result = check_file_modified(current->path_name);
        if (result == 1) {
           
            syslog(LOG_WARNING, "File modificato: %s", current->path_name);
            modified_count++;
            
            /* Aggiorna le informazioni del file */
            struct stat st;
            if (stat(current->path_name, &st) == 0) {
                current->st_size = st.st_size;
                current->last_modified = st.st_mtime;
            }
        } else if (result == -1) {
           
            syslog(LOG_WARNING, "File non trovato: %s", current->path_name);
        }
        current = current->next;
    }
    
    if (modified_count > 0) {
        syslog(LOG_INFO, "Trovati %d file modificati", modified_count);
    } else {
        syslog(LOG_DEBUG, "Nessun file modificato");
    }
}

void cleanup_file_list() {
    FileInfo* current = file_list;
    while (current != NULL) {
        FileInfo* temp = current;
        current = current->next;
        free(temp);
    }
    file_list = NULL;
}

/*=====================DIRECTORY RICORSIVAMENTE==========*/
/* function type that is called for each filename */
typedef	int	Myfunc(const char *, const struct stat *, int);

static Myfunc	myfunc;
static int		myftw(char *, Myfunc *);
static int		dopath(Myfunc *);

static long	nreg, ndir, nblk, nchr, nfifo, nslink, nsock, ntot;

void all_file_inpath(char* pathname) {
    int ret;

    ret = myftw(pathname, myfunc);  /* Questo ora popola anche la lista! */

    ntot = nreg + ndir + nblk + nchr + nfifo + nslink + nsock;
    
    if (ntot == 0)
        ntot = 1;
    
   
    syslog(LOG_INFO, "File trovati: regular=%ld, directories=%ld", nreg, ndir);
    
    
    if (ret != 0) {
        err_quit("Errore durante la scansione della directory");
    }
}
/*
 * Descend through the hierarchy, starting at "pathname".
 * The caller's func() is called for every file.
 */
#define	FTW_F	1		/* file other than directory */
#define	FTW_D	2		/* directory */
#define	FTW_DNR	3		/* directory that can't be read */
#define	FTW_NS	4		/* file that we can't stat */

static char	*fullpath;		/* contains full pathname for every file */
static size_t pathlen;

static int					/* we return whatever func() returns */
myftw(char *pathname, Myfunc *func)
{
	fullpath = path_alloc(&pathlen);	/* malloc PATH_MAX+1 bytes */
										/* ({Prog pathalloc}) */
	if (pathlen <= strlen(pathname)) {
		pathlen = strlen(pathname) * 2;
		if ((fullpath = realloc(fullpath, pathlen)) == NULL)
			err_sys("realloc failed");
	}
	strcpy(fullpath, pathname);
	return(dopath(func));
}

/*
 * Descend through the hierarchy, starting at "fullpath".
 * If "fullpath" is anything other than a directory, we lstat() it,
 * call func(), and return.  For a directory, we call ourself
 * recursively for each name in the directory.
 */
static int					/* we return whatever func() returns */
dopath(Myfunc* func)
{
	struct stat		statbuf;
	struct dirent	*dirp;
	DIR				*dp;
	int				ret, n;

	if (lstat(fullpath, &statbuf) < 0)	/* stat error */
		return(func(fullpath, &statbuf, FTW_NS));
	if (S_ISDIR(statbuf.st_mode) == 0)	/* not a directory */
		return(func(fullpath, &statbuf, FTW_F));

	/*
	 * It's a directory.  First call func() for the directory,
	 * then process each filename in the directory.
	 */
	if ((ret = func(fullpath, &statbuf, FTW_D)) != 0)
		return(ret); /*Errore */

	n = strlen(fullpath);
	if (n + NAME_MAX + 2 > pathlen) {	/* expand path buffer */
		pathlen *= 2;
		if ((fullpath = realloc(fullpath, pathlen)) == NULL)
			err_sys("realloc failed");
	}
	fullpath[n++] = '/';
	fullpath[n] = 0;


	if ((dp = opendir(fullpath)) == NULL)	/* can't read directory */
		return(func(fullpath, &statbuf, FTW_DNR));

	while ((dirp = readdir(dp)) != NULL) {
		if (strcmp(dirp->d_name, ".") == 0  ||
		    strcmp(dirp->d_name, "..") == 0)
				continue;		/* ignore dot and dot-dot */
		strcpy(&fullpath[n], dirp->d_name);	/* append name after "/" */
		if ((ret = dopath(func)) != 0)		/* recursive */
			break;	/* time to leave */
	}
	fullpath[n-1] = 0;	/* erase everything from slash onward */

	if (closedir(dp) < 0)
		err_ret("can't close directory %s", fullpath);
	return(ret);
}

static int
myfunc(const char *pathname, const struct stat *statptr, int type)
{
	switch (type) {
	case FTW_F:
    add_file_to_list(pathname,statptr->st_size,statptr->st_mtime);
		switch (statptr->st_mode & S_IFMT) {
		case S_IFREG:	nreg++;	
                                	break;
		case S_IFBLK:	nblk++;		break;
		case S_IFCHR:	nchr++;		break;
		case S_IFIFO:	nfifo++;	break;
		case S_IFLNK:	nslink++;	break;
		case S_IFSOCK:	nsock++;	break;
		case S_IFDIR:	/* directories should have type = FTW_D */
			err_dump("for S_IFDIR for %s", pathname);
		}
		break;
	case FTW_D:
		ndir++;
		break;
	case FTW_DNR:
		err_ret("can't read directory %s", pathname);
		break;
	case FTW_NS:
		err_ret("stat error for %s", pathname);
		break;
	default:
		err_dump("unknown type %d for pathname %s", type, pathname);
	}
	return(0);
}





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

    fd = open(lockfile, O_RDWR | O_CREAT, LOCKMODE);
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


            check_all_files();

            last_check = current_time;
            


            










            sleep(1);
        }
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



int main(int argc, char *argv[]){

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

    printf("Scansionando directory iniziale: %s\n", monitor_directory);
    all_file_inpath(monitor_directory);  /* Popola la lista */


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

    return 0;

}