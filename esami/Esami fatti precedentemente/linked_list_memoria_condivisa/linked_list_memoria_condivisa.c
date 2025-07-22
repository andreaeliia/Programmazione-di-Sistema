/*
 * ============================================================================
 * TRACCIA ESAME: Lista Concatenata in Memoria Condivisa
 * ============================================================================
 * 
 * RICHIESTA:
 * Si permetta a due processi di gestire, congiuntamente in un'area di memoria 
 * condivisa, la stessa lista concatenata (linked list) di elementi descritti 
 * da una struttura dati, (con le funzioni pop, insert, count). Per la 
 * condivisione della memoria e per la gestione della concorrenza, si usi la 
 * POSIX IPC. Si fornisca a entrambi i processi un'interfaccia utente testuale 
 * per ricevere dall'utente i comandi.
 * 
 * SOLUZIONE:
 * - Memoria condivisa POSIX (shm_open, mmap)
 * - Lista concatenata con OFFSET invece di puntatori
 * - Sincronizzazione con semafori POSIX
 * - Pool di nodi fisso per evitare malloc in memoria condivisa
 * - Interfaccia utente testuale per entrambi i processi
 * - Gestione multi-terminale per evitare output sovrapposto
 * 
 * COMPILAZIONE:
 * gcc -o traccia traccia_completa.c -lrt -lpthread
 * 
 * UTILIZZO:
 * 1. Terminale 1: ./traccia (scegli PROCESSO-A)
 * 2. Terminale 2: ./traccia (scegli PROCESSO-B)
 * 
 * AUTORE: [Il tuo nome]
 * DATA: [Data]
 * ============================================================================
 */

// === INCLUDE NECESSARI ===
#include <stdio.h>          // printf, scanf, perror
#include <stdlib.h>         // exit, system
#include <unistd.h>         // fork, getpid, close
#include <sys/wait.h>       // wait
#include <sys/mman.h>       // mmap, munmap, shm_open, shm_unlink
#include <sys/stat.h>       // costanti per shm_open
#include <fcntl.h>          // costanti O_* per shm_open
#include <string.h>         // strcmp, sprintf
#include <semaphore.h>      // sem_init, sem_wait, sem_post, sem_destroy

// === COSTANTI GLOBALI ===
#define MAX_NODES 100                      // Numero massimo di nodi nel pool
#define SHM_SIZE (sizeof(SharedMemory))    // Dimensione memoria condivisa
#define SHM_NAME "/linked_list_shm"        // Nome memoria condivisa POSIX

// ============================================================================
// STRUTTURE DATI
// ============================================================================

/**
 * Nodo della lista concatenata
 * 
 * PROBLEMA: I puntatori normali (Node*) non funzionano tra processi diversi
 * perché ogni processo mappa la memoria condivisa a indirizzi diversi.
 * 
 * SOLUZIONE: Usiamo OFFSET invece di puntatori. Un offset è la posizione
 * relativa del nodo nell'array, valida per tutti i processi.
 */
typedef struct {
    int data;           // Dato contenuto nel nodo
    int next_offset;    // ⭐ OFFSET del prossimo nodo (invece di puntatore!)
    int is_free;        // Flag: 1 = nodo libero, 0 = nodo occupato
} Node;

/**
 * Struttura principale della memoria condivisa
 * 
 * Contiene tutto ciò che deve essere condiviso tra i processi:
 * - Pool di nodi (array fisso)
 * - Metadati della lista (head, count)
 * - Semaforo per sincronizzazione
 */
typedef struct {
    Node nodes[MAX_NODES];  // Pool fisso di nodi (no malloc in memoria condivisa!)
    int head_offset;        // Offset del primo nodo (-1 se lista vuota)
    int count;              // Numero di elementi nella lista
    sem_t mutex;            // Semaforo per mutua esclusione
} SharedMemory;

// ============================================================================
// FUNZIONI HELPER PER GESTIONE OFFSET
// ============================================================================

/**
 * Converte un offset in un puntatore al nodo
 * 
 * @param shm: Puntatore alla memoria condivisa
 * @param offset: Offset del nodo nell'array (-1 = NULL)
 * @return: Puntatore al nodo o NULL se offset non valido
 */
Node* get_node_by_offset(SharedMemory* shm, int offset) {
    // Offset -1 rappresenta NULL
    if (offset == -1 || offset >= MAX_NODES) {
        return NULL;
    }
    
    // Restituisce puntatore al nodo all'offset specificato
    return &shm->nodes[offset];
}

/**
 * Converte un puntatore al nodo in un offset
 * 
 * @param shm: Puntatore alla memoria condivisa
 * @param node: Puntatore al nodo
 * @return: Offset del nodo nell'array (-1 se NULL)
 */
int get_offset_from_node(SharedMemory* shm, Node* node) {
    if (node == NULL) {
        return -1;  // NULL diventa offset -1
    }
    
    // Calcola offset sottraendo l'indirizzo base dell'array
    return node - shm->nodes;
}

/**
 * Alloca un nuovo nodo dal pool
 * 
 * Cerca il primo nodo libero nel pool e lo marca come occupato.
 * 
 * @param shm: Puntatore alla memoria condivisa
 * @return: Offset del nodo allocato (-1 se nessun nodo disponibile)
 */
int allocate_node(SharedMemory* shm) {
    // Scansiona tutto il pool cercando un nodo libero
    for (int i = 0; i < MAX_NODES; i++) {
        if (shm->nodes[i].is_free) {
            // Nodo trovato: marcalo come occupato
            shm->nodes[i].is_free = 0;
            shm->nodes[i].next_offset = -1;  // Inizializza next a NULL
            return i;  // Restituisce l'offset del nodo allocato
        }
    }
    
    // Nessun nodo disponibile
    return -1;
}

/**
 * Libera un nodo e lo rimette nel pool
 * 
 * @param shm: Puntatore alla memoria condivisa
 * @param offset: Offset del nodo da liberare
 */
void free_node(SharedMemory* shm, int offset) {
    // Controlla che l'offset sia valido
    if (offset >= 0 && offset < MAX_NODES) {
        // Marca il nodo come libero
        shm->nodes[offset].is_free = 1;
        shm->nodes[offset].next_offset = -1;  // Reset next
        shm->nodes[offset].data = 0;          // Reset data (opzionale)
    }
}

// ============================================================================
// OPERAZIONI SULLA LISTA (THREAD-SAFE)
// ============================================================================

/**
 * INSERT: Inserisce un elemento all'inizio della lista
 * 
 * Implementa l'inserimento in testa tipico delle liste concatenate.
 * La funzione è thread-safe grazie al semaforo.
 * 
 * @param shm: Puntatore alla memoria condivisa
 * @param data: Valore da inserire
 * @param process_name: Nome del processo (per log)
 */
void list_insert(SharedMemory* shm, int data, const char* process_name) {
    // === SEZIONE CRITICA INIZIO ===
    sem_wait(&shm->mutex);  // 🔒 Acquisisce il lock
    
    printf("%s: Inserisco %d nella lista\n", process_name, data);
    
    // Alloca un nuovo nodo dal pool
    int new_offset = allocate_node(shm);
    if (new_offset == -1) {
        printf("%s: ERRORE - Nessun nodo disponibile! (Pool esaurito)\n", process_name);
        sem_post(&shm->mutex);  // 🔓 Rilascia il lock prima di uscire
        return;
    }
    
    // Configura il nuovo nodo
    Node* new_node = &shm->nodes[new_offset];
    new_node->data = data;
    new_node->next_offset = shm->head_offset;  // Il nuovo nodo punta al vecchio primo
    
    // Il nuovo nodo diventa il primo della lista
    shm->head_offset = new_offset;
    shm->count++;  // Incrementa il contatore
    
    printf("%s: Inserito %d (offset: %d, count totale: %d)\n", 
           process_name, data, new_offset, shm->count);
    
    sem_post(&shm->mutex);  // 🔓 Rilascia il lock
    // === SEZIONE CRITICA FINE ===
}

/**
 * POP: Rimuove e restituisce il primo elemento della lista
 * 
 * Implementa la rimozione dalla testa tipica delle liste concatenate.
 * La funzione è thread-safe grazie al semaforo.
 * 
 * @param shm: Puntatore alla memoria condivisa
 * @param process_name: Nome del processo (per log)
 * @return: Valore rimosso (-1 se lista vuota)
 */
int list_pop(SharedMemory* shm, const char* process_name) {
    // === SEZIONE CRITICA INIZIO ===
    sem_wait(&shm->mutex);  // 🔒 Acquisisce il lock
    
    // Controlla se la lista è vuota
    if (shm->head_offset == -1) {
        printf("%s: Lista vuota - niente da rimuovere\n", process_name);
        sem_post(&shm->mutex);  // 🔓 Rilascia il lock
        return -1;
    }
    
    // Ottiene il primo nodo
    Node* head = get_node_by_offset(shm, shm->head_offset);
    int data = head->data;              // Salva il dato da restituire
    int old_head_offset = shm->head_offset;  // Salva l'offset del nodo da rimuovere
    
    // Il secondo nodo diventa il primo
    shm->head_offset = head->next_offset;
    shm->count--;  // Decrementa il contatore
    
    // Libera il nodo rimosso
    free_node(shm, old_head_offset);
    
    printf("%s: Rimosso %d (era all'offset: %d, count totale: %d)\n", 
           process_name, data, old_head_offset, shm->count);
    
    sem_post(&shm->mutex);  // 🔓 Rilascia il lock
    // === SEZIONE CRITICA FINE ===
    
    return data;  // Restituisce il valore rimosso
}

/**
 * COUNT: Conta e stampa il numero di elementi nella lista
 * 
 * @param shm: Puntatore alla memoria condivisa
 * @param process_name: Nome del processo (per log)
 * @return: Numero di elementi nella lista
 */
int list_count(SharedMemory* shm, const char* process_name) {
    // === SEZIONE CRITICA INIZIO ===
    sem_wait(&shm->mutex);  // 🔒 Acquisisce il lock
    
    int count = shm->count;  // Legge il contatore
    printf("%s: La lista contiene %d elementi\n", process_name, count);
    
    sem_post(&shm->mutex);  // 🔓 Rilascia il lock
    // === SEZIONE CRITICA FINE ===
    
    return count;
}

/**
 * PRINT: Stampa tutti gli elementi della lista
 * 
 * Attraversa la lista seguendo gli offset e stampa tutti i valori.
 * 
 * @param shm: Puntatore alla memoria condivisa
 * @param process_name: Nome del processo (per log)
 */
void list_print(SharedMemory* shm, const char* process_name) {
    // === SEZIONE CRITICA INIZIO ===
    sem_wait(&shm->mutex);  // 🔒 Acquisisce il lock
    
    printf("%s: Lista: [", process_name);
    
    // Attraversa la lista partendo dal primo nodo
    int current_offset = shm->head_offset;
    while (current_offset != -1) {
        Node* current = get_node_by_offset(shm, current_offset);
        printf("%d", current->data);
        
        // Passa al nodo successivo
        current_offset = current->next_offset;
        
        // Stampa freccia se ci sono altri elementi
        if (current_offset != -1) {
            printf(" -> ");
        }
    }
    
    printf("] (count: %d)\n", shm->count);
    
    sem_post(&shm->mutex);  // 🔓 Rilascia il lock
    // === SEZIONE CRITICA FINE ===
}

// ============================================================================
// INTERFACCIA UTENTE TESTUALE
// ============================================================================

/**
 * Mostra il menu delle opzioni disponibili
 * 
 * @param process_name: Nome del processo corrente
 */
void mostra_menu(const char* process_name) {
    printf("\n=== MENU %s ===\n", process_name);
    printf("1. Inserisci elemento\n");
    printf("2. Rimuovi elemento (pop)\n");
    printf("3. Conta elementi\n");
    printf("4. Stampa lista\n");
    printf("5. Inserisci multipli elementi\n");
    printf("6. Stato memoria (debug)\n");
    printf("0. Esci\n");
    printf("Scelta: ");
}

/**
 * Stampa informazioni di debug sulla memoria condivisa
 * 
 * @param shm: Puntatore alla memoria condivisa
 * @param process_name: Nome del processo (per log)
 */
void stampa_stato_memoria(SharedMemory* shm, const char* process_name) {
    sem_wait(&shm->mutex);
    
    printf("\n%s: === STATO MEMORIA CONDIVISA ===\n", process_name);
    printf("Head offset: %d", shm->head_offset);
    if (shm->head_offset == -1) {
        printf(" (lista vuota)\n");
    } else {
        printf(" (primo elemento: %d)\n", shm->nodes[shm->head_offset].data);
    }
    printf("Count: %d elementi\n", shm->count);
    
    // Conta nodi liberi e occupati
    int nodi_liberi = 0;
    int nodi_occupati = 0;
    for (int i = 0; i < MAX_NODES; i++) {
        if (shm->nodes[i].is_free) {
            nodi_liberi++;
        } else {
            nodi_occupati++;
        }
    }
    
    printf("Pool nodi: %d liberi + %d occupati = %d totali\n", 
           nodi_liberi, nodi_occupati, MAX_NODES);
    printf("Utilizzo memoria: %.1f%%\n", 
           (float)nodi_occupati / MAX_NODES * 100);
    
    sem_post(&shm->mutex);
}

/**
 * Esegue il comando scelto dall'utente
 * 
 * @param shm: Puntatore alla memoria condivisa
 * @param scelta: Comando scelto dall'utente
 * @param process_name: Nome del processo corrente
 */
void esegui_comando(SharedMemory* shm, int scelta, const char* process_name) {
    switch (scelta) {
        case 1: {  // Inserisci elemento
            int valore;
            printf("Inserisci valore: ");
            if (scanf("%d", &valore) == 1) {
                list_insert(shm, valore, process_name);
                printf("✅ Elemento %d inserito!\n", valore);
            } else {
                printf("❌ Valore non valido!\n");
                // Pulisce il buffer di input in caso di errore
                while (getchar() != '\n');
            }
            break;
        }
        
        case 2: {  // Rimuovi elemento (pop)
            int rimosso = list_pop(shm, process_name);
            if (rimosso != -1) {
                printf("✅ Elemento %d rimosso!\n", rimosso);
            } else {
                printf("❌ Lista vuota - niente da rimuovere!\n");
            }
            break;
        }
        
        case 3: {  // Conta elementi
            int count = list_count(shm, process_name);
            printf("📊 La lista contiene %d elementi\n", count);
            break;
        }
        
        case 4: {  // Stampa lista
            list_print(shm, process_name);
            break;
        }
        
        case 5: {  // Inserisci multipli elementi
            int num_elementi;
            printf("Quanti elementi vuoi inserire? ");
            if (scanf("%d", &num_elementi) == 1 && num_elementi > 0) {
                for (int i = 0; i < num_elementi; i++) {
                    int valore;
                    printf("Elemento %d: ", i + 1);
                    if (scanf("%d", &valore) == 1) {
                        list_insert(shm, valore, process_name);
                    } else {
                        printf("❌ Valore non valido, salto...\n");
                        while (getchar() != '\n');
                    }
                }
                printf("✅ Inseriti %d elementi!\n", num_elementi);
                list_print(shm, process_name);
            } else {
                printf("❌ Numero non valido!\n");
                while (getchar() != '\n');
            }
            break;
        }
        
        case 6: {  // Stato memoria (debug)
            stampa_stato_memoria(shm, process_name);
            break;
        }
        
        case 0:  // Esci
            printf("👋 %s in uscita...\n", process_name);
            break;
            
        default:
            printf("❌ Scelta non valida! Riprova.\n");
            break;
    }
}

/**
 * Loop principale dell'interfaccia utente
 * 
 * Gestisce il menu e l'input dell'utente in modo continuo.
 * 
 * @param shm: Puntatore alla memoria condivisa
 * @param process_name: Nome del processo corrente
 */
void interfaccia_utente(SharedMemory* shm, const char* process_name) {
    int scelta;
    
    printf("🚀 %s avviato! Interfaccia utente attiva.\n", process_name);
    printf("ℹ️  Puoi gestire la lista condivisa con l'altro processo.\n");
    
    // Loop principale del menu
    do {
        mostra_menu(process_name);
        
        // Legge la scelta dell'utente
        if (scanf("%d", &scelta) != 1) {
            printf("❌ Input non valido! Inserisci un numero.\n");
            // Pulisce il buffer di input in caso di errore
            while (getchar() != '\n');
            scelta = -1;  // Forza ripetizione del menu
            continue;
        }
        
        // Esegue il comando se non è "esci"
        if (scelta != 0) {
            esegui_comando(shm, scelta, process_name);
        }
        
    } while (scelta != 0);  // Continua fino a quando l'utente sceglie "0. Esci"
    
    printf("✅ %s terminato.\n", process_name);
}

// ============================================================================
// GESTIONE PROCESSI E SETUP
// ============================================================================

/**
 * Permette all'utente di scegliere quale processo essere
 * 
 * @return: 1 per PROCESSO-A, 2 per PROCESSO-B
 */
int scegli_processo() {
    int scelta;
    printf("=== GESTIONE LISTA CONCATENATA CONDIVISA ===\n");
    printf("Scegli quale processo vuoi essere:\n");
    printf("1. PROCESSO-A\n");
    printf("2. PROCESSO-B\n");
    printf("Scelta: ");
    
    if (scanf("%d", &scelta) == 1 && (scelta == 1 || scelta == 2)) {
        return scelta;
    } else {
        printf("❌ Scelta non valida, defaulting a PROCESSO-A\n");
        return 1;
    }
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    printf("=== TRACCIA COMPLETA: Lista Concatenata (Versione Multi-Terminale) ===\n");
    printf("Due processi gestiranno congiuntamente la stessa lista in memoria condivisa.\n\n");
    
    // === SETUP MEMORIA CONDIVISA ===
    
    // Tenta di aprire memoria condivisa esistente
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    SharedMemory* shm;
    int prima_istanza = 0;
    
    if (shm_fd == -1) {
        // Prima istanza - crea nuova memoria condivisa
        printf("🆕 Prima istanza: creo memoria condivisa...\n");
        prima_istanza = 1;
        
        // Crea nuovo segmento di memoria condivisa
        shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
        if (shm_fd == -1) {
            perror("Errore shm_open (creazione)");
            exit(1);
        }
        
        // Imposta la dimensione del segmento
        if (ftruncate(shm_fd, SHM_SIZE) == -1) {
            perror("Errore ftruncate");
            exit(1);
        }
        
        // Mappa il segmento nella memoria del processo
        shm = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, 
                   MAP_SHARED, shm_fd, 0);
        if (shm == MAP_FAILED) {
            perror("Errore mmap");
            exit(1);
        }
        
        // === INIZIALIZZAZIONE STRUTTURE DATI ===
        
        // Inizializza la lista come vuota
        shm->head_offset = -1;  // Lista vuota
        shm->count = 0;
        
        // Inizializza tutti i nodi del pool come liberi
        for (int i = 0; i < MAX_NODES; i++) {
            shm->nodes[i].is_free = 1;      // Nodo libero
            shm->nodes[i].next_offset = -1; // Next a NULL
            shm->nodes[i].data = 0;         // Data azzerato
        }
        
        // Inizializza il semaforo per la sincronizzazione
        // Parametri: semaforo, condiviso_tra_processi, valore_iniziale
        if (sem_init(&shm->mutex, 1, 1) == -1) {
            perror("Errore sem_init");
            exit(1);
        }
        
        printf("✅ Lista concatenata inizializzata in memoria condivisa!\n");
        printf("✅ Semaforo per sincronizzazione attivato!\n");
        printf("✅ Pool di %d nodi disponibili!\n\n", MAX_NODES);
        
    } else {
        // Seconda istanza - usa memoria condivisa esistente
        printf("🔄 Seconda istanza: connessione alla memoria condivisa esistente...\n");
        
        // Mappa il segmento esistente nella memoria del processo
        shm = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, 
                   MAP_SHARED, shm_fd, 0);
        if (shm == MAP_FAILED) {
            perror("Errore mmap");
            exit(1);
        }
        
        printf("✅ Connesso alla lista condivisa!\n");
        printf("✅ Lista attualmente contiene %d elementi!\n\n", shm->count);
    }
    
    // Chiude il file descriptor (non più necessario dopo mmap)
    close(shm_fd);
    
    // === SELEZIONE PROCESSO ===
    
    // L'utente sceglie quale processo essere
    int processo = scegli_processo();
    char nome_processo[20];
    sprintf(nome_processo, "PROCESSO-%c", 'A' + processo - 1);
    
    printf("\n🚀 Avvio %s\n", nome_processo);
    
    if (prima_istanza) {
        printf("💡 SUGGERIMENTO: Apri un altro terminale ed esegui di nuovo questo programma!\n");
        printf("💡 L'altro processo potrà gestire la stessa lista condivisa.\n");
        printf("💡 Comando: ./traccia\n");
    } else {
        printf("💡 Connesso! Ora stai gestendo la lista insieme all'altro processo.\n");
        printf("💡 Le modifiche saranno visibili in entrambi i terminali.\n");
    }
    printf("\n");
    
    // Mostra stato iniziale se non è la prima istanza
    if (!prima_istanza) {
        list_print(shm, nome_processo);
    }
    
    // === INTERFACCIA UTENTE ===
    
    // Avvia l'interfaccia utente testuale
    interfaccia_utente(shm, nome_processo);
    
    // === CLEANUP ===
    
    // Chiede all'utente se vuole rimuovere la memoria condivisa
    printf("\n❓ Vuoi rimuovere la memoria condivisa? (s/n): ");
    char risposta;
    scanf(" %c", &risposta);
    
    if (risposta == 's' || risposta == 'S') {
        // Distrugge il semaforo
        sem_destroy(&shm->mutex);
        
        // Rimuove il segmento di memoria condivisa
        if (shm_unlink(SHM_NAME) == 0) {
            printf("✅ Memoria condivisa rimossa.\n");
        } else {
            printf("ℹ️  Memoria condivisa già rimossa da altro processo.\n");
        }
    } else {
        printf("ℹ️  Memoria condivisa mantenuta per altre istanze.\n");
        printf("ℹ️  Per rimuoverla manualmente: ipcrm -M %s\n", SHM_NAME);
    }
    
    // Smappa la memoria dal processo
    munmap(shm, SHM_SIZE);
    
    printf("\n✅ %s terminato correttamente.\n", nome_processo);
    printf("🎓 Traccia completata con successo!\n");
    
    return 0;
}

/*
 * ============================================================================
 * NOTE TECNICHE IMPORTANTI
 * ============================================================================
 * 
 * 1. PERCHÉ OFFSET INVECE DI PUNTATORI?
 *    I puntatori sono indirizzi assoluti in memoria. Ogni processo mappa la
 *    memoria condivisa a indirizzi diversi, quindi un puntatore valido in un
 *    processo non è valido nell'altro. Gli offset sono posizioni relative
 *    nell'array, valide per tutti i processi.
 * 
 * 2. PERCHÉ POOL DI NODI FISSO?
 *    malloc() alloca memoria nell'heap privato del processo, non accessibile
 *    ad altri processi. Un pool fisso in memoria condivisa permette a tutti
 *    i processi di allocare/deallocare nodi dallo stesso spazio.
 * 
 * 3. PERCHÉ SEMAFORI?
 *    Senza sincronizzazione, due processi potrebbero modificare la lista
 *    contemporaneamente causando race conditions (es: entrambi leggono
 *    head_offset=5, modificano, uno sovrascrive l'altro). Il semaforo
 *    garantisce mutua esclusione.
 * 
 * 4. COMPILAZIONE CON -lrt -lpthread:
 *    -lrt: Link alla libreria POSIX Real-Time per shm_open/shm_unlink
 *    -lpthread: Link alla libreria POSIX Threads per semafori
 * 
 * 5. GESTIONE MULTI-TERMINALE:
 *    Invece di fork() che causa output sovrapposto, il programma permette
 *    di lanciare manualmente due istanze in terminali separati. La prima
 *    crea la memoria condivisa, la seconda si connette.
 * 
 * ============================================================================
 */