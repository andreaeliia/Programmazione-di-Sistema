#include <time.h>
#include <pthread.h>
#include "apue.h"
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtx2 = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
static pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;
pthread_t thread1,thread2;

typedef	int	Myfunc( char *, const struct stat *, int);

 struct fileNameIno {
	char * path;
	ino_t inode;
};

int numberOfFiles = 1;

 struct fileNameIno * arrayFiles;  

int ret;
static Myfunc	myfunc;
static int		myftw(char *, Myfunc *);
static int		dopath(Myfunc *);
char * mypath;

static void *
findReg(void *arg)
{

	printf("\n\n\n\n\n\n\n\n\n!!! RUN THE FIRST THREAD !!! \n\n\n");	
    ret = myftw(mypath, myfunc);
    
    int s = pthread_cond_signal(&cond1); 
            if (s != 0)
                err_sys("pthread_cond_signal");

    return NULL;
}

static void *
findInode(void *arg)
{
	pthread_cond_wait(&cond1,&mtx);
	
	printf("!!! RUN THE SECOND THREAD !!! \n\n\n");	
    
    for(int i = 0; i < numberOfFiles - 1; i++){
    
    	printf("\nThe files that inode is %d are: \n", arrayFiles[i].inode);
    
    	for(int j = 0; j < numberOfFiles - 1; j++ ){
    		
    		if(arrayFiles[i].inode == arrayFiles[j].inode)
    		printf("-%s\n",arrayFiles[j].path);
    		
    	}
    	printf("\n");
    }
    
    
    int s = pthread_cond_signal(&cond2); 
            if (s != 0)
                err_sys("pthread_cond_signal");

    return NULL;
}



int main(int argc, char *argv[])
{
		
		if(argc != 2){
		err_sys( "Number of args");
		}
		
		arrayFiles = ( struct fileNameIno *)malloc(sizeof(struct fileNameIno) * numberOfFiles);
		mypath = (char*)argv[1];
          

    	
        int s = pthread_create(&thread1, NULL, findReg, NULL);
        if (s != 0)
            err_sys( "pthread1_create");
            
        s = pthread_create(&thread2, NULL, findInode, NULL);
        if (s != 0)
            err_sys( "pthread2_create");
            
        
        pthread_cond_wait(&cond2,&mtx2);
   


         

    exit(EXIT_SUCCESS);
}


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
		return(ret);

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
myfunc(char *pathname, const struct stat *statptr, int type)
{

	switch (type) {
	case FTW_F:
		switch (statptr->st_mode & S_IFMT) {
		case S_IFREG:	
		if(statptr->st_nlink >= 1){
		int nlinks = (int)statptr->st_nlink;
		printf("\n-%s",pathname);
		printf("\n WITH N-LINSK %d \n",nlinks);
		arrayFiles = realloc(arrayFiles,sizeof(struct fileNameIno)*numberOfFiles);
		arrayFiles[numberOfFiles - 1].path = pathname;
		arrayFiles[numberOfFiles - 1].inode = statptr->st_ino;
		numberOfFiles++;
		}
			break;
		
		}
		break;
	}
	//printf("%s \n", pathname);
	return(0);
}

