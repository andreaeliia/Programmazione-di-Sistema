/*
Scrivere nel linguaggio C un programma client di nome A e un programma 
server di nome B che funzionino nel modo seguente:

1) Il programma A chiede all'utente di scrivere un numero intero.

2) Il programma A invia con un datagramma UDP tale numero al 
programma B.

3) Il programma B trova tutti i file presenti nei volumi montati nella
macchina in cui gira la cui dimensione in bytes è superiore al numero 
ricevuto e per ciascuno di essi invia il percorso assoluto in un 
datagramma UDP al programma A, concludendo la lista con un datagramma
contenente la scritta "end".

4) Il programma A stampa al terminale la lista dei percorsi ricevuti. 

La coppia di programmi può essere provata usando l'indirizzo
localhost all'interno della stessa macchina.

I due programmi non devono usare le chiamate system() e popen().*/
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>



#define PORT 8080
#define BUFFER_SIZE 1024
/*=================VARIABILI GLOBALI===============*/
long long size_limit;


/*==============SERVER UDP================*/

int send_udp(int server_fd,sockaddr_in client_addr,socklen_t client_len,char* pathname){
		
		char response[BUFFER_SIZE];
        snprintf(response, BUFFER_SIZE, "%s", pathname);
        
        if(sendto(server_fd, response, strlen(response), 0,
               (struct sockaddr*)&client_addr, client_len)>0){
			return 0;
			
		}else{
			return -1;
		}
	}


/*========================ESEMPI COPIATi=====================*/
/* function type that is called for each filename */
typedef	int Myfunc(const char *, const struct stat *, int);

static Myfunc	myfunc;
static int		myftw(char *, Myfunc *);
static int		dopath(Myfunc *);



void file_in_path(char* pathname) {
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
    
        if(statptr->st_size > size_limit){
		
            printf("%s\n",pathname);
			printf("----------------\n");
			return 0;
        }
	
		break;
	case FTW_D:
		return -1;
		
	case FTW_DNR:
		return -1;
		
	case FTW_NS:
		
		return -1;
	default:
		err_dump("unknown type %d for pathname %s\n", type, pathname);
	}
	return(0);
}


int main() {
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    
    /* 1. Crea socket UDP */
    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(1);
    }
    
    /* 2. Configura indirizzo server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    /* 3. Bind socket */
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(1);
    }
    
    printf("Server UDP in ascolto sulla porta %d\n", PORT);
    
    /* 4. Loop principale */
        while (1) {
        /* Ricevi messaggio */
        int bytes_received = recvfrom(server_fd, buffer, BUFFER_SIZE - 1, 0,
                                     (struct sockaddr*)&client_addr, &client_len);
        
        if (bytes_received < 0) {
            perror("recvfrom failed");
            continue;
        }
        
        buffer[bytes_received] = '\0';
        printf("Ricevuto da %s:%d: %s\n", 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port), 
               buffer);
        
        /*Prendere il numero dal buffer*/
        size_limit = atoll(buffer);
		

		
        /*Qua mettere la parte di trovare il mnt*/
        file_in_path("/mnt/c/Users/recre/Programmazione-di-Sistema");

        /* Invia risposta */
        char response[BUFFER_SIZE];
        snprintf(response, BUFFER_SIZE, "Echo: %s", buffer);
        
        sendto(server_fd, response, strlen(response), 0,
               (struct sockaddr*)&client_addr, client_len);
        
	}

	
    
    close(server_fd);
    return 0;
}