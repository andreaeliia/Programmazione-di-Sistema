#include "Header.h"
#include "apue.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

/* map3 and map4 test whether a modification made by a process in a mapped file
are seen in the memory mapping of the same file in another process.
To run the test, give to both program as argument the name of a file containing a string of 'A' letters. */

int
main(int argc, char *argv[])
{
	int			fdin,fdlk,fdwr;
	void		*src;
	char 		*p;
	struct stat	statbuf;
	

	if ((fdin = open("/Users/patryk/Desktop/universita/Magistrale/PrimoAnno/Sistemi/Esempi/apue.3e/Esempi/EserciziEsame/Esame54/FileCondiviso.txt", O_RDWR|O_CREAT|O_TRUNC,0666)) < 0)
			err_sys("can't open FileCondiviso for writing");
			
	if ((fdlk = open("/Users/patryk/Desktop/universita/Magistrale/PrimoAnno/Sistemi/Esempi/apue.3e/Esempi/EserciziEsame/Esame54/FileDiLock.txt", O_RDWR|O_CREAT,0666)) < 0)
			err_sys("can't open FileDiLock");
	
	if ((fdwr = open("/Users/patryk/Desktop/universita/Magistrale/PrimoAnno/Sistemi/Esempi/apue.3e/Esempi/EserciziEsame/Esame54/ListaContatti.txt", O_RDWR|O_CREAT|O_TRUNC|O_APPEND,0666)) < 0)
			err_sys("can't open FileDiLock");
			
	int quit = 0;
	
	while (!quit){
	
	writew_lock(fdlk, 0, SEEK_SET, 1);
	writew_lock(fdin, 0, SEEK_SET, BUFSIZE * 2);
	
	if ((src = mmap(0, BUFSIZE * 2, PROT_WRITE, MAP_SHARED,fdin, 0)) == MAP_FAILED)
		err_sys("mmap error for input");
	    p=src;
	    
	    write(fdwr,p,BUFSIZE*2);
	    
	un_lock(fdin, 0, SEEK_SET, BUFSIZE*2);
	un_lock(fdlk, 0, SEEK_SET, 1);
	
	
	}
	
	
	exit(0);
}
