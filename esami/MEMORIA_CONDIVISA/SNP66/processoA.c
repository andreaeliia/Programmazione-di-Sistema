

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <semaphore.h>
#include <signal.h>
#include <time.h>


#define SEM_NAME "/semaphore"
#define FILENAME "shared_file.txt"
#define FILE_SIZE 4096
#define MAX_RECORD_SIZE 200

/* ========== GESTIONE SEGNALI ========== */
void signal_handler(int sig) {
    printf("\n[Processo A] Ricevuto segnale %d. Terminazione...\n", sig);
    continua_esecuzione = 0;
}


int main(){

}