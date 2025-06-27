# Memoria Condivisa - Guida Completa per Esame

## Indice
1. [Concetti Teorici](#concetti-teorici)
2. [Memory Mapping (mmap)](#memory-mapping-mmap)
3. [POSIX Shared Memory](#posix-shared-memory)
4. [System V Shared Memory](#system-v-shared-memory)
5. [Esempi Pratici](#esempi-pratici)
6. [Gestione Errori](#gestione-errori)
7. [Cleanup e Best Practices](#cleanup-e-best-practices)

---

## Concetti Teorici

### Che cos'è la Memoria Condivisa
La memoria condivisa è un meccanismo di comunicazione inter-process (IPC) che permette a più processi di accedere alla stessa regione di memoria fisica. È il metodo IPC più veloce perché evita la copia dei dati.

### Vantaggi
- **Performance elevata** - Accesso diretto alla memoria
- **Efficienza** - Nessuna copia di dati tra processi
- **Flessibilità** - Strutture dati complesse condivisibili

### Svantaggi  
- **Sincronizzazione richiesta** - Necessari semafori/mutex
- **Gestione complessa** - Cleanup e gestione errori
- **Debugging difficile** - Race conditions e memory corruption

### Tipi di Memoria Condivisa
1. **File-based (mmap)** - Mappatura di file in memoria
2. **Anonymous (mmap)** - Memoria senza file sottostante
3. **POSIX Shared Memory** - Oggetti shm_* 
4. **System V Shared Memory** - API shmget/shmat

---

## Memory Mapping (mmap)

### Funzioni Principali

```c
#include <sys/mman.h>

// Mappa un file o memoria in un processo
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);

// Rimuove il mapping
int munmap(void *addr, size_t length);

// Sincronizza memoria con file
int msync(void *addr, size_t length, int flags);
```

### Parametri mmap()

**addr**: Indirizzo preferito (di solito NULL)
**length**: Dimensione da mappare
**prot**: Protezioni memoria
- `PROT_READ` - Lettura
- `PROT_WRITE` - Scrittura  
- `PROT_EXEC` - Esecuzione
- `PROT_NONE` - Nessun accesso

**flags**: Tipo di mapping
- `MAP_SHARED` - Condiviso tra processi
- `MAP_PRIVATE` - Privato al processo
- `MAP_ANONYMOUS` - Senza file (con fd = -1)
- `MAP_FIXED` - Usa esattamente addr specificato

### Esempio Base File Mapping

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

int main() {
    const char* filename = "test_file.txt";
    int fd;
    char* mapped_memory;
    struct stat file_info;
    
    // Crea/apre file
    fd = open(filename, O_CREAT | O_RDWR, 0644);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }
    
    // Scrive dati nel file
    const char* data = "Hello, Memory Mapping!";
    write(fd, data, strlen(data));
    
    // Ottiene informazioni sul file
    if (fstat(fd, &file_info) == -1) {
        perror("fstat");
        close(fd);
        exit(EXIT_FAILURE);
    }
    
    // Mappa il file in memoria
    mapped_memory = mmap(NULL, file_info.st_size, 
                        PROT_READ | PROT_WRITE, 
                        MAP_SHARED, fd, 0);
    if (mapped_memory == MAP_FAILED) {
        perror("mmap");
        close(fd);
        exit(EXIT_FAILURE);
    }
    
    // Il file descriptor non serve più
    close(fd);
    
    // Legge da memoria mappata
    printf("Contenuto: %.*s\n", (int)file_info.st_size, mapped_memory);
    
    // Modifica in memoria (riflessa sul file)
    mapped_memory[0] = 'h';
    
    // Sincronizza con disco
    if (msync(mapped_memory, file_info.st_size, MS_SYNC) == -1) {
        perror("msync");
    }
    
    // Cleanup
    if (munmap(mapped_memory, file_info.st_size) == -1) {
        perror("munmap");
    }
    
    return 0;
}
```

### Esempio Memoria Condivisa tra Processi

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <string.h>

#define SHARED_SIZE 1024

typedef struct {
    int counter;
    char message[256];
} SharedData;

int main() {
    SharedData* shared_data;
    pid_t pid;
    
    // Crea memoria condivisa anonima
    shared_data = mmap(NULL, SHARED_SIZE,
                      PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shared_data == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    
    // Inizializza dati condivisi
    shared_data->counter = 0;
    strcpy(shared_data->message, "Messaggio iniziale");
    
    // Fork per creare processo figlio
    pid = fork();
    
    if (pid == -1) {
        perror("fork");
        munmap(shared_data, SHARED_SIZE);
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        // Processo figlio
        printf("Figlio: counter = %d\n", shared_data->counter);
        printf("Figlio: message = %s\n", shared_data->message);
        
        // Modifica dati condivisi
        shared_data->counter = 42;
        strcpy(shared_data->message, "Modificato dal figlio");
        
        printf("Figlio: modifiche completate\n");
    }
    else {
        // Processo padre
        sleep(1); // Aspetta che il figlio modifichi
        
        printf("Padre: counter = %d\n", shared_data->counter);
        printf("Padre: message = %s\n", shared_data->message);
        
        // Aspetta terminazione figlio
        wait(NULL);
    }
    
    // Cleanup
    if (munmap(shared_data, SHARED_SIZE) == -1) {
        perror("munmap");
    }
    
    return 0;
}
```

---

## POSIX Shared Memory

### Funzioni Principali

```c
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

// Crea/apre oggetto shared memory
int shm_open(const char *name, int oflag, mode_t mode);

// Rimuove oggetto shared memory
int shm_unlink(const char *name);

// Imposta dimensione
int ftruncate(int fd, off_t length);
```

### Esempio POSIX Shared Memory

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define SHM_NAME "/my_shared_memory"
#define SHM_SIZE 4096

typedef struct {
    int data[100];
    int count;
} SharedArray;

// Processo che crea la memoria condivisa
int creator_process() {
    int shm_fd;
    SharedArray* shared_array;
    
    // Crea oggetto shared memory
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0644);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    
    // Imposta dimensione
    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        perror("ftruncate");
        close(shm_fd);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }
    
    // Mappa in memoria
    shared_array = mmap(NULL, SHM_SIZE,
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED, shm_fd, 0);
    if (shared_array == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
    }
    
    // Inizializza dati
    shared_array->count = 5;
    for (int i = 0; i < shared_array->count; i++) {
        shared_array->data[i] = i * 10;
    }
    
    printf("Creator: dati inizializzati\n");
    
    // Mantiene il processo attivo
    sleep(10);
    
    // Cleanup
    munmap(shared_array, SHM_SIZE);
    close(shm_fd);
    shm_unlink(SHM_NAME);
    
    return 0;
}

// Processo che accede alla memoria condivisa
int accessor_process() {
    int shm_fd;
    SharedArray* shared_array;
    
    // Apre oggetto shared memory esistente
    shm_fd = shm_open(SHM_NAME, O_RDWR, 0);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    
    // Mappa in memoria
    shared_array = mmap(NULL, SHM_SIZE,
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED, shm_fd, 0);
    if (shared_array == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }
    
    // Legge dati condivisi
    printf("Accessor: count = %d\n", shared_array->count);
    printf("Accessor: data = ");
    for (int i = 0; i < shared_array->count; i++) {
        printf("%d ", shared_array->data[i]);
    }
    printf("\n");
    
    // Modifica dati
    shared_array->data[0] = 999;
    
    // Cleanup
    munmap(shared_array, SHM_SIZE);
    close(shm_fd);
    
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Uso: %s [creator|accessor]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    if (strcmp(argv[1], "creator") == 0) {
        return creator_process();
    }
    else if (strcmp(argv[1], "accessor") == 0) {
        return accessor_process();
    }
    else {
        printf("Argomento non valido. Usa 'creator' o 'accessor'\n");
        exit(EXIT_FAILURE);
    }
}
```

---

## System V Shared Memory

### Funzioni Principali

```c
#include <sys/ipc.h>
#include <sys/shm.h>

// Genera chiave univoca
key_t ftok(const char *pathname, int proj_id);

// Crea/ottiene segmento shared memory
int shmget(key_t key, size_t size, int shmflg);

// Attacca segmento al processo
void *shmat(int shmid, const void *shmaddr, int shmflg);

// Distacca segmento dal processo  
int shmdt(const void *shmaddr);

// Controllo segmento
int shmctl(int shmid, int cmd, struct shmid_ds *buf);
```

### Esempio System V Shared Memory

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <string.h>

#define PATHNAME "/tmp"
#define PROJ_ID 65
#define SHM_SIZE 1024

typedef struct {
    int numbers[10];
    char text[256];
    int flag;
} SharedMemory;

int main() {
    key_t key;
    int shmid;
    SharedMemory* shared_mem;
    pid_t pid;
    
    // Genera chiave univoca
    key = ftok(PATHNAME, PROJ_ID);
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    
    // Crea segmento shared memory
    shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0644);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    
    // Attacca il segmento
    shared_mem = (SharedMemory*)shmat(shmid, NULL, 0);
    if (shared_mem == (void*)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    
    // Inizializza memoria condivisa
    memset(shared_mem, 0, sizeof(SharedMemory));
    for (int i = 0; i < 10; i++) {
        shared_mem->numbers[i] = i + 1;
    }
    strcpy(shared_mem->text, "Messaggio dal padre");
    shared_mem->flag = 0;
    
    printf("Padre: memoria condivisa inizializzata\n");
    
    // Fork
    pid = fork();
    
    if (pid == -1) {
        perror("fork");
        shmdt(shared_mem);
        shmctl(shmid, IPC_RMID, NULL);
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        // Processo figlio
        printf("Figlio: accesso alla memoria condivisa\n");
        
        // Legge dati
        printf("Figlio: numbers = ");
        for (int i = 0; i < 10; i++) {
            printf("%d ", shared_mem->numbers[i]);
        }
        printf("\n");
        printf("Figlio: text = %s\n", shared_mem->text);
        
        // Modifica dati
        for (int i = 0; i < 10; i++) {
            shared_mem->numbers[i] *= 2;
        }
        strcpy(shared_mem->text, "Modificato dal figlio");
        shared_mem->flag = 1;
        
        printf("Figlio: modifiche completate\n");
        
        // Distacca segmento
        shmdt(shared_mem);
        exit(0);
    }
    else {
        // Processo padre
        wait(NULL); // Aspetta il figlio
        
        printf("Padre: verifica modifiche del figlio\n");
        printf("Padre: flag = %d\n", shared_mem->flag);
        printf("Padre: text = %s\n", shared_mem->text);
        printf("Padre: numbers = ");
        for (int i = 0; i < 10; i++) {
            printf("%d ", shared_mem->numbers[i]);
        }
        printf("\n");
        
        // Distacca e rimuove segmento
        shmdt(shared_mem);
        shmctl(shmid, IPC_RMID, NULL);
        
        printf("Padre: cleanup completato\n");
    }
    
    return 0;
}
```

---

## Esempi Pratici

### Esempio Completo: Buffer Circolare Condiviso

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>

#define BUFFER_SIZE 10
#define SHARED_SIZE sizeof(CircularBuffer)

typedef struct {
    int buffer[BUFFER_SIZE];
    int head;           // Indice di scrittura
    int tail;           // Indice di lettura  
    int count;          // Numero elementi
    int producer_done;  // Flag terminazione producer
} CircularBuffer;

// Funzione per produrre dati
void producer(CircularBuffer* cb) {
    printf("Producer: avviato\n");
    
    for (int i = 1; i <= 20; i++) {
        // Aspetta spazio disponibile
        while (cb->count == BUFFER_SIZE) {
            usleep(10000); // 10ms
        }
        
        // Inserisce elemento
        cb->buffer[cb->head] = i;
        cb->head = (cb->head + 1) % BUFFER_SIZE;
        cb->count++;
        
        printf("Producer: inserito %d (count=%d)\n", i, cb->count);
        usleep(50000); // 50ms
    }
    
    cb->producer_done = 1;
    printf("Producer: terminato\n");
}

// Funzione per consumare dati
void consumer(CircularBuffer* cb) {
    printf("Consumer: avviato\n");
    
    while (!cb->producer_done || cb->count > 0) {
        // Aspetta dati disponibili
        while (cb->count == 0 && !cb->producer_done) {
            usleep(10000); // 10ms
        }
        
        if (cb->count > 0) {
            // Legge elemento
            int item = cb->buffer[cb->tail];
            cb->tail = (cb->tail + 1) % BUFFER_SIZE;
            cb->count--;
            
            printf("Consumer: letto %d (count=%d)\n", item, cb->count);
            usleep(80000); // 80ms
        }
    }
    
    printf("Consumer: terminato\n");
}

int main() {
    CircularBuffer* shared_buffer;
    pid_t pid;
    
    // Crea memoria condivisa
    shared_buffer = mmap(NULL, SHARED_SIZE,
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shared_buffer == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    
    // Inizializza buffer
    memset(shared_buffer, 0, SHARED_SIZE);
    shared_buffer->head = 0;
    shared_buffer->tail = 0;
    shared_buffer->count = 0;
    shared_buffer->producer_done = 0;
    
    printf("Buffer circolare inizializzato\n");
    
    // Fork
    pid = fork();
    
    if (pid == -1) {
        perror("fork");
        munmap(shared_buffer, SHARED_SIZE);
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        // Processo figlio - Consumer
        consumer(shared_buffer);
    }
    else {
        // Processo padre - Producer
        producer(shared_buffer);
        
        // Aspetta terminazione consumer
        wait(NULL);
    }
    
    // Cleanup
    munmap(shared_buffer, SHARED_SIZE);
    return 0;
}
```

---

## Gestione Errori

### Controlli Essenziali

```c
// Template per gestione errori robusta
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int safe_mmap_example() {
    int fd = -1;
    void* mapped_mem = NULL;
    const char* filename = "data.txt";
    size_t file_size = 4096;
    
    // Apertura file con controllo errori
    fd = open(filename, O_CREAT | O_RDWR, 0644);
    if (fd == -1) {
        fprintf(stderr, "Errore open: %s\n", strerror(errno));
        return -1;
    }
    
    // Impostazione dimensione con controllo
    if (ftruncate(fd, file_size) == -1) {
        fprintf(stderr, "Errore ftruncate: %s\n", strerror(errno));
        close(fd);
        return -1;
    }
    
    // Memory mapping con controllo
    mapped_mem = mmap(NULL, file_size, 
                     PROT_READ | PROT_WRITE, 
                     MAP_SHARED, fd, 0);
    if (mapped_mem == MAP_FAILED) {
        fprintf(stderr, "Errore mmap: %s\n", strerror(errno));
        close(fd);
        return -1;
    }
    
    // File descriptor non più necessario
    close(fd);
    
    // Usa la memoria mappata...
    printf("Memory mapping riuscito\n");
    
    // Cleanup con controllo errori
    if (munmap(mapped_mem, file_size) == -1) {
        fprintf(stderr, "Errore munmap: %s\n", strerror(errno));
        return -1;
    }
    
    return 0;
}
```

---

## Cleanup e Best Practices

### Regole Fondamentali

1. **Sempre fare cleanup** - munmap(), close(), shm_unlink()
2. **Gestire i segnali** - Cleanup anche in caso di terminazione
3. **Verificare tutti i return values**
4. **Utilizzare atomic operations per contatori condivisi**
5. **Sincronizzazione obbligatoria** - Semafori o mutex

### Template di Cleanup

```c
#include <signal.h>

// Variabili globali per cleanup
static void* g_mapped_memory = NULL;
static size_t g_mapped_size = 0;
static int g_shm_fd = -1;
static const char* g_shm_name = NULL;

// Handler per segnali
void cleanup_handler(int sig) {
    printf("Ricevuto segnale %d, cleanup in corso...\n", sig);
    
    if (g_mapped_memory && g_mapped_memory != MAP_FAILED) {
        munmap(g_mapped_memory, g_mapped_size);
    }
    
    if (g_shm_fd != -1) {
        close(g_shm_fd);
    }
    
    if (g_shm_name) {
        shm_unlink(g_shm_name);
    }
    
    exit(sig);
}

// Registra handler per cleanup
void setup_cleanup() {
    signal(SIGINT, cleanup_handler);   // Ctrl+C
    signal(SIGTERM, cleanup_handler);  // Terminazione
    signal(SIGSEGV, cleanup_handler);  // Segmentation fault
}
```

### Checklist per l'Esame

**Memoria Condivisa File-based:**
- [ ] open() con flags corretti
- [ ] ftruncate() per dimensione  
- [ ] mmap() con MAP_SHARED
- [ ] close() del file descriptor
- [ ] munmap() per cleanup

**Memoria Condivisa Anonima:**
- [ ] mmap() con MAP_ANONYMOUS
- [ ] fd = -1
- [ ] Fork dopo mapping
- [ ] munmap() in tutti i processi

**POSIX Shared Memory:**
- [ ] shm_open() per creare/aprire
- [ ] ftruncate() per dimensione
- [ ] mmap() per mapping
- [ ] shm_unlink() per rimuovere

**System V Shared Memory:**
- [ ] ftok() per generare chiave
- [ ] shmget() per creare segmento
- [ ] shmat() per attaccare
- [ ] shmdt() per distaccare
- [ ] shmctl(IPC_RMID) per rimuovere