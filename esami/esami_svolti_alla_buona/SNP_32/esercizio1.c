/*
 * Esercizio 1: Due processi comunicano tra loro mappando lo stesso file 
 * nelle rispettive memorie. Verificare se dopo la chiusura del file descriptor
 * i processi possono ancora comunicare.
 * 
 * Autore: Generato automaticamente
 * Compilazione: make esercizio1
 */

#include "apue.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

#define FILENAME "shared_file.tmp"
#define FILESIZE 4096

/* Struttura dati condivisa */
struct shared_data {
    int counter;
    char message[256];
    int process_done[2]; /* Flag per indicare quando un processo ha finito */
};

/*
 * Funzione per creare e inizializzare il file condiviso
 * Ritorna il file descriptor o -1 in caso di errore
 */
int create_shared_file(void)
{
    int fd;
    struct shared_data init_data = {0, "Inizializzazione", {0, 0}};
    
    /* Crea il file */
    if ((fd = open(FILENAME, O_CREAT | O_RDWR | O_TRUNC, 0644)) < 0) {
        err_sys("Errore nella creazione del file");
    }
    
    /* Estende il file alla dimensione desiderata */
    if (lseek(fd, FILESIZE - 1, SEEK_SET) == -1) {
        err_sys("Errore lseek");
    }
    
    if (write(fd, "", 1) != 1) {
        err_sys("Errore write");
    }
    
    /* Inizializza i dati */
    if (lseek(fd, 0, SEEK_SET) == -1) {
        err_sys("Errore lseek reset");
    }
    
    if (write(fd, &init_data, sizeof(struct shared_data)) != sizeof(struct shared_data)) {
        err_sys("Errore inizializzazione dati");
    }
    
    return fd;
}

/*
 * Funzione del processo figlio
 * Mappa il file, comunica con il padre, chiude il fd e continua a comunicare
 */
void child_process(int fd)
{
    struct shared_data *shared;
    int i;
    
    printf("FIGLIO: Avvio processo figlio (PID: %d)\n", getpid());
    
    /* Mappa il file in memoria */
    shared = mmap(NULL, sizeof(struct shared_data), PROT_READ | PROT_WRITE, 
                  MAP_SHARED, fd, 0);
    if (shared == MAP_FAILED) {
        err_sys("FIGLIO: Errore mmap");
    }
    
    printf("FIGLIO: File mappato in memoria\n");
    
    /* Comunica con il padre prima di chiudere il fd */
    for (i = 0; i < 3; i++) {
        shared->counter++;
        snprintf(shared->message, sizeof(shared->message), 
                 "Messaggio dal figlio #%d (fd aperto)", i + 1);
        printf("FIGLIO: Inviato messaggio con fd aperto: %s\n", shared->message);
        sleep(1);
    }
    
    /* CHIUDE il file descriptor */
    printf("FIGLIO: Chiudo il file descriptor\n");
    if (close(fd) < 0) {
        err_sys("FIGLIO: Errore chiusura fd");
    }
    
    /* Continua a comunicare DOPO aver chiuso il fd */
    printf("FIGLIO: Continuo a comunicare dopo chiusura fd...\n");
    for (i = 0; i < 3; i++) {
        shared->counter++;
        snprintf(shared->message, sizeof(shared->message), 
                 "Messaggio dal figlio #%d (fd CHIUSO!)", i + 4);
        printf("FIGLIO: Inviato messaggio con fd chiuso: %s\n", shared->message);
        sleep(1);
    }
    
    /* Segnala la fine */
    shared->process_done[1] = 1;
    printf("FIGLIO: Processo figlio terminato\n");
    
    /* Smappa la memoria */
    if (munmap(shared, sizeof(struct shared_data)) < 0) {
        err_sys("FIGLIO: Errore munmap");
    }
    
    exit(0);
}

/*
 * Funzione del processo padre  
 * Mappa il file, comunica con il figlio, chiude il fd e continua a comunicare
 */
void parent_process(int fd, pid_t child_pid)
{
    struct shared_data *shared;
    int i;
    int status;
    
    printf("PADRE: Avvio processo padre (PID: %d)\n", getpid());
    
    /* Mappa il file in memoria */
    shared = mmap(NULL, sizeof(struct shared_data), PROT_READ | PROT_WRITE, 
                  MAP_SHARED, fd, 0);
    if (shared == MAP_FAILED) {
        err_sys("PADRE: Errore mmap");
    }
    
    printf("PADRE: File mappato in memoria\n");
    
    /* Aspetta un po' e poi inizia a comunicare */
    sleep(2);
    
    /* Comunica con il figlio prima di chiudere il fd */
    for (i = 0; i < 3; i++) {
        printf("PADRE: Letto dal figlio (fd aperto): %s (counter=%d)\n", 
               shared->message, shared->counter);
        sleep(1);
    }
    
    /* CHIUDE il file descriptor */
    printf("PADRE: Chiudo il file descriptor\n");
    if (close(fd) < 0) {
        err_sys("PADRE: Errore chiusura fd");
    }
    
    /* Continua a leggere DOPO aver chiuso il fd */
    printf("PADRE: Continuo a leggere dopo chiusura fd...\n");
    while (shared->process_done[1] == 0) {
        printf("PADRE: Letto dal figlio (fd CHIUSO!): %s (counter=%d)\n", 
               shared->message, shared->counter);
        sleep(1);
    }
    
    printf("PADRE: Il figlio ha terminato\n");
    
    /* Aspetta la terminazione del figlio */
    if (waitpid(child_pid, &status, 0) < 0) {
        err_sys("PADRE: Errore waitpid");
    }
    
    printf("PADRE: Processo padre terminato\n");
    
    /* Smappa la memoria */
    if (munmap(shared, sizeof(struct shared_data)) < 0) {
        err_sys("PADRE: Errore munmap");
    }
}

/*
 * Funzione principale
 */
int main(void)
{
    int fd;
    pid_t pid;
    
    printf("=== ESERCIZIO 1: Comunicazione tramite Memory Mapping ===\n");
    printf("Verifica comunicazione dopo chiusura file descriptor\n\n");
    
    /* Crea il file condiviso */
    fd = create_shared_file();
    printf("File condiviso creato: %s\n", FILENAME);
    
    /* Fork */
    if ((pid = fork()) < 0) {
        err_sys("Errore fork");
    } else if (pid == 0) {
        /* Processo figlio */
        child_process(fd);
    } else {
        /* Processo padre */
        parent_process(fd, pid);
    }
    
    /* Rimuove il file temporaneo */
    unlink(FILENAME);
    
    printf("\n=== RISULTATO SPERIMENTALE ===\n");
    printf("I processi possono comunicare anche dopo aver chiuso il file descriptor!\n");
    printf("Il memory mapping mantiene la condivisione della memoria.\n");
    
    return 0;
}