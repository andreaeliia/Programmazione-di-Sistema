/*
Filename: ATEST22TLS.c
Downloaded from: IBM Knowledge center
http://www.ibm.com/support/knowledgecenter/ssw_ibm_i_73/rzahw/rzahwex1.htm
The output of this example is as follows:
 Enter Testcase - LIBRARY/ATEST22TLS
 Create/start threads
 Wait for the threads to complete, and release their resources
 Thread 0000000000000036: Entered
 Thread 0000000000000037: Entered
 Thread 0000000000000036: foo(), TLS data=0 2
 Thread 0000000000000036: bar(), TLS data=0 2
 Thread 0000000000000037: foo(), TLS data=1 4
 Thread 0000000000000037: bar(), TLS data=1 4
 Main completed
*/
#define _MULTI_THREADED
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void foo(void);  /* Functions that use the TLS data */
void bar(void);

#define checkResults(string, val) {             \
 if (val) {                                     \
   printf("Failed with %d at %s", val, string); \
   exit(1);                                     \
 }                                              \
}

__thread int TLS_data1;
__thread int TLS_data2;

#define                 NUMTHREADS   8

typedef struct {
   int   data1;
   int   data2;
} threadparm_t;

void *theThread(void *parm)
{
   int               rc;
   threadparm_t     *gData;

   printf("Thread %.16lx: Entered\n", pthread_self());

   gData = (threadparm_t *)parm;

   TLS_data1 = gData->data1;
   TLS_data2 = gData->data2;

   foo();
   return NULL;
}

void foo() {
   printf("Thread %.16lx: foo(), TLS data=%d %d\n",
          pthread_self(), TLS_data1, TLS_data2);
   bar();
}

void bar() {
   printf("Thread %.16lx: bar(), TLS data=%d %d\n",
          pthread_self(), TLS_data1, TLS_data2);
   return;
}


int main(int argc, char **argv)
{
  pthread_t             thread[NUMTHREADS];
  int                   rc=0;
  int                   i;
  threadparm_t          gData[NUMTHREADS];

  printf("Enter Testcase - %s\n", argv[0]);

  printf("Create/start threads\n");
  for (i=0; i < NUMTHREADS; i++) {
     /* Create per-thread TLS data and pass it to the thread */
     gData[i].data1 = i;
     gData[i].data2 = (i+1)*2;
     rc = pthread_create(&thread[i], NULL, theThread, &gData[i]);
     checkResults("pthread_create()\n", rc);
  }

  printf("Wait for the threads to complete, and release their resources\n");
  for (i=0; i < NUMTHREADS; i++) {
     rc = pthread_join(thread[i], NULL);
     checkResults("pthread_join()\n", rc);
  }

  printf("Main completed\n");
  return 0;
}
