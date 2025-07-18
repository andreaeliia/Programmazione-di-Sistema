/***********************************************************************
* Code listing from "Advanced Linux Programming," by CodeSourcery LLC  *
* Copyright (C) 2001 by New Riders Publishing                          *
* See COPYRIGHT for license information.                               *
***********************************************************************/

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

sig_atomic_t sigusr1_count = 0;

void handler (int signal_number)
{
  ++sigusr1_count;
}

int main ()
{
  struct sigaction sa;
  memset (&sa, 0, sizeof (sa));
  sa.sa_handler = &handler;
  sigaction (SIGUSR1, &sa, NULL);
  sleep(250);
	
  /* Do some lengthy stuff here.  */
  /* Attenzione: la sleep esce appena arriva un qualunque segnale   */
  /* http://man7.org/linux/man-pages/man7/signal.7.html   */
  /* Si pu� risolvere in vari modi  (es un loop) */

  printf ("SIGUSR1 was raised %d times\n", sigusr1_count);
  return 0;
}
