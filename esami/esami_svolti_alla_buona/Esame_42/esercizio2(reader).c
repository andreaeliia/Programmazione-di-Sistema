#include "apue.h"
#include <sys/shm.h>
#include <sys/ipc.h>

#define SHM_KEY 0x1234

/* Struttura per un elemento della lista */
struct list_element {
    int value;
    int next_offset;  /* Offset del prossimo elemento, -1 se ultimo */
};

/* Struttura header della memoria condivisa */
struct shm_header {
    int num_elements;
    int first_element_offset;
};

/* Prototipi delle funzioni */
static int connect_to_shared_memory(void);
static void read_and_print_list(void *shm_ptr);
static void print_list_info(struct shm_header *header);
static void cleanup_shared_memory(int shmid);

/*
 * Si connette alla memoria condivisa esistente
 */
static int
connect_to_shared_memory(void)
{
    int shmid;
    
    /* Ottiene l'ID della memoria condivisa esistente */
    if ((shmid = shmget(SHM_KEY, 0, 0)) < 0) {
        if (errno == ENOENT) {
            printf("Errore: memoria condivisa non trovata.\n");
            printf("Eseguire prima il writer: ./shm_writer\n");
        }
        err_sys("shmget error");
    }
    
    return shmid;
}

/*
 * Legge e stampa tutti gli elementi della lista
 */
static void
read_and_print_list(void *shm_ptr)
{
    struct shm_header *header = (struct shm_header *)shm_ptr;
    struct list_element *elements = (struct list_element *)((char *)shm_ptr + sizeof(struct shm_header));
    int current_offset;
    int element_count = 0;
    
    printf("\n=== CONTENUTO DELLA LISTA ===\n");
    
    if (header->num_elements == 0) {
        printf("La lista è vuota.\n");
        return;
    }
    
    printf("Elementi nella lista (ordine di inserimento inverso):\n");
    current_offset = header->first_element_offset;
    
    while (current_offset != -1 && element_count < header->num_elements) {
        struct list_element *current = &elements[current_offset];
        
        printf("  Elemento %d: %d\n", element_count + 1, current->value);
        
        current_offset = current->next_offset;
        element_count++;
    }
    
    if (element_count != header->num_elements) {
        printf("Avvertimento: numero elementi letti (%d) diverso da quello atteso (%d)\n",
               element_count, header->num_elements);
    }
}

/*
 * Stampa informazioni generali sulla lista
 */
static void
print_list_info(struct shm_header *header)
{
    printf("\n=== INFORMAZIONI LISTA ===\n");
    printf("Numero totale di elementi: %d\n", header->num_elements);
    printf("Offset primo elemento: %d\n", header->first_element_offset);
    printf("Dimensione header: %lu bytes\n", (unsigned long)sizeof(struct shm_header));
    printf("Dimensione elemento: %lu bytes\n", (unsigned long)sizeof(struct list_element));
}

/*
 * Rimuove la memoria condivisa
 */
static void
cleanup_shared_memory(int shmid)
{
    char response;
    
    printf("\nVuoi rimuovere la memoria condivisa? (y/n): ");
    fflush(stdout);
    
    if (scanf(" %c", &response) == 1 && (response == 'y' || response == 'Y')) {
        if (shmctl(shmid, IPC_RMID, NULL) < 0) {
            err_ret("shmctl IPC_RMID error");
        } else {
            printf("Memoria condivisa rimossa con successo.\n");
        }
    } else {
        printf("Memoria condivisa mantenuta.\n");
    }
}

int
main(void)
{
    int shmid;
    void *shm_ptr;
    struct shm_header *header;
    
    printf("=== SHARED MEMORY READER ===\n");
    printf("Connessione alla memoria condivisa...\n");
    
    /* Si connette alla memoria condivisa */
    shmid = connect_to_shared_memory();
    
    /* Collega la memoria condivisa al processo */
    if ((shm_ptr = shmat(shmid, 0, SHM_RDONLY)) == (void *)-1) {
        err_sys("shmat error");
    }
    
    header = (struct shm_header *)shm_ptr;
    
    /* Stampa informazioni sulla lista */
    print_list_info(header);
    
    /* Legge e stampa il contenuto della lista */
    read_and_print_list(shm_ptr);
    
    /* Scollega la memoria condivisa */
    if (shmdt(shm_ptr) < 0) {
        err_sys("shmdt error");
    }
    
    /* Opzione per rimuovere la memoria condivisa */
    cleanup_shared_memory(shmid);
    
    exit(0);
}