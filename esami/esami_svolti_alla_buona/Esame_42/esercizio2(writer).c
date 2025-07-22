#include "apue.h"
#include <sys/shm.h>
#include <sys/ipc.h>

#define SHM_KEY 0x1234
#define MAX_ELEMENTS 10

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
static int create_shared_memory(void);
static void initialize_list(void *shm_ptr);
static void add_element_to_list(void *shm_ptr, int value);
static void print_creation_info(int num_elements);

/*
 * Crea e configura la memoria condivisa
 */
static int
create_shared_memory(void)
{
    int shmid;
    size_t shm_size;
    
    /* Calcola la dimensione necessaria */
    shm_size = sizeof(struct shm_header) + (MAX_ELEMENTS * sizeof(struct list_element));
    
    /* Crea la memoria condivisa */
    if ((shmid = shmget(SHM_KEY, shm_size, IPC_CREAT | IPC_EXCL | 0666)) < 0) {
        if (errno == EEXIST) {
            /* La memoria condivisa esiste già, la rimuove */
            if ((shmid = shmget(SHM_KEY, 0, 0)) >= 0) {
                shmctl(shmid, IPC_RMID, NULL);
            }
            /* Riprova a crearla */
            if ((shmid = shmget(SHM_KEY, shm_size, IPC_CREAT | IPC_EXCL | 0666)) < 0) {
                err_sys("shmget error on retry");
            }
        } else {
            err_sys("shmget error");
        }
    }
    
    return shmid;
}

/*
 * Inizializza la struttura della lista
 */
static void
initialize_list(void *shm_ptr)
{
    struct shm_header *header = (struct shm_header *)shm_ptr;
    
    header->num_elements = 0;
    header->first_element_offset = -1;
}

/*
 * Aggiunge un elemento alla lista
 */
static void
add_element_to_list(void *shm_ptr, int value)
{
    struct shm_header *header = (struct shm_header *)shm_ptr;
    struct list_element *elements = (struct list_element *)((char *)shm_ptr + sizeof(struct shm_header));
    struct list_element *new_element;
    int new_offset;
    
    if (header->num_elements >= MAX_ELEMENTS) {
        printf("Lista piena, impossibile aggiungere altri elementi\n");
        return;
    }
    
    /* Calcola l'offset del nuovo elemento */
    new_offset = header->num_elements;
    new_element = &elements[new_offset];
    
    /* Configura il nuovo elemento */
    new_element->value = value;
    new_element->next_offset = header->first_element_offset;
    
    /* Aggiorna l'header */
    header->first_element_offset = new_offset;
    header->num_elements++;
}

/*
 * Stampa informazioni sulla creazione della lista
 */
static void
print_creation_info(int num_elements)
{
    printf("\n=== SHARED MEMORY WRITER ===\n");
    printf("Lista creata con successo!\n");
    printf("Numero di elementi: %d\n", num_elements);
    printf("Chiave memoria condivisa: 0x%x\n", SHM_KEY);
    printf("La lista è pronta per essere letta dal reader.\n");
    printf("Eseguire: ./shm_reader\n\n");
}

int
main(void)
{
    int shmid;
    void *shm_ptr;
    int i;
    int values[] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
    int num_values = sizeof(values) / sizeof(values[0]);
    
    printf("Creazione della lista in memoria condivisa...\n");
    
    /* Crea la memoria condivisa */
    shmid = create_shared_memory();
    
    /* Collega la memoria condivisa al processo */
    if ((shm_ptr = shmat(shmid, 0, 0)) == (void *)-1) {
        err_sys("shmat error");
    }
    
    /* Inizializza la lista */
    initialize_list(shm_ptr);
    
    /* Aggiunge elementi alla lista */
    printf("Aggiunta elementi alla lista:\n");
    for (i = 0; i < num_values; i++) {
        add_element_to_list(shm_ptr, values[i]);
        printf("  Aggiunto: %d\n", values[i]);
    }
    
    /* Stampa informazioni */
    print_creation_info(num_values);
    
    /* Scollega la memoria condivisa */
    if (shmdt(shm_ptr) < 0) {
        err_sys("shmdt error");
    }
    
    exit(0);
}