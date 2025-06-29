#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

static const int s = 23;    /* Static var */

extern int main             /* Main appears later */
    (int argc,char *const argv[],char *const envp[]);

/*
 * Dump item's address to be sorted :
 */
void                        /* This must be extern */
dumpAddr(FILE *p,const char *desc,const void *addr) {

    fprintf(p,"0x%08lX  %-32s\t%lu\n",
        (long)addr, desc, (unsigned long)addr);
}

/*
 * Splat out unprintable characters :
 */
static void
mkPrintable(char *buf) {

    for ( ; *buf; ++buf )
        if ( *buf <= ' ' || *buf >= 0x7F )
            *buf = '@';
}

/*
 * Test (static) Function 2 :
 */
static void
fun2(FILE *p) {
    int f2;

    dumpAddr(p,"&f2",&f2);
}

/*
 * Test (extern) Function 1 :
 */
void
fun1(FILE *p) {
    int f1;

    dumpAddr(p,"&f1",&f1);
    fun2(p);
}

/*
 * Dump the environment + various selected
 * addresses :
 *
 * ARGUMENTS:
 *      envp        Pointer to the argument (from main)
 *      heading     Display heading comment
 */
void
dump(char * const envp[],const char *heading) {
    int x;          /* Indexes envp[x] */
    char *cp;       /* Work pointer */
    char buf[32];   /* Local buffer */
    FILE *p = 0;    /* Pipe FILE pointer */

    /*
     * Open a pipe to sort all address by
     * decreasing address, so that the last
     * displayed address will be the lowest
     * addressed item:
     */
    if ( !(p = popen("sort -n -r -k3","w")) ) {
        fprintf(stderr,"%s: popen('sort ...');\n",
            strerror(errno));
        exit(13);
    }

    /*
     * Display a heading:
     */
    printf("\n%s\n\n",heading);
    fflush(stdout);

    /*
     * Display environment variables:
     */
    for ( x=0; envp[x] != 0; ++x ) {
	buf[0] = '&';           /* Put '&' in 1st col. */
        strncpy(buf+1,envp[x],sizeof buf-2);
        buf[sizeof buf - 1] = 0;/* Make sure we have a nul */
        mkPrintable(buf);       /* Splat out special chars */
        dumpAddr(p,buf,envp[x]);/* Dump this var's address */

        /*
         * Special test for TERM variable:
         * Here we discard the '=' and what follows.
         */
	if ( (cp = strchr(buf,'=')) != 0 )
            *cp = 0;

        /*
         * Test if envp[x] is the "TERM" entry:
         */
	if ( !strcmp(buf+1,"TERM")      /* Got "TERM" ?    */
	&&   (cp = getenv("TERM")) != 0 /* "TERM" defined? */
        &&   cp != envp[x]+5 ) {        /* Ptrs mismatch?  */
            strcpy(buf,"@TERM=");       /* Yes, mark this. */
            strncpy(buf+6,cp,sizeof buf-7); /* Copy new val.*/
            buf[sizeof buf - 1] = 0;    /* Enforce nul byte */
            mkPrintable(buf);           /* Splat out       */
            dumpAddr(p,buf,cp);         /* Dump this entry */
	}
    }   /* End for () */

    dumpAddr(p,"&envp[0]",&envp[0]);    /* Start of envp[] */
    sprintf(buf,"&envp[%u]",x);
    dumpAddr(p,buf,&envp[x]);        /* Dump end of envp[] */

    dumpAddr(p,"&s",&s);            /* Dump static &s addr */
    dumpAddr(p,"&x",&x);            /* Dump auto &x addr   */

    fun1(p);                        /* Call upon fun1()    */

    dumpAddr(p,"stderr",stderr);    /* Dump stderr address */
    dumpAddr(p,"stdout",stdout);    /* Dump stdout address */
    dumpAddr(p,"stdin",stdin);      /* Dump stdin address  */
    
    dumpAddr(p,"fprintf",fprintf);  /* Dump fprintf addr.  */
    dumpAddr(p,"main",main);        /* Dump main's addr.   */
    dumpAddr(p,"dump",dump);        /* Dump dump's addr.   */
    dumpAddr(p,"fun1",fun1);        /* Dump fun1's addr.   */
    dumpAddr(p,"fun2",fun2);        /* Dump fun2's addr.   */

    if ( (cp = getenv("NVAR1")) != 0 ) /* Is NVAR1 defined?*/
        dumpAddr(p,"&NVAR1",cp);       /* Dump &NVAR1      */

    if ( (cp = getenv("NVAR2")) != 0 ) /* Is NVAR2 defined?*/
        dumpAddr(p,"&NVAR2",cp);       /* Dump &NVAR2      */

    /*
     * By closing this pipe now, we'll start sort on its
     * merry way, sorting our results. The sort output will
     * be sent to standard output.
     */
    pclose(p);
}

/*
 * The main program: 
 */
int
main(int argc,char * const argv[],char * const envp[]) {
	
    /*
     * This dump() call will dump our unmodified environment:
     */
    dump(envp,"INITIAL ENVIRONMENT:");

    /*
     * Here we'll create two new environment variables
     * NVAR1 and NVAR2.
     */
    if ( putenv("NVAR1=New Variable") == -1 ) {
        fprintf(stderr,"%s: putenv()\n", strerror(errno));
        exit(13);
    }
    if ( putenv("NVAR2=New Variable 2") == -1 ) {
        fprintf(stderr,"%s: putenv()\n", strerror(errno));
        exit(13);
    }
    sleep(5);   /* per avere il tempo di invocare vmmap */

    /*
     * Here we change the value of our TERM variable, to
     * see what changes take place for an existing exported
     * environment variable here:
     */
    if ( putenv("TERM=oink-term") == -1 ) {
        fprintf(stderr,"%s: putenv()\n", strerror(errno));
        exit(13);
    }

    /*
     * Now that we've modified our environment, let us
     * see what havoc we wreaked:
     */
    dump(envp,"MODIFIED ENVIRONMENT:");
    sleep(1);   /* This waits for the sort output */

    /*
     * This simple test just proves that the new value
     * for TERM has been changed.
     */
    printf("\ngetenv('TERM') = '%s';\n",getenv("TERM"));
    return 0;
}
