/*
 * ============================================================================
 * DAEMON DIRECTORY SERVER - VERSIONE SUPER COMMENTATA
 * ============================================================================
 * 
 * OBIETTIVO: Creare un daemon che riceve path dai client e restituisce
 * tutte le directory contenute ricorsivamente
 * 
 * FLUSSO GENERALE:
 * 1. Daemonizzazione (processo background)
 * 2. Creazione socket server TCP
 * 3. Loop infinito: accept client → fork → gestisci richiesta
 * 4. Ogni client invia path, riceve lista directory
 * 
 * ARGOMENTI TRATTATI:
 * - Fork e processi
 * - Socket TCP (bind, listen, accept)
 * - Daemonizzazione (setsid, chdir, etc.)
 * - Signal handling
 * - Directory traversal ricorsivo
 * - Protocolli di comunicazione di rete
 * - Gestione memoria dinamica
 * - Logging con syslog
 * ============================================================================
 */

// === INCLUDE DEI HEADER NECESSARI ===

#include <stdio.h>          // printf, sprintf, FILE operations
#include <stdlib.h>         // malloc, free, exit, EXIT_SUCCESS/FAILURE
#include <string.h>         // strlen, strcmp, strcpy, strstr, strerror
#include <unistd.h>         // fork, close, chdir, getpid, sleep
#include <sys/socket.h>     // socket, bind, listen, accept, send, recv
#include <netinet/in.h>     // struct sockaddr_in, htonl, htons, INADDR_ANY
#include <signal.h>         // signal, SIGTERM, SIGINT, SIGCHLD, SIGPIPE
#include <syslog.h>         // openlog, syslog, closelog, LOG_INFO, LOG_ERR
#include <errno.h>          // errno, strerror per gestire errori system call
#include <sys/wait.h>       // waitpid, WNOHANG per gestire processi figli zombie

// Header specifici per directory traversal
#include <dirent.h>         // opendir, readdir, closedir, struct dirent
#include <sys/stat.h>       // stat, struct stat, S_ISDIR per controllare se è directory

// === DEFINIZIONE COSTANTI ===

#define DAEMON_NAME "directory_daemon"  // Nome del daemon per logging
#define SERVER_PORT 8080               // Porta TCP su cui il server ascolta
#define BUFFER_SIZE 1048576             // Dimensione buffer per messaggi di rete
#define MAX_PATH_LENGTH 512           // Lunghezza massima path accettato

// === VARIABILE GLOBALE PER CONTROLLO DAEMON ===

/*
 * volatile sig_atomic_t: Tipo sicuro per variabili modificate nei signal handler
 * - volatile: Dice al compilatore di non ottimizzare questa variabile
 * - sig_atomic_t: Tipo che garantisce operazioni atomiche nei signal handler
 * - daemon_running: Flag globale che controlla se il daemon deve continuare
 */
volatile sig_atomic_t daemon_running = 1;

// ============================================================================
// STRUTTURE DATI PER GESTIONE LISTA DIRECTORY
// ============================================================================

/*
 * Struttura per gestire una lista dinamica di path delle directory
 * 
 * PROBLEMA: Non sappiamo a priori quante directory troveremo
 * SOLUZIONE: Array dinamico che si espande automaticamente
 */
typedef struct {
    char **paths;        // Array di puntatori a stringhe (i path delle directory)
    size_t count;        // Numero di elementi attualmente nella lista
    size_t capacity;     // Capacità massima attuale dell'array
} DirectoryList;

/*
 * FUNZIONE: create_directory_list
 * SCOPO: Crea e inizializza una nuova lista vuota per i path delle directory
 * 
 * DETTAGLI:
 * - Alloca memoria per la struttura DirectoryList
 * - Inizializza con capacità di 10 elementi (si espanderà se necessario)
 * - Alloca array di puntatori per contenere i path
 * 
 * RETURN: Puntatore alla lista creata, NULL se errore allocazione memoria
 */
DirectoryList* create_directory_list() {
    // Alloca memoria per la struttura principale
    DirectoryList *list = malloc(sizeof(DirectoryList));
    if (!list) {
        // malloc è fallita - memoria insufficiente
        return NULL;
    }
    
    // Inizializza con capacità di 10 elementi (numero arbitrario di partenza)
    list->capacity = 10;
    list->count = 0;  // Lista inizialmente vuota
    
    // Alloca array di puntatori a char (per contenere i path)
    // sizeof(char*) perché ogni elemento è un puntatore a stringa
    list->paths = malloc(sizeof(char*) * list->capacity);
    
    if (!list->paths) {
        // Allocazione dell'array fallita - libera la struttura e restituisci errore
        free(list);
        return NULL;
    }
    
    return list;
}

/*
 * FUNZIONE: add_path_to_list
 * SCOPO: Aggiunge un nuovo path alla lista, espandendo l'array se necessario
 * 
 * PARAMETRI:
 * - list: Puntatore alla lista dove aggiungere
 * - path: Stringa contenente il path da aggiungere
 * 
 * ALGORITMO:
 * 1. Controlla se c'è spazio nell'array
 * 2. Se non c'è spazio, raddoppia la capacità (realloc)
 * 3. Copia il path nella lista (strdup fa malloc + strcpy)
 * 4. Incrementa il contatore
 * 
 * RETURN: 0 se successo, -1 se errore
 */
int add_path_to_list(DirectoryList *list, const char *path) {
    // Validazione parametri - controlla che non siano NULL
    if (!list || !path) {
        return -1;
    }
    
    // Controlla se l'array è pieno e deve essere espanso
    if (list->count >= list->capacity) {
        // Raddoppia la capacità (strategia comune per array dinamici)
        size_t new_capacity = list->capacity * 2;
        
        // Rialloca l'array con la nuova dimensione
        // realloc mantiene i dati esistenti e aggiunge spazio
        char **new_paths = realloc(list->paths, sizeof(char*) * new_capacity);
        if (!new_paths) {
            // realloc fallita - l'array originale rimane intatto
            return -1;
        }
        
        // Aggiorna i puntatori e la capacità
        list->paths = new_paths;
        list->capacity = new_capacity;
    }
    
    // Copia il path nella lista usando strdup
    // strdup: alloca memoria e copia la stringa (equivale a malloc + strcpy)
    list->paths[list->count] = strdup(path);
    if (!list->paths[list->count]) {
        // strdup fallita - memoria insufficiente
        return -1;
    }
    
    // Incrementa il contatore degli elementi
    list->count++;
    return 0;  // Successo
}

/*
 * FUNZIONE: free_directory_list
 * SCOPO: Libera tutta la memoria utilizzata dalla lista
 * 
 * IMPORTANTE: Deve liberare sia le stringhe che l'array che la struttura
 * per evitare memory leak
 * 
 * ORDINE DI LIBERAZIONE:
 * 1. Ogni singola stringa (allocata con strdup)
 * 2. L'array di puntatori
 * 3. La struttura principale
 */
void free_directory_list(DirectoryList *list) {
    if (!list) {
        return;  // Lista già NULL, niente da fare
    }
    
    // Libera ogni singolo path (stringa) nella lista
    for (size_t i = 0; i < list->count; i++) {
        free(list->paths[i]);  // Libera la stringa allocata con strdup
    }
    
    // Libera l'array di puntatori
    free(list->paths);
    
    // Libera la struttura principale
    free(list);
}

/*
 * FUNZIONE: directory_list_to_string
 * SCOPO: Converte la lista di path in una singola stringa con separatori \n
 * 
 * ALGORITMO:
 * 1. Calcola la lunghezza totale necessaria
 * 2. Alloca memoria per la stringa risultato
 * 3. Concatena tutti i path separati da \n
 * 
 * FORMATO OUTPUT: "path1\npath2\npath3\n"
 * 
 * RETURN: Stringa allocata dinamicamente (da liberare con free()), NULL se errore
 */
char* directory_list_to_string(DirectoryList *list) {
    // Lista vuota o NULL - restituisce stringa vuota
    if (!list || list->count == 0) {
        return strdup("");  // strdup("") alloca memoria per stringa vuota
    }
    
    // Calcola la lunghezza totale necessaria per la stringa risultato
    size_t total_length = 0;
    for (size_t i = 0; i < list->count; i++) {
        total_length += strlen(list->paths[i]) + 1;  // +1 per il carattere \n
    }
    total_length++;  // +1 per il carattere terminatore \0
    
    // Alloca memoria per la stringa risultato
    char *result = malloc(total_length);
    if (!result) {
        return NULL;  // Memoria insufficiente
    }
    
    // Inizializza stringa vuota (importante per strcat)
    result[0] = '\0';
    
    // Concatena tutti i path separati da \n
    for (size_t i = 0; i < list->count; i++) {
        strcat(result, list->paths[i]);  // Aggiunge il path
        strcat(result, "\n");           // Aggiunge separatore
    }
    
    return result;
}

// ============================================================================
// FUNZIONI HELPER PER DIRECTORY TRAVERSAL
// ============================================================================

/*
 * FUNZIONE: is_directory
 * SCOPO: Controlla se un path è una directory
 * 
 * USA: stat() system call per ottenere informazioni sul file/directory
 * 
 * PARAMETRI:
 * - path: Path da controllare
 * 
 * RETURN: 1 se è directory, 0 altrimenti
 */
int is_directory(const char *path) {
    struct stat statbuf;  // Struttura per contenere informazioni file/directory
    
    // stat() riempie statbuf con informazioni su path
    if (stat(path, &statbuf) != 0) {
        // stat fallita - file/directory non esiste o non accessibile
        return 0;
    }
    
    // S_ISDIR è una macro che controlla se il tipo è directory
    // statbuf.st_mode contiene il tipo e i permessi del file
    return S_ISDIR(statbuf.st_mode);
}

/*
 * FUNZIONE: build_path
 * SCOPO: Costruisce un path completo concatenando directory + nome file
 * 
 * ESEMPI:
 * - build_path("/home", "user") → "/home/user"
 * - build_path("/home/", "user") → "/home/user" (evita doppie /)
 * - build_path(".", "subdir") → "./subdir"
 * 
 * GESTISCE: Il carattere separatore '/' automaticamente
 * 
 * RETURN: Path completo allocato dinamicamente (da liberare con free())
 */
char* build_path(const char *dir, const char *name) {
    size_t dir_len = strlen(dir);    // Lunghezza directory base
    size_t name_len = strlen(name);  // Lunghezza nome file/directory
    
    // Alloca memoria: lunghezza dir + lunghezza name + '/' + '\0'
    char *path = malloc(dir_len + name_len + 2);
    if (!path) {
        return NULL;  // Memoria insufficiente
    }
    
    // Copia la directory base
    strcpy(path, dir);
    
    // Aggiungi '/' se necessario (evita doppie /)
    if (dir_len > 0 && dir[dir_len - 1] != '/') {
        strcat(path, "/");
    }
    
    // Aggiungi il nome file/directory
    strcat(path, name);
    
    return path;
}

/*
 * FUNZIONE: explore_directory_recursive
 * SCOPO: Esplora ricorsivamente una directory e aggiunge tutti i path
 *        delle subdirectory alla lista
 * 
 * ALGORITMO RICORSIVO:
 * 1. Apre la directory
 * 2. Per ogni elemento nella directory:
 *    a. Se è "." o ".." → salta (evita loop infiniti)
 *    b. Costruisce path completo
 *    c. Se è una directory → aggiunge alla lista E chiama ricorsivamente
 *    d. Se è un file → ignora (la traccia chiede solo directory)
 * 3. Chiude la directory
 * 
 * PARAMETRI:
 * - dir_path: Path della directory da esplorare
 * - list: Lista dove aggiungere i path trovati
 * 
 * RETURN: 0 se successo, -1 se errore
 */
int explore_directory_recursive(const char *dir_path, DirectoryList *list) {
    DIR *dir;                    // Handle per la directory aperta
    struct dirent *entry;        // Struttura per ogni elemento nella directory
    
    // Apre la directory per la lettura
    dir = opendir(dir_path);
    if (dir == NULL) {
        // opendir fallita - directory non esiste, permessi insufficienti, etc.
        syslog(LOG_WARNING, "Impossibile aprire directory '%s': %s", 
               dir_path, strerror(errno));
        return -1;
    }
    
    // Legge tutti gli elementi della directory
    while ((entry = readdir(dir)) != NULL) {
        // Salta le directory speciali "." (corrente) e ".." (parent)
        // Importante per evitare loop infiniti nella ricorsione
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Costruisce il path completo dell'elemento corrente
        char *full_path = build_path(dir_path, entry->d_name);
        if (!full_path) {
            // build_path fallita - memoria insufficiente
            closedir(dir);
            return -1;
        }
        
        // Controlla se l'elemento è una directory
        if (is_directory(full_path)) {
            // È una directory - aggiungila alla lista
            if (add_path_to_list(list, full_path) != 0) {
                // add_path_to_list fallita
                free(full_path);
                closedir(dir);
                return -1;
            }
            
            // RICORSIONE: Esplora anche questa subdirectory
            if (explore_directory_recursive(full_path, list) != 0) {
                // Ricorsione fallita
                free(full_path);
                closedir(dir);
                return -1;
            }
        }
        // Se non è una directory (è un file), lo ignoriamo
        // La traccia chiede solo directory
        
        // Libera memoria del path (allocato da build_path)
        free(full_path);
    }
    
    // Chiude la directory
    closedir(dir);
    return 0;  // Successo
}

/*
 * FUNZIONE PRINCIPALE: get_directories_recursive
 * SCOPO: Funzione pubblica che restituisce tutte le directory contenute
 *        ricorsivamente nel path dato
 * 
 * QUESTA È LA FUNZIONE CHE USERAI NEL SERVER per elaborare le richieste client
 * 
 * FLUSSO:
 * 1. Valida il path di input
 * 2. Crea una lista vuota
 * 3. Esplora ricorsivamente
 * 4. Converte la lista in stringa
 * 5. Libera la lista
 * 6. Restituisce la stringa
 * 
 * PARAMETRI:
 * - base_path: Path da esplorare (ricevuto dal client)
 * 
 * RETURN: Stringa con directory separate da \n (da liberare con free())
 *         NULL se errore grave
 */
char* get_directories_recursive(const char *base_path) {
    // Validazione input
    if (!base_path) {
        syslog(LOG_ERR, "Path nullo ricevuto");
        return NULL;
    }
    
    // Controlla che il path sia effettivamente una directory
    if (!is_directory(base_path)) {
        syslog(LOG_WARNING, "'%s' non è una directory", base_path);
        // Restituisce messaggio di errore invece di NULL
        // Così il client riceve una risposta invece di un errore di connessione
        return strdup("ERRORE: Path specificato non è una directory\n");
    }
    
    // Crea lista vuota per accumulare i risultati
    DirectoryList *list = create_directory_list();
    if (!list) {
        syslog(LOG_ERR, "Impossibile creare lista directory");
        return NULL;
    }
    
    // Esplora ricorsivamente la directory
    if (explore_directory_recursive(base_path, list) != 0) {
        // Esplorazione fallita - libera la lista e restituisci errore
        free_directory_list(list);
        return strdup("ERRORE: Impossibile esplorare directory\n");
    }
    
    // Converte la lista in stringa formattata
    char *result = directory_list_to_string(list);
    
    // Libera la lista (non più necessaria)
    free_directory_list(list);
    
    return result;
}

// ============================================================================
// FUNZIONI SIGNAL HANDLER
// ============================================================================

/*
 * FUNZIONE: signal_handler
 * SCOPO: Gestisce i segnali per terminazione pulita del daemon
 * 
 * SEGNALI GESTITI:
 * - SIGTERM: Terminazione richiesta dal sistema (kill)
 * - SIGINT: Interruzione da tastiera (Ctrl+C)
 * 
 * AZIONE: Imposta daemon_running = 0 per far uscire il main loop
 * 
 * NOTA: I signal handler devono essere il più semplici possibile
 *       Non chiamare funzioni non async-signal-safe
 */
void signal_handler(int sig) {
    if (sig == SIGTERM || sig == SIGINT) {
        // Log del segnale ricevuto
        syslog(LOG_INFO, "Ricevuto segnale terminazione %d", sig);
        
        // Imposta flag per terminazione del main loop
        daemon_running = 0;
    }
}

/*
 * FUNZIONE: sigchld_handler
 * SCOPO: Gestisce il segnale SIGCHLD per cleanup processi figli zombie
 * 
 * PROBLEMA: Quando un processo figlio termina, diventa zombie fino a quando
 *           il padre non fa wait(). Senza questo handler, si accumulano zombie.
 * 
 * SOLUZIONE: waitpid() con WNOHANG raccoglie tutti i figli terminati
 *            senza bloccare il processo padre
 */
void sigchld_handler(int sig) {
    // Raccoglie tutti i processi figli terminati
    // waitpid(-1, NULL, WNOHANG):
    // - -1: qualsiasi processo figlio
    // - NULL: non interessa il codice di uscita
    // - WNOHANG: non bloccare se non ci sono figli terminati
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        // Loop fino a quando non ci sono più figli da raccogliere
    }
}

// ============================================================================
// FUNZIONI DAEMONIZZAZIONE
// ============================================================================

/*
 * FUNZIONE: daemonize
 * SCOPO: Trasforma il processo corrente in un daemon (processo background)
 * 
 * PROCEDURA STANDARD PER DAEMONIZZAZIONE:
 * 1. Fork e termina il padre
 * 2. Figlio diventa session leader (setsid)
 * 3. Secondo fork per evitare riacquisizione terminale
 * 4. Cambia directory di lavoro
 * 5. Imposta umask
 * 6. Chiude file descriptor standard
 * 
 * RETURN: 0 se successo, -1 se errore
 */
int daemonize() {
    pid_t pid, sid;
    
    // === PRIMO FORK ===
    // Crea processo figlio e termina il padre
    pid = fork();
    if (pid < 0) {
        // Fork fallita
        syslog(LOG_ERR, "Primo fork fallito: %s", strerror(errno));
        return -1;
    }
    if (pid > 0) {
        // Siamo nel processo padre - terminare
        exit(EXIT_SUCCESS);
    }
    
    // Da qui in poi eseguiamo nel processo figlio
    
    // === DIVENTA SESSION LEADER ===
    // setsid() crea una nuova sessione con il processo corrente come leader
    // Questo distacca il processo dal terminale di controllo
    sid = setsid();
    if (sid < 0) {
        syslog(LOG_ERR, "setsid fallito: %s", strerror(errno));
        return -1;
    }
    
    // === SECONDO FORK ===
    // Evita che il daemon possa riacquisire un terminale di controllo
    pid = fork();
    if (pid < 0) {
        syslog(LOG_ERR, "Secondo fork fallito: %s", strerror(errno));
        return -1;
    }
    if (pid > 0) {
        // Termina il primo figlio
        exit(EXIT_SUCCESS);
    }
    
    // Ora siamo nel secondo figlio (il vero daemon)
    
    // === CAMBIA DIRECTORY DI LAVORO ===
    // Cambia a directory root per evitare di mantenere mount point occupati
    if (chdir("/") < 0) {
        syslog(LOG_ERR, "chdir fallito: %s", strerror(errno));
        return -1;
    }
    
    // === IMPOSTA UMASK ===
    // umask(0) dà controllo completo sui permessi dei file creati
    umask(0);
    
    // === CHIUDE FILE DESCRIPTOR STANDARD ===
    // Un daemon non dovrebbe usare stdin/stdout/stderr
    close(STDIN_FILENO);   // File descriptor 0
    close(STDOUT_FILENO);  // File descriptor 1
    close(STDERR_FILENO);  // File descriptor 2
    
    syslog(LOG_INFO, "Daemon creato con successo, PID: %d", getpid());
    return 0;
}

// ============================================================================
// FUNZIONI NETWORKING
// ============================================================================

/*
 * FUNZIONE: create_server_socket
 * SCOPO: Crea e configura il socket server TCP
 * 
 * FLUSSO:
 * 1. socket() - crea socket TCP
 * 2. setsockopt() - configura opzioni (riutilizzo indirizzo)
 * 3. bind() - associa socket a indirizzo e porta
 * 4. listen() - mette socket in ascolto
 * 
 * RETURN: File descriptor del socket, -1 se errore
 */
int create_server_socket() {
    int sockfd;                  // File descriptor del socket
    struct sockaddr_in addr;     // Struttura indirizzo IPv4
    int opt = 1;                 // Opzione per setsockopt
    
    // === STEP 1: CREA SOCKET TCP ===
    // socket(AF_INET, SOCK_STREAM, 0):
    // - AF_INET: famiglia IPv4
    // - SOCK_STREAM: TCP (stream affidabile)
    // - 0: protocollo di default per TCP
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        syslog(LOG_ERR, "Errore creazione socket: %s", strerror(errno));
        return -1;
    }
    
    // === STEP 2: CONFIGURA OPZIONI SOCKET ===
    // SO_REUSEADDR permette di riutilizzare immediatamente l'indirizzo
    // Utile quando si riavvia il daemon (evita "Address already in use")
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        syslog(LOG_ERR, "Errore setsockopt: %s", strerror(errno));
        close(sockfd);
        return -1;
    }
    
    // === STEP 3: CONFIGURA INDIRIZZO SERVER ===
    memset(&addr, 0, sizeof(addr));           // Azzera la struttura
    addr.sin_family = AF_INET;                // Famiglia IPv4
    addr.sin_addr.s_addr = INADDR_ANY;        // Accetta connessioni da qualsiasi IP
    addr.sin_port = htons(SERVER_PORT);       // Porta in network byte order
    
    // htons() converte da host byte order a network byte order
    // Necessario perché i dati di rete usano big-endian
    
    // === STEP 4: BIND SOCKET ALL'INDIRIZZO ===
    // Associa il socket all'indirizzo e porta specificati
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        syslog(LOG_ERR, "Errore bind porta %d: %s", SERVER_PORT, strerror(errno));
        close(sockfd);
        return -1;
    }
    
    // === STEP 5: METTI SOCKET IN ASCOLTO ===
    // listen(sockfd, backlog):
    // - backlog = 5: massimo 5 connessioni in coda
    if (listen(sockfd, 5) < 0) {
        syslog(LOG_ERR, "Errore listen: %s", strerror(errno));
        close(sockfd);
        return -1;
    }
    
    syslog(LOG_INFO, "Socket server creato su porta %d", SERVER_PORT);
    return sockfd;
}

/*
 * FUNZIONE: receive_message
 * SCOPO: Riceve un messaggio dal client usando protocollo [lunghezza][dati]
 * 
 * PROTOCOLLO:
 * - Primi 4 bytes: lunghezza messaggio (uint32_t in network byte order)
 * - Seguenti N bytes: messaggio effettivo
 * 
 * VANTAGGI:
 * - TCP può spezzare i messaggi in più packet
 * - Questo protocollo garantisce di ricevere il messaggio completo
 * - Evita problemi di buffering parziale
 * 
 * PARAMETRI:
 * - client_fd: File descriptor del client
 * - buffer: Buffer dove salvare il messaggio
 * - max_len: Dimensione massima buffer
 * 
 * RETURN: Numero di bytes del messaggio, -1 se errore
 */
int receive_message(int client_fd, char *buffer, int max_len) {
    uint32_t msg_length;  // Lunghezza messaggio (4 bytes)
    
    // === STEP 1: RICEVI LUNGHEZZA MESSAGGIO ===
    // MSG_WAITALL: non ritornare fino a quando non hai ricevuto tutti i bytes
    int bytes_received = recv(client_fd, &msg_length, sizeof(msg_length), MSG_WAITALL);
    if (bytes_received != sizeof(msg_length)) {
        // recv non ha ricevuto esattamente 4 bytes
        if (bytes_received == 0) {
            // Client ha chiuso la connessione normalmente
            syslog(LOG_INFO, "Client [%d] ha chiuso la connessione", client_fd);
        } else {
            // Errore di rete o disconnessione anomala
            syslog(LOG_WARNING, "Errore ricezione lunghezza da client [%d]: %s", 
                   client_fd, strerror(errno));
        }
        return -1;
    }
    
    // === STEP 2: CONVERTI LUNGHEZZA DA NETWORK BYTE ORDER ===
    // ntohl() converte da network byte order (big-endian) a host byte order
    msg_length = ntohl(msg_length);
    
    // === STEP 3: VALIDA LUNGHEZZA ===
    if (msg_length == 0 || msg_length >= max_len) {
        syslog(LOG_WARNING, "Lunghezza messaggio non valida da client [%d]: %u", 
               client_fd, msg_length);
        return -1;
    }
    
    // === STEP 4: RICEVI MESSAGGIO EFFETTIVO ===
    bytes_received = recv(client_fd, buffer, msg_length, MSG_WAITALL);
    if (bytes_received != (int)msg_length) {
        syslog(LOG_WARNING, "Errore ricezione messaggio da client [%d]: %s", 
               client_fd, strerror(errno));
        return -1;
    }
    
    // === STEP 5: TERMINA STRINGA ===
    buffer[msg_length] = '\0';
    
    syslog(LOG_INFO, "Ricevuto da client [%d]: '%s' (%u bytes)", 
           client_fd, buffer, msg_length);
    return msg_length;
}

/*
 * FUNZIONE: send_message
 * SCOPO: Invia un messaggio al client usando protocollo [lunghezza][dati]
 * 
 * PROTOCOLLO SIMMETRICO A receive_message:
 * - Invia prima la lunghezza (4 bytes)
 * - Poi invia il messaggio
 * 
 * PARAMETRI:
 * - client_fd: File descriptor del client
 * - message: Messaggio da inviare
 * 
 * RETURN: Numero di bytes inviati, -1 se errore
 */
int send_message(int client_fd, const char *message) {
    uint32_t msg_length = strlen(message);              // Lunghezza messaggio
    uint32_t net_length = htonl(msg_length);            // Converti in network byte order
    
    // === STEP 1: INVIA LUNGHEZZA ===
    int bytes_sent = send(client_fd, &net_length, sizeof(net_length), 0);
    if (bytes_sent != sizeof(net_length)) {
        syslog(LOG_WARNING, "Errore invio lunghezza a client [%d]: %s", 
               client_fd, strerror(errno));
        return -1;
    }
    
    // === STEP 2: INVIA MESSAGGIO ===
    bytes_sent = send(client_fd, message, msg_length, 0);
    if (bytes_sent != (int)msg_length) {
        syslog(LOG_WARNING, "Errore invio messaggio a client [%d]: %s", 
               client_fd, strerror(errno));
        return -1;
    }
    
    syslog(LOG_INFO, "Inviato a client [%d]: %u bytes", client_fd, msg_length);
    return bytes_sent;
}

// ============================================================================
// GESTIONE CLIENT
// ============================================================================

/*
 * FUNZIONE: handle_client
 * SCOPO: Gestisce una singola connessione client
 * 
 * QUESTA FUNZIONE VIENE ESEGUITA NEL PROCESSO FIGLIO
 * 
 * FLUSSO:
 * 1. Loop per gestire multiple richieste dallo stesso client
 * 2. Per ogni richiesta:
 *    a. Ricevi path dal client
 *    b. Valida path (sicurezza)
 *    c. Esplora directory
 *    d. Invia risultato
 * 3. Quando client disconnette, chiudi socket e termina processo
 * 
 * PARAMETRI:
 * - client_fd: File descriptor del socket client
 */
void handle_client(int client_fd) {
    char path_buffer[MAX_PATH_LENGTH];  // Buffer per il path ricevuto
    
    syslog(LOG_INFO, "Gestione client [%d] avviata nel processo %d", client_fd, getpid());
    
    // === MAIN LOOP CLIENT ===
    // Loop infinito per gestire multiple richieste dallo stesso client
    while (1) {
        // === STEP 1: RICEVI PATH DAL CLIENT ===
        int received = receive_message(client_fd, path_buffer, MAX_PATH_LENGTH);
        if (received <= 0) {
            // Client disconnesso o errore di rete
            break;
        }
        
        syslog(LOG_INFO, "Client [%d] richiede esplorazione di: '%s'", 
               client_fd, path_buffer);
        
        // === STEP 2: VALIDAZIONE SICUREZZA ===
        // Controllo di base per path traversal attack
        // ".." permette di uscire dalla directory corrente
        if (strstr(path_buffer, "..") != NULL) {
            syslog(LOG_WARNING, "Client [%d] tentativo path traversal: '%s'", 
                   client_fd, path_buffer);
            send_message(client_fd, "ERRORE: Path non sicuro (contiene '..')\n");
            continue;  // Non disconnette, permette altre richieste
        }
        
        // === STEP 3: ESPLORA DIRECTORY ===
        // Chiama la funzione principale per ottenere lista directory
        char *result = get_directories_recursive(path_buffer);
        if (result) {
            // === STEP 4: INVIA RISULTATO AL CLIENT ===
            send_message(client_fd, result);
            free(result);  // Importante: libera memoria allocata
            
            syslog(LOG_INFO, "Inviato risultato esplorazione a client [%d]", client_fd);
        } else {
            // === STEP 5: GESTISCI ERRORE ===
            send_message(client_fd, "ERRORE: Impossibile esplorare directory\n");
            syslog(LOG_ERR, "Errore esplorazione directory per client [%d]", client_fd);
        }
    }
    
    // === CLEANUP E TERMINAZIONE ===
    syslog(LOG_INFO, "Connessione client [%d] terminata", client_fd);
    close(client_fd);
    // Nota: il processo figlio terminerà automaticamente al return da main
}

// ============================================================================
// MAIN - PUNTO DI INGRESSO DEL DAEMON
// ============================================================================

/*
 * FUNZIONE: main
 * SCOPO: Punto di ingresso principale del daemon
 * 
 * FLUSSO GENERALE:
 * 1. Inizializzazione logging
 * 2. Daemonizzazione (se richiesta)
 * 3. Setup signal handler
 * 4. Creazione socket server
 * 5. Main loop: accept → fork → handle client
 * 6. Cleanup e terminazione
 * 
 * PARAMETRI:
 * - argc: Numero argomenti comando
 * - argv: Array argomenti comando
 *         argv[1] = "--foreground" per esecuzione non daemon (debug)
 */
int main(int argc, char* argv[]) {
    int server_fd, client_fd;         // File descriptor socket server e client
    struct sockaddr_in client_addr;  // Indirizzo del client connesso
    socklen_t client_len;             // Lunghezza struttura indirizzo client
    pid_t child_pid;                  // PID del processo figlio per gestire client
    
    // === STEP 1: INIZIALIZZA LOGGING ===
    // openlog configura il logging di sistema (syslog)
    // LOG_PID: include PID nei messaggi
    // LOG_CONS: scrivi anche su console se syslog non disponibile
    // LOG_DAEMON: categoria daemon
    openlog(DAEMON_NAME, LOG_PID | LOG_CONS, LOG_DAEMON);
    syslog(LOG_INFO, "=== Avvio Daemon Directory Server ===");
    
    // === STEP 2: DAEMONIZZAZIONE (OPZIONALE) ===
    // Se non viene passato --foreground, daemonizza
    if (argc == 1 || strcmp(argv[1], "--foreground") != 0) {
        if (daemonize() < 0) {
            syslog(LOG_ERR, "Errore daemonizzazione");
            return EXIT_FAILURE;
        }
    } else {
        syslog(LOG_INFO, "Esecuzione in foreground per debug");
    }
    
    // === STEP 3: CONFIGURA GESTIONE SEGNALI ===
    signal(SIGTERM, signal_handler);    // Terminazione dal sistema (kill)
    signal(SIGINT, signal_handler);     // Interruzione tastiera (Ctrl+C)
    signal(SIGPIPE, SIG_IGN);          // Ignora broken pipe (client disconnesso)
    signal(SIGCHLD, sigchld_handler);   // Cleanup processi figli zombie
    
    // === STEP 4: CREA SOCKET SERVER ===
    server_fd = create_server_socket();
    if (server_fd < 0) {
        syslog(LOG_ERR, "Errore creazione socket server");
        return EXIT_FAILURE;
    }
    
    syslog(LOG_INFO, "Daemon avviato con successo - PID: %d, Porta: %d", 
           getpid(), SERVER_PORT);
    
    // === STEP 5: MAIN LOOP - ACCETTA CONNESSIONI CLIENT ===
    while (daemon_running) {  // Loop fino a segnale di terminazione
        client_len = sizeof(client_addr);
        
        // === ACCEPT NUOVA CONNESSIONE ===
        // accept() blocca fino a quando un client si connette
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (errno == EINTR) {
                // accept interrotta da segnale - riprova
                continue;
            }
            syslog(LOG_ERR, "Errore accept: %s", strerror(errno));
            break;
        }
        
        syslog(LOG_INFO, "Nuova connessione client [%d] accettata", client_fd);
        
        // === FORK PROCESSO FIGLIO PER GESTIRE CLIENT ===
        child_pid = fork();
        if (child_pid == 0) {
            // === CODICE PROCESSO FIGLIO ===
            close(server_fd);     // Il figlio non ha bisogno del socket server
            handle_client(client_fd);  // Gestisce questo client specifico
            exit(EXIT_SUCCESS);   // Termina processo figlio quando finito
            
        } else if (child_pid > 0) {
            // === CODICE PROCESSO PADRE ===
            close(client_fd);     // Il padre non ha bisogno del socket client
            // Il padre continua il loop per accettare altri client
            
        } else {
            // === ERRORE FORK ===
            syslog(LOG_ERR, "Errore fork per client [%d]: %s", 
                   client_fd, strerror(errno));
            close(client_fd);  // Chiudi connessione se non puoi gestirla
        }
    }
    
    // === STEP 6: CLEANUP E TERMINAZIONE ===
    syslog(LOG_INFO, "Terminazione daemon in corso...");
    close(server_fd);  // Chiudi socket server
    syslog(LOG_INFO, "=== Daemon Directory Server terminato ===");
    closelog();        // Chiudi logging
    
    return EXIT_SUCCESS;
}

/*
 * ============================================================================
 * ISTRUZIONI COMPILAZIONE ED ESECUZIONE
 * ============================================================================
 * 
 * COMPILAZIONE:
 * gcc -Wall -g -o directory_daemon daemon_super_commentato.c
 * 
 * ESECUZIONE:
 * # Modalità daemon (background)
 * ./directory_daemon
 * 
 * # Modalità foreground (per debug)
 * ./directory_daemon --foreground
 * 
 * TERMINAZIONE:
 * # Termina con segnale
 * killall directory_daemon
 * kill -TERM <PID>
 * 
 * # Se in foreground: Ctrl+C
 * 
 * CONTROLLO LOG:
 * tail -f /var/log/syslog | grep directory_daemon
 * journalctl -f | grep directory_daemon
 * 
 * ============================================================================
 */