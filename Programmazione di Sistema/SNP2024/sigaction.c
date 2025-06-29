/*
 * Copyright (C) ANTRIX Inc.
 */

/* sigaction.c
 * This program allows the calling process to specify and examine
 * the action associated with signal using function sigaction.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

void handler(int signal)
{
  if ( signal == SIGALRM )
     fprintf (stdout,"received signal SIGALRM\n");
  else
     fprintf (stderr,"ERROR: expected=%d, received=%d\n",
                             SIGALRM, signal);
}

int main()
{
  int     sigaction_value;
  struct  sigaction  act, oact;

  fprintf (stdout,"example 1\n");
  fprintf (stdout,"set the signal\n");
  sigemptyset (&act.sa_mask);
  sigemptyset (&oact.sa_mask);
  sigaddset (&act.sa_mask, SIGALRM);
  act.sa_handler = handler;
  act.sa_flags =  SA_ONSTACK;
  sigaction_value = sigaction (SIGALRM, &act, &oact);
  if ( sigaction_value == 0 ) {
     fprintf (stdout,"sigaction() call successful\n");
     sleep(3);
     kill(getpid(), SIGALRM);
  }
  else
     fprintf (stderr,"ERROR: sigaction() call failed\n");

  fprintf (stdout,"example 2\n");
  fprintf (stdout,"examine the signal\n");
  sigemptyset (&act.sa_mask);
  sigemptyset (&oact.sa_mask);
  sigaddset (&act.sa_mask, SIGALRM);
  act.sa_handler = handler;
  act.sa_flags =  SA_ONSTACK;
  sigaction_value = sigaction (SIGALRM, &act, &oact);
  if ( sigaction_value == 0 ) {
    fprintf (stdout,"sigaction() call successful\n");
    sigaction (SIGALRM, &act, &oact);
    if ( sigismember(&oact.sa_mask, SIGALRM) == 1 )
      fprintf (stdout,"previous signal set was SIGALRM\n");
    else
      fprintf (stderr,"ERROR: did not reterive previous signal SIGALRM\n");
    if ( oact.sa_handler == handler )
      fprintf (stdout,"previous signal function set was handler\n");
    else
      fprintf (stderr,"ERROR: did not reterive previous signal handler\n");
  }
  else
     fprintf (stdout,"error: sigaction() call failed\n") ;
}
