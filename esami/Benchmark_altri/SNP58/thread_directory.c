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
#include <pthread.h>


typedef struct file_info{
    off_t st_size; /*size in bytes*/
    char path_name[1024];
    nlink_t hard_link;
    struct file_info* next;
    ino_t inode; /*Inode del file*/
} FileInfo;




/*=================== PROTOTIPI FUNZIONI ===============*/
/* Funzioni directory ricorsive */
void all_file_inpath(char* pathname);
static int myfunc(const char *pathname, const struct stat *statptr, int type);

/* Funzioni linked list */
int search_by_inode(FileInfo* head, ino_t inode);
void cleanup_file_list(void);
void add_file_to_list(const char* path, off_t size, nlink_t hard_link, ino_t inode);
void initialize_file_list(const char* directory);

/* Funzioni thread */
void* thread1_func(void* arg);
void* thread2_func(void* arg);

/* Funzioni segnali */
void signal_handler(int sig);

/*=================== VARIBILI GLOBAL ===============*/

static char *monitor_directory = NULL;
static volatile sig_atomic_t running = 1;
static FileInfo* file_list = NULL;

pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
volatile int end_directory = 0;

pthread_t thread1;
pthread_t thread2;

long size_temp =0;
unsigned long long total_bytes = 0; /*Contatore di bytes*/






/*=====================DIRECTORY RICORSIVAMENTE==========*/
/* function type that is called for each filename */
typedef	int	Myfunc(const char *, const struct stat *, int);

static Myfunc	myfunc;
static int		myftw(char *, Myfunc *);
static int		dopath(Myfunc *);

void all_file_inpath(char* pathname) {
    int ret;

    ret = myftw(pathname, myfunc);  /* Questo ora popola anche la lista! */

    
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
    
    add_file_to_list(pathname,statptr->st_size,statptr->st_nlink,statptr->st_ino);
		break;
	case FTW_D:
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


/*===============LINKED_LIST===================*/

/* Cerca un valore nella lista */
int search_by_inode(FileInfo* head, ino_t inode) {
    FileInfo* current = head;
    while (current != NULL) {
        if (current->inode == inode) {
            return 1;
        }
        current = current->next;
    }
    return 0;  /* Non trovato */
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


void add_file_to_list(const char* path, off_t size,nlink_t hard_link,ino_t inode) {
    if(search_by_inode(file_list,inode) == 1){
        return;
    }
    
    
    
    FileInfo* new_file = (FileInfo*)malloc(sizeof(FileInfo));
    if (new_file == NULL) {
        err_sys("malloc failed");
        return;
    }
    
    strncpy(new_file->path_name, path, sizeof(new_file->path_name) - 1);
    new_file->path_name[sizeof(new_file->path_name) - 1] = '\0';
    new_file->st_size = size;
    new_file->hard_link = hard_link;
    new_file->inode = inode;
    new_file->next = file_list;
    
    file_list = new_file;
    
    pthread_mutex_lock(&mutex);
    printf("[THREAD1]\n");
    size_temp = size;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);

    /*PROBELMA CAPIRE SE UTILIZZARE LA FLAG O SLEEP*/

}

void initialize_file_list(const char* directory) {
    /* Reset della lista precedente se esiste */
    cleanup_file_list();
    
    
    
    /* Usa la funzione di attraversamento ricorsivo */
    all_file_inpath((char*)directory);
    
    syslog(LOG_INFO, "Inizializzazione completata per directory: %s", directory);
}







/*====================THREAD=============*/
void* thread1_func(void* arg){
/* una thread che visiti ricorsivamente tutti i nodi all'interno di una 
directory passata come argomento registrando in un'area di memoria, per 
ogni nodo visitato, la dimensione in byte del nodo e il numero di hard 
link ad esso associato;*/

    initialize_file_list(monitor_directory); 
    /* SEGNALA FINE SCANSIONE */
    pthread_mutex_lock(&mutex);
    end_directory = 1;
    pthread_cond_signal(&cond);  /* Sveglia thread2 */
    pthread_mutex_unlock(&mutex);
    
    printf("[THREAD1] Scansione completata!\n"); 
    return NULL;
}

void* thread2_func(void* arg){

    
    while (running)
    {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond,&mutex);
        printf("[THREAD2]\n");
        total_bytes += size_temp;
        size_temp = 0;
        pthread_mutex_unlock(&mutex);
        
        if(end_directory){
            printf("Total bytes Folder : %lld",total_bytes);
            running = 0;
        }

    }    
    return NULL;
}


/*=====================SEGNAlI ===================*/
void signal_handler(int sig) {
    syslog(LOG_INFO, "Ricevuto segnale %d, terminazione...", sig);
    running = 0;
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
    


    

    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGPIPE, SIG_IGN); /*Viene ignorato il segnale SIGPIPE (se crash per esempi un client il daemon si chiude)*/
    
    /*thread*/
    pthread_create(&thread1,NULL,thread1_func,NULL);
    pthread_create(&thread2,NULL,thread2_func,NULL);


    pthread_join(thread1,NULL);
    pthread_join(thread2,NULL);


    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);



    return 0;

}