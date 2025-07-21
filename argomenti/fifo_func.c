/* ========================================= */
/*           FIFO WRITER (INSERIMENTO)      */
/* ========================================= */
/* File: fifo_writer.c */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#define FIFO_NAME "/tmp/my_fifo"
#define BUFFER_SIZE 256

int main()
{
    int pipe_fd;
    int res;
    int open_mode = O_WRONLY;
    char message[BUFFER_SIZE];
    int counter = 1;
    
    /* Creazione della FIFO se non esistente */
    if (access(FIFO_NAME, F_OK) == -1) {
        res = mkfifo(FIFO_NAME, 0666);
        if (res != 0) {
            fprintf(stderr, "Errore: impossibile creare FIFO %s\n", FIFO_NAME);
            exit(EXIT_FAILURE);
        }
        printf("FIFO creata: %s\n", FIFO_NAME);
    }

    printf("Writer %d: apertura FIFO in modalita' scrittura...\n", getpid());
    pipe_fd = open(FIFO_NAME, open_mode);
    
    if (pipe_fd == -1) {
        fprintf(stderr, "Errore: impossibile aprire FIFO per scrittura\n");
        exit(EXIT_FAILURE);
    }
    
    printf("Writer %d: FIFO aperta con successo (fd=%d)\n", getpid(), pipe_fd);

    /* Invio di messaggi numerati */
    while (counter <= 10) {
        sprintf(message, "Messaggio numero %d dal processo %d", counter, getpid());
        
        printf("Writer: invio -> %s\n", message);
        res = write(pipe_fd, message, strlen(message) + 1);
        
        if (res == -1) {
            fprintf(stderr, "Errore nella scrittura sulla FIFO\n");
            break;
        }
        
        counter++;
        sleep(2); /* Pausa di 2 secondi tra ogni messaggio */
    }

    /* Invio messaggio di fine */
    strcpy(message, "FINE");
    write(pipe_fd, message, strlen(message) + 1);
    printf("Writer: invio messaggio di terminazione\n");

    close(pipe_fd);
    printf("Writer %d: operazioni completate\n", getpid());
    
    return 0;
}

/* ========================================= */
/*           FIFO READER (RIMOZIONE)        */
/* ========================================= */
/* File: fifo_reader.c */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>

#define FIFO_NAME "/tmp/my_fifo"
#define BUFFER_SIZE 256

int main()
{
    int pipe_fd;
    int res;
    int open_mode = O_RDONLY;
    char buffer[BUFFER_SIZE];
    
    /* Verifica esistenza FIFO */
    if (access(FIFO_NAME, F_OK) == -1) {
        fprintf(stderr, "Errore: FIFO %s non esiste\n", FIFO_NAME);
        fprintf(stderr, "Esegui prima il writer per crearla\n");
        exit(EXIT_FAILURE);
    }

    printf("Reader %d: apertura FIFO in modalita' lettura...\n", getpid());
    pipe_fd = open(FIFO_NAME, open_mode);
    
    if (pipe_fd == -1) {
        fprintf(stderr, "Errore: impossibile aprire FIFO per lettura\n");
        exit(EXIT_FAILURE);
    }
    
    printf("Reader %d: FIFO aperta con successo (fd=%d)\n", getpid(), pipe_fd);
    printf("Reader: in attesa di messaggi...\n\n");

    /* Lettura continua dalla FIFO */
    while (1) {
        res = read(pipe_fd, buffer, BUFFER_SIZE);
        
        if (res == -1) {
            fprintf(stderr, "Errore nella lettura dalla FIFO\n");
            break;
        }
        
        if (res > 0) {
            buffer[res] = '\0'; /* Assicura terminazione stringa */
            
            /* Controlla messaggio di fine */
            if (strcmp(buffer, "FINE") == 0) {
                printf("Reader: ricevuto messaggio di terminazione\n");
                break;
            }
            
            printf("Reader: ricevuto -> %s\n", buffer);
        }
    }

    close(pipe_fd);
    printf("Reader %d: operazioni completate\n", getpid());
    
    return 0;
}

/* ========================================= */
/*         UTILITY PER GESTIRE LA FIFO      */
/* ========================================= */
/* File: fifo_manager.c */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>

#define FIFO_NAME "/tmp/my_fifo"

void print_usage(char* program_name) {
    printf("Uso: %s [create|remove|status]\n", program_name);
    printf("  create - Crea una nuova FIFO\n");
    printf("  remove - Rimuove la FIFO esistente\n");
    printf("  status - Verifica lo stato della FIFO\n");
}

int create_fifo() {
    int res;
    
    if (access(FIFO_NAME, F_OK) != -1) {
        printf("FIFO %s esiste gia'\n", FIFO_NAME);
        return 1;
    }
    
    res = mkfifo(FIFO_NAME, 0666);
    if (res != 0) {
        fprintf(stderr, "Errore: impossibile creare FIFO %s\n", FIFO_NAME);
        return -1;
    }
    
    printf("FIFO creata con successo: %s\n", FIFO_NAME);
    printf("Permessi: rw-rw-rw- (0666)\n");
    return 0;
}

int remove_fifo() {
    if (access(FIFO_NAME, F_OK) == -1) {
        printf("FIFO %s non esiste\n", FIFO_NAME);
        return 1;
    }
    
    if (unlink(FIFO_NAME) == -1) {
        fprintf(stderr, "Errore: impossibile rimuovere FIFO %s\n", FIFO_NAME);
        return -1;
    }
    
    printf("FIFO rimossa con successo: %s\n", FIFO_NAME);
    return 0;
}

int check_fifo_status() {
    struct stat fifo_stat;
    
    if (access(FIFO_NAME, F_OK) == -1) {
        printf("FIFO %s: NON ESISTE\n", FIFO_NAME);
        return 1;
    }
    
    if (stat(FIFO_NAME, &fifo_stat) == -1) {
        fprintf(stderr, "Errore: impossibile ottenere informazioni su %s\n", FIFO_NAME);
        return -1;
    }
    
    printf("FIFO %s: ESISTE\n", FIFO_NAME);
    printf("Tipo: ");
    if (S_ISFIFO(fifo_stat.st_mode)) {
        printf("Named Pipe (FIFO)\n");
    } else {
        printf("NON e' una FIFO!\n");
    }
    
    printf("Permessi: %o\n", fifo_stat.st_mode & 0777);
    printf("Dimensione: %ld bytes\n", (long)fifo_stat.st_size);
    
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    if (strcmp(argv[1], "create") == 0) {
        return create_fifo();
    }
    else if (strcmp(argv[1], "remove") == 0) {
        return remove_fifo();
    }
    else if (strcmp(argv[1], "status") == 0) {
        return check_fifo_status();
    }
    else {
        printf("Comando non riconosciuto: %s\n", argv[1]);
        print_usage(argv[0]);
        return 1;
    }
}

/* ========================================= */
/*              ISTRUZIONI D'USO             */
/* ========================================= */

/*
COMPILAZIONE:
gcc -o fifo_writer fifo_writer.c
gcc -o fifo_reader fifo_reader.c  
gcc -o fifo_manager fifo_manager.c

USO:
1. Gestione FIFO:
   ./fifo_manager create    # Crea la FIFO
   ./fifo_manager status    # Verifica stato
   ./fifo_manager remove    # Rimuove la FIFO

2. Comunicazione:
   Terminale 1: ./fifo_reader    # Avvia il lettore (aspetta)
   Terminale 2: ./fifo_writer    # Avvia lo scrittore (invia dati)

3. Sequenza tipica:
   ./fifo_manager create
   ./fifo_reader &           # In background
   ./fifo_writer            # In foreground
   ./fifo_manager remove    # Pulizia finale

CARATTERISTICHE:
- FIFO con nome fisso: /tmp/my_fifo
- Permessi 0666 (lettura/scrittura per tutti)
- Gestione automatica della creazione
- Messaggi di terminazione
- Gestione errori completa
- Compatibile C90
*/