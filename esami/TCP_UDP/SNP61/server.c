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


typedef struct{
	char **file_paths;
	int end;
	int file_count;
	int capacity;
}path_list;

/*========================Struct function=============*/
path_list* init_path_list(){
	path_list* list = malloc(sizeof(path_list));
	list->file_paths = malloc(3000*sizeof(char*));
	list->end = 0;
	list->file_count = 0;
	list->capacity = 3000;

	return list;
}


void add_file(path_list* list,const char* pathname){




	if(list->file_count >= list->capacity){
		list->capacity *=2;
		list->file_paths = realloc(list->file_paths,list->capacity * sizeof(char*));
	}
	list->file_paths[list->file_count] =  malloc(strlen(pathname)+1);
	strcpy(list->file_paths[list->file_count],pathname);
	list->file_count++;

}

/*==============SERVER UDP================*/

int send_udp(int server_fd,struct sockaddr_in client_addr,socklen_t client_len,char* pathname){
		
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
static int		myftw(char *, Myfunc *,path_list*);
static int		dopath(Myfunc *,path_list*);



path_list* file_in_path(char* pathname) {
    int ret;


	path_list* list;

	list = init_path_list();
    ret = myftw(pathname, myfunc,list);  /* Questo ora popola anche la lista! */

   
    
    
    if (ret != 0) {
        err_quit("Errore durante la scansione della directory");
    }

	add_file(list,"end");

	return list;
}
/*
 * Descend through the hierarchy, starting at "pathname".
 * The caller's func() is called for every file.
 */
#define	FTW_F	1		/* file other than directory */
#define	FTW_D	2		/* directory */
#define	FTW_DNR	3		/* directory that can't be read */
#define	FTW_NS	4		/* file that we can't stat */


/*
	define per i return value

*/
#define CONTINUE_SCAN 0
#define FILE_FOUND    2
#define ERROR_STOP   -1

static char	*fullpath;		/* contains full pathname for every file */
static size_t pathlen;

static int					/* we return whatever func() returns */
myftw(char *pathname, Myfunc *func,path_list* list)
{
	fullpath = path_alloc(&pathlen);	/* malloc PATH_MAX+1 bytes */
										/* ({Prog pathalloc}) */
	if (pathlen <= strlen(pathname)) {
		pathlen = strlen(pathname) * 2;
		if ((fullpath = realloc(fullpath, pathlen)) == NULL)
			err_sys("realloc failed");
	}
	strcpy(fullpath, pathname);
	return(dopath(func,list));
}

/*
 * Descend through the hierarchy, starting at "fullpath".
 * If "fullpath" is anything other than a directory, we lstat() it,
 * call func(), and return.  For a directory, we call ourself
 * recursively for each name in the directory.
 */

static int dopath(Myfunc* func, path_list* list)
{
	struct stat		statbuf;
	struct dirent	*dirp;
	DIR				*dp;
	int				ret, n;

	if (lstat(fullpath, &statbuf) < 0) {
		ret = func(fullpath, &statbuf, FTW_NS);
		if (ret == FILE_FOUND) {
			return CONTINUE_SCAN;
		} else {
			return ret;
		}
	}
	
	if (S_ISDIR(statbuf.st_mode) == 0) {
		ret = func(fullpath, &statbuf, FTW_F);
		if (ret == FILE_FOUND) {
			add_file(list, fullpath);  /* Aggiungi il file alla lista! */
			return CONTINUE_SCAN;      /* Continua la scansione */
		}
		return ret;
	}

	/* È una directory */
	ret = func(fullpath, &statbuf, FTW_D);
	if (ret != 0) {
		if (ret == FILE_FOUND) {
			/* Non dovrebbe succedere per le directory, ma gestiamo il caso */
			return CONTINUE_SCAN;
		}
		return ret;
	}

	n = strlen(fullpath);
	if (n + NAME_MAX + 2 > pathlen) {
		pathlen *= 2;
		if ((fullpath = realloc(fullpath, pathlen)) == NULL)
			err_sys("realloc failed");
	}
	fullpath[n++] = '/';
	fullpath[n] = 0;

	if ((dp = opendir(fullpath)) == NULL) {
		ret = func(fullpath, &statbuf, FTW_DNR);
		if (ret == FILE_FOUND) {
			return CONTINUE_SCAN;
		} else {
			return ret;
		}
	}

	while ((dirp = readdir(dp)) != NULL) {
		if (strcmp(dirp->d_name, ".") == 0  || strcmp(dirp->d_name, "..") == 0)
				continue;
		strcpy(&fullpath[n], dirp->d_name);
		ret = dopath(func, list);  /* Passa la lista ricorsivamente */
		if (ret != 0) {/*time to leave*/
			break;
		}
	
	}
	fullpath[n-1] = 0;
	

	if (closedir(dp) < 0)
		err_ret("can't close directory %s", fullpath);
	return ret;
}


static int
myfunc(const char *pathname, const struct stat *statptr, int type)
{
	switch (type) {
	case FTW_F:
    
        if(statptr->st_size > size_limit){
		
            printf("%s\n",pathname);
			printf("----------------\n");
			return FILE_FOUND;
        }
	
		break;
	case FTW_D:
		return CONTINUE_SCAN;
		
	case FTW_DNR:
		err_ret("can't read directory %s", pathname);
		return CONTINUE_SCAN;
		break;
	case FTW_NS:
		err_ret("stat error for %s", pathname);
		return CONTINUE_SCAN;
		break;
	default:
		err_dump("unknown type %d for pathname %s", type, pathname);
	}
	return CONTINUE_SCAN;
}


int main() {
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
	path_list* list;
	int i;

    
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
        list = file_in_path("/mnt/c/Users/recre/Programmazione-di-Sistema");

        /* Invia risposta */


        for ( i = 0; i <= list->file_count-1; i++)
		{
			send_udp(server_fd,client_addr,client_len,list->file_paths[i]);

			sleep(1);
		}
		
        
	}

	
    
    close(server_fd);
    return 0;
}