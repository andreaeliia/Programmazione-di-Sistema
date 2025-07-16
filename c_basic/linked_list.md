# Liste Linkate in C90 - Teoria ed Esempi

## Introduzione alle Liste Linkate

Una **lista linkata** è una struttura dati dinamica composta da nodi, dove ogni nodo contiene:
- **Dati**: le informazioni che vogliamo memorizzare
- **Puntatore**: riferimento al nodo successivo

Le liste linkate permettono di gestire collezioni di dati di dimensione variabile durante l'esecuzione del programma.

## Vantaggi e Svantaggi

### Vantaggi
- **Dimensione dinamica**: cresce/decresce durante l'esecuzione
- **Inserimento/cancellazione efficiente**: O(1) se abbiamo il puntatore al nodo
- **Uso efficiente della memoria**: alloca solo quello che serve

### Svantaggi
- **Accesso sequenziale**: per arrivare al nodo N devo attraversare N-1 nodi
- **Overhead di memoria**: ogni nodo ha un puntatore aggiuntivo
- **Nessuna cache locality**: i nodi possono essere sparsi in memoria

## Definizione di Base

```c
#include "apue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Nodo base per una lista di interi */
typedef struct node {
    int data;
    struct node* next;
} Node;

/* Per il tuo daemon - nodo per file */
typedef struct file_node {
    off_t st_size;
    char path_name[1024];
    time_t last_modified;
    struct file_node* next;
} FileNode;
```

## Operazioni Fondamentali

### 1. Creazione di un Nodo

```c
/* Crea un nuovo nodo con un valore */
Node* create_node(int value) {
    Node* new_node = (Node*)malloc(sizeof(Node));
    if (new_node == NULL) {
        err_sys("malloc failed");
        return NULL;
    }
    new_node->data = value;
    new_node->next = NULL;
    return new_node;
}

/* Crea nodo per file */
FileNode* create_file_node(const char* path, off_t size, time_t mtime) {
    FileNode* new_node = (FileNode*)malloc(sizeof(FileNode));
    if (new_node == NULL) {
        err_sys("malloc failed for file node");
        return NULL;
    }
    strncpy(new_node->path_name, path, sizeof(new_node->path_name) - 1);
    new_node->path_name[sizeof(new_node->path_name) - 1] = '\0';
    new_node->st_size = size;
    new_node->last_modified = mtime;
    new_node->next = NULL;
    return new_node;
}
```

### 2. Inserimento in Testa

```c
/* Inserisce un nodo all'inizio della lista */
Node* insert_head(Node* head, int value) {
    Node* new_node = create_node(value);
    if (new_node == NULL) return head;
    
    new_node->next = head;
    return new_node;  /* Il nuovo nodo diventa la testa */
}

/* Inserimento file in testa */
FileNode* insert_file_head(FileNode* head, const char* path, off_t size, time_t mtime) {
    FileNode* new_node = create_file_node(path, size, mtime);
    if (new_node == NULL) return head;
    
    new_node->next = head;
    return new_node;
}
```

### 3. Inserimento in Coda

```c
/* Inserisce un nodo alla fine della lista */
Node* insert_tail(Node* head, int value) {
    Node* new_node = create_node(value);
    if (new_node == NULL) return head;
    
    /* Se la lista è vuota, il nuovo nodo diventa la testa */
    if (head == NULL) {
        return new_node;
    }
    
    /* Trova l'ultimo nodo */
    Node* current = head;
    while (current->next != NULL) {
        current = current->next;
    }
    
    /* Collega il nuovo nodo */
    current->next = new_node;
    return head;
}
```

### 4. Ricerca

```c
/* Cerca un valore nella lista */
Node* search(Node* head, int value) {
    Node* current = head;
    while (current != NULL) {
        if (current->data == value) {
            return current;
        }
        current = current->next;
    }
    return NULL;  /* Non trovato */
}

/* Cerca un file per path */
FileNode* search_file(FileNode* head, const char* path) {
    FileNode* current = head;
    while (current != NULL) {
        if (strcmp(current->path_name, path) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}
```

### 5. Cancellazione

```c
/* Cancella il primo nodo con un determinato valore */
Node* delete_value(Node* head, int value) {
    if (head == NULL) return NULL;
    
    /* Se il nodo da cancellare è la testa */
    if (head->data == value) {
        Node* temp = head;
        head = head->next;
        free(temp);
        return head;
    }
    
    /* Cerca il nodo da cancellare */
    Node* current = head;
    while (current->next != NULL && current->next->data != value) {
        current = current->next;
    }
    
    /* Se trovato, cancella */
    if (current->next != NULL) {
        Node* temp = current->next;
        current->next = current->next->next;
        free(temp);
    }
    
    return head;
}
```

### 6. Stampa della Lista

```c
/* Stampa tutti gli elementi della lista */
void print_list(Node* head) {
    Node* current = head;
    printf("Lista: ");
    while (current != NULL) {
        printf("%d -> ", current->data);
        current = current->next;
    }
    printf("NULL\n");
}

/* Stampa informazioni sui file */
void print_file_list(FileNode* head) {
    FileNode* current = head;
    printf("File monitorati:\n");
    while (current != NULL) {
        printf("File: %s, Size: %ld, Modified: %ld\n", 
               current->path_name, (long)current->st_size, (long)current->last_modified);
        current = current->next;
    }
}
```

### 7. Liberazione della Memoria

```c
/* Libera tutta la memoria della lista */
void free_list(Node* head) {
    Node* current = head;
    while (current != NULL) {
        Node* temp = current;
        current = current->next;
        free(temp);
    }
}

/* Libera lista di file */
void free_file_list(FileNode* head) {
    FileNode* current = head;
    while (current != NULL) {
        FileNode* temp = current;
        current = current->next;
        free(temp);
    }
}
```

## Esempio Completo per il Tuo Daemon

```c
#include "apue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

/* Struttura per memorizzare informazioni sui file */
typedef struct file_info {
    char path_name[1024];
    off_t st_size;
    time_t last_modified;
    struct file_info* next;
} FileInfo;

/* Variabile globale per la lista dei file */
static FileInfo* file_list = NULL;

/* Aggiunge un file alla lista */
void add_file_to_list(const char* path, off_t size, time_t mtime) {
    FileInfo* new_file = (FileInfo*)malloc(sizeof(FileInfo));
    if (new_file == NULL) {
        err_sys("malloc failed");
        return;
    }
    
    strncpy(new_file->path_name, path, sizeof(new_file->path_name) - 1);
    new_file->path_name[sizeof(new_file->path_name) - 1] = '\0';
    new_file->st_size = size;
    new_file->last_modified = mtime;
    new_file->next = file_list;
    
    file_list = new_file;
}

/* Controlla se un file è stato modificato */
int check_file_modified(const char* path) {
    struct stat st;
    FileInfo* current = file_list;
    
    /* Cerca il file nella lista */
    while (current != NULL) {
        if (strcmp(current->path_name, path) == 0) {
            /* File trovato, controlla se modificato */
            if (stat(path, &st) == 0) {
                if (st.st_mtime != current->last_modified || 
                    st.st_size != current->st_size) {
                    return 1;  /* File modificato */
                }
            }
            return 0;  /* File non modificato */
        }
        current = current->next;
    }
    return -1;  /* File non trovato nella lista */
}

/* Inizializza la lista con i file della directory */
void initialize_file_list(const char* directory) {
    /* Qui useresti la funzione myftw del tuo codice */
    /* per attraversare ricorsivamente la directory */
    
    /* Esempio semplificato: */
    struct stat st;
    if (stat(directory, &st) == 0) {
        add_file_to_list(directory, st.st_size, st.st_mtime);
    }
}

/* Controlla tutti i file per modifiche */
void check_all_files() {
    FileInfo* current = file_list;
    while (current != NULL) {
        int result = check_file_modified(current->path_name);
        if (result == 1) {
            printf("File modificato: %s\n", current->path_name);
            /* Aggiorna le informazioni del file */
            struct stat st;
            if (stat(current->path_name, &st) == 0) {
                current->st_size = st.st_size;
                current->last_modified = st.st_mtime;
            }
        }
        current = current->next;
    }
}

/* Cleanup: libera la memoria */
void cleanup_file_list() {
    FileInfo* current = file_list;
    while (current != NULL) {
        FileInfo* temp = current;
        current = current->next;
        free(temp);
    }
    file_list = NULL;
}
```

## Gestione degli Errori

```c
/* Versione sicura dell'inserimento */
int safe_insert_head(Node** head, int value) {
    Node* new_node = create_node(value);
    if (new_node == NULL) {
        return -1;  /* Errore */
    }
    
    new_node->next = *head;
    *head = new_node;
    return 0;  /* Successo */
}

/* Uso: */
Node* my_list = NULL;
if (safe_insert_head(&my_list, 42) != 0) {
    fprintf(stderr, "Errore nell'inserimento\n");
}
```

## Considerazioni per l'Esame

### Pattern Comuni
1. **Attraversamento**: usa sempre `while (current != NULL)`
2. **Modifica durante attraversamento**: fai attenzione ai puntatori
3. **Gestione memoria**: ogni `malloc` deve avere il suo `free`
4. **Lista vuota**: controlla sempre se `head == NULL`

### Errori Tipici da Evitare
- **Memory leak**: non liberare la memoria
- **Dangling pointer**: usare puntatori dopo `free`
- **Segmentation fault**: dereferenziare puntatori NULL
- **Lost pointer**: perdere il riferimento alla testa

### Trucchi per l'Esame
1. **Disegna sempre** la lista prima di scrivere il codice
2. **Testa i casi limite**: lista vuota, un solo elemento
3. **Usa nomi significativi** per le variabili
4. **Commenta il codice** per spiegare la logica

## Integrazione nel Tuo Daemon

Per il tuo daemon, puoi usare la lista per:

1. **Inizializzazione**: attraversa la directory e crea la lista
2. **Monitoraggio**: periodicamente controlla ogni file nella lista
3. **Aggiornamento**: modifica le informazioni quando rilevi cambiamenti
4. **Cleanup**: libera la memoria quando il daemon termina

La struttura che hai definito è perfetta per questo scopo!