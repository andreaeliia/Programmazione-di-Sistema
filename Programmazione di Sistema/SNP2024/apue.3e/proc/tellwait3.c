#include "apue.h"

static void charatatime(char *);

int
main(void)
{
	pid_t	pid;

	TELL_WAIT();

	if ((pid = fork()) < 0) {
		err_sys("fork error");
	} else if (pid == 0) {
		WAIT_PARENT();   /*questa macro è contenuta in apue.h e */
		charatatime("output from child\n");
	} else {
		charatatime("OUTPUT FROM PARENT\n");
		TELL_CHILD(pid);
	}
	exit(0);
}

static void
charatatime(char *str)
{
	char	*ptr;
	int	c;
	struct timespec times;

	setbuf(stdout, NULL);			/* set unbuffered */
	for (ptr = str; (c = *ptr++) != 0; ){
		times.tv_sec = 0;
		times.tv_nsec = 6000;
		nanosleep(&times, NULL);
		putc(c, stdout);
	}
}
