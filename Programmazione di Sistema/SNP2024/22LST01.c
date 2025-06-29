#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

static int count = 0;

static void
catch_sigint(int signo) {
    ++count;
    write(1,"CATTURATO SIGINT!\n",18);
}

int
main(int argc,char *argv[]) {
    struct sigaction sa_old;
    struct sigaction sa_new;

    sa_new.sa_handler = catch_sigint;
    sigemptyset(&sa_new.sa_mask);
    sa_new.sa_flags = 0;
    sigaction(SIGINT,&sa_new,&sa_old);

    puts("STARTED:");

    do  {
        sleep(1);
    } while ( count < 3 );

    puts("ENDED.");
    return 0;
}

