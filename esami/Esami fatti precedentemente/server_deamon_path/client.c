/*
 * ============================================================================
 * CLIENT DIRECTORY TRAVERSAL - VERSIONE SUPER COMMENTATA
 * ============================================================================
 * 
 * OBIETTIVO: Creare un client che si connette al daemon directory server
 * e richiede l'esplorazione ricorsiva di un path
 * 
 * FLUSSO GENERALE:
 * 1. Parsing argomenti comando
 * 2. Connessione al daemon server
 * 3. Invio path da esplorare
 * 4. Ricezione lista directory
 * 5. Stampa risultato su stdout (come richiesto dalla traccia)
 * 
 * MODALITÀ SUPPORTATE:
 * - Singola query: ./client /path/da/esplorare
 * - Modalità interattiva: ./client --interactive
 * - Help: ./client --help
 * 
 * ARGOMENTI TRATTATI:
 * - Socket TCP client-side
 * - Protocolli di comunicazione di rete
 * - Gestione input utente
 * - Validazione dati
 * - Error handling per connessioni di rete
 * - Conversione byte order (htonl/ntohl)
 * ============================================================================
 */

// === INCLUDE DEI HEADER NECESSARI ===

#include <stdio.h>          // printf, scanf, fgets, FILE operations
#include <stdlib.h>         // malloc, free, exit, EXIT_SUCCESS/FAILURE
#include <string.h>         // strlen, strcmp, strcpy, strstr, strerror, strcspn
#include <unistd.h>         // close, read, write
#include <sys/socket.h>     // socket, connect, send, recv
#include <netinet/in.h>     // struct sockaddr_in, htons, htonl, ntohl
#include <arpa/inet.h>      // inet_pton per convertire IP string in binario
#include <errno.h>          // errno, strerror per gestire errori

// === DEFINIZIONE COSTANTI ===

#define SERVER_IP "127.0.0.1"      // Indirizzo IP del server (localhost)
#define SERVER_PORT 8080            // Porta TCP del server (uguale al daemon)
#define BUFFER_SIZE 1048576            // Buffer per ricevere risposte (più grande del daemon)
#define MAX_PATH_LENGTH 512         // Lunghezza massima path (uguale al daemon)

// ============================================================================
// FUNZIONI NETWORKING - PROTOCOLLO DI COMUNICAZIONE
// ============================================================================

/*
 * FUNZIONE: send_message
 * SCOPO: Invia un messaggio al server usando protocollo [lunghezza][dati]
 * 
 * PROTOCOLLO (identico al daemon):
 * - Primi 4 bytes: lunghezza messaggio in network byte order
 * - Seguenti N bytes: messaggio effettivo
 * 
 * PERCHÉ QUESTO PROTOCOLLO:
 * - TCP è un stream protocol: i dati possono arrivare spezzati
 * - Senza lunghezza, non sai quando finisce un messaggio
 * - Con lunghezza, sai esattamente quanti bytes aspettare
 * 
 * PARAMETRI:
 * - socket_fd: File descriptor del socket connesso al server
 * - message: Stringa da inviare al server
 * 
 * RETURN: Numero di bytes inviati, -1 se errore
 */
int send_message(int socket_fd, const char *message) {
    uint32_t msg_length = strlen(message);          // Calcola lunghezza stringa
    uint32_t net_length = htonl(msg_length);        // Converti in network byte order
    
    // Debug: mostra cosa stiamo inviando
    printf("📤 Invio messaggio: '%s' (%u bytes)\n", message, msg_length);
    
    // === STEP 1: INVIA LUNGHEZZA MESSAGGIO (4 BYTES) ===
    // send(socket, buffer, lunghezza, flags)
    // - socket_fd: socket connesso
    // - &net_length: puntatore ai 4 bytes della lunghezza
    // - sizeof(net_length): sempre 4 bytes
    // - 0: nessun flag speciale
    int bytes_sent = send(socket_fd, &net_length, sizeof(net_length), 0);
    if (bytes_sent != sizeof(net_length)) {
        // send ha inviato meno di 4 bytes - errore
        printf("❌ Errore invio lunghezza: %s\n", strerror(errno));
        return -1;
    }
    
    // === STEP 2: INVIA MESSAGGIO EFFETTIVO ===
    bytes_sent = send(socket_fd, message, msg_length, 0);
    if (bytes_sent != (int)msg_length) {
        // send ha inviato meno bytes del previsto - errore
        printf("❌ Errore invio messaggio: %s\n", strerror(errno));
        return -1;
    }
    
    printf("✅ Messaggio inviato con successo\n");
    return bytes_sent;
}

/*
 * FUNZIONE: receive_message
 * SCOPO: Riceve un messaggio dal server usando protocollo [lunghezza][dati]
 * 
 * ALGORITMO:
 * 1. Ricevi primi 4 bytes (lunghezza)
 * 2. Converti da network byte order
 * 3. Valida lunghezza
 * 4. Ricevi messaggio della lunghezza specificata
 * 5. Termina stringa con \0
 * 
 * PARAMETRI:
 * - socket_fd: File descriptor del socket
 * - buffer: Buffer dove salvare il messaggio ricevuto
 * - max_len: Dimensione massima del buffer
 * 
 * RETURN: Numero di bytes del messaggio, -1 se errore
 */
int receive_message(int socket_fd, char *buffer, int max_len) {
    uint32_t msg_length;  // Variabile per contenere lunghezza messaggio
    
    printf("📥 Attendo risposta dal server...\n");
    
    // === STEP 1: RICEVI LUNGHEZZA MESSAGGIO (4 BYTES) ===
    // MSG_WAITALL: non ritornare finché non hai ricevuto tutti i bytes richiesti
    // Importante per il protocollo: devi ricevere esattamente 4 bytes
    int bytes_received = recv(socket_fd, &msg_length, sizeof(msg_length), MSG_WAITALL);
    if (bytes_received != sizeof(msg_length)) {
        if (bytes_received == 0) {
            // recv() ritorna 0 quando l'altro capo chiude la connessione
            printf("🔌 Server ha chiuso la connessione\n");
        } else {
            // recv() ritorna -1 in caso di errore, altro valore = bytes parziali
            printf("❌ Errore ricezione lunghezza: %s\n", strerror(errno));
        }
        return -1;
    }
    
    // === STEP 2: CONVERTI DA NETWORK BYTE ORDER ===
    // ntohl() converte da network byte order (big-endian) a host byte order
    // Network byte order è sempre big-endian, host byte order dipende dall'architettura
    msg_length = ntohl(msg_length);
    printf("📏 Lunghezza messaggio da ricevere: %u bytes\n", msg_length);
    
    // === STEP 3: VALIDAZIONE LUNGHEZZA ===
    if (msg_length == 0 || msg_length >= max_len) {
        printf("❌ Lunghezza messaggio non valida: %u (max buffer: %d)\n", 
               msg_length, max_len);
        return -1;
    }
    
    // === STEP 4: RICEVI MESSAGGIO EFFETTIVO ===
    bytes_received = recv(socket_fd, buffer, msg_length, MSG_WAITALL);
    if (bytes_received != (int)msg_length) {
        printf("❌ Errore ricezione messaggio: %s\n", strerror(errno));
        return -1;
    }
    
    // === STEP 5: TERMINA STRINGA ===
    // Il messaggio ricevuto potrebbe non avere \0 finale
    buffer[msg_length] = '\0';
    
    printf("✅ Messaggio ricevuto: %u bytes\n", msg_length);
    return bytes_received;
}

// ============================================================================
// FUNZIONI CONNESSIONE
// ============================================================================

/*
 * FUNZIONE: connect_to_server
 * SCOPO: Crea socket TCP e si connette al daemon server
 * 
 * FLUSSO:
 * 1. socket() - crea socket TCP
 * 2. Configura indirizzo server (IP + porta)
 * 3. connect() - stabilisce connessione
 * 
 * DETTAGLI TECNICI:
 * - AF_INET: famiglia IPv4
 * - SOCK_STREAM: TCP (stream affidabile, orientato alla connessione)
 * - inet_pton(): converte IP stringa in formato binario
 * - htons(): converte porta da host a network byte order
 * 
 * RETURN: File descriptor del socket connesso, -1 se errore
 */
int connect_to_server() {
    int socket_fd;                    // File descriptor del socket
    struct sockaddr_in server_addr;  // Struttura indirizzo server
    
    printf("🔗 Connessione al server %s:%d...\n", SERVER_IP, SERVER_PORT);
    
    // === STEP 1: CREA SOCKET TCP ===
    // socket(domain, type, protocol)
    // - AF_INET: famiglia IPv4
    // - SOCK_STREAM: TCP (reliable, connection-oriented)
    // - 0: protocollo di default per TCP
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        printf("❌ Errore creazione socket: %s\n", strerror(errno));
        return -1;
    }
    
    // === STEP 2: CONFIGURA INDIRIZZO SERVER ===
    memset(&server_addr, 0, sizeof(server_addr));     // Azzera struttura
    server_addr.sin_family = AF_INET;                 // Famiglia IPv4
    server_addr.sin_port = htons(SERVER_PORT);        // Porta in network byte order
    
    // Converte indirizzo IP da stringa a formato binario
    // inet_pton(family, src_string, dst_binary)
    // - AF_INET: IPv4
    // - SERVER_IP: "127.0.0.1"
    // - &server_addr.sin_addr: dove salvare il risultato binario
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        printf("❌ Indirizzo server non valido: %s\n", SERVER_IP);
        close(socket_fd);
        return -1;
    }
    
    // === STEP 3: CONNETTI AL SERVER ===
    // connect(socket, address, address_length)
    // Questa è una chiamata bloccante: si blocca finché non si connette o fallisce
    if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("❌ Connessione fallita: %s\n", strerror(errno));
        printf("💡 Verifica che il daemon server sia avviato\n");
        close(socket_fd);
        return -1;
    }
    
    printf("✅ Connesso al server!\n");
    return socket_fd;
}

// ============================================================================
// FUNZIONI VALIDAZIONE E UTILITÀ
// ============================================================================

/*
 * FUNZIONE: validate_path
 * SCOPO: Valida il path inserito dall'utente prima di inviarlo al server
 * 
 * CONTROLLI ESEGUITI:
 * 1. Path non nullo e non vuoto
 * 2. Lunghezza entro limiti
 * 3. Controllo sicurezza di base (no "..")
 * 
 * SICUREZZA:
 * - ".." permette path traversal attack
 * - Es: "../../etc/passwd" potrebbe uscire dalla directory consentita
 * - Meglio controllare lato client E lato server
 * 
 * PARAMETRI:
 * - path: Path da validare
 * 
 * RETURN: 1 se valido, 0 se non valido
 */
int validate_path(const char *path) {
    // === CONTROLLO 1: PATH NON VUOTO ===
    if (!path || strlen(path) == 0) {
        printf("❌ Path vuoto\n");
        return 0;
    }
    
    // === CONTROLLO 2: LUNGHEZZA MASSIMA ===
    if (strlen(path) >= MAX_PATH_LENGTH) {
        printf("❌ Path troppo lungo (max %d caratteri)\n", MAX_PATH_LENGTH - 1);
        return 0;
    }
    
    // === CONTROLLO 3: SICUREZZA PATH TRAVERSAL ===
    // strstr(haystack, needle) cerca needle in haystack
    // Ritorna puntatore se trovato, NULL se non trovato
    if (strstr(path, "..") != NULL) {
        printf("❌ Path non sicuro (contiene '..')\n");
        printf("💡 I path con '..' potrebbero uscire dalla directory consentita\n");
        return 0;
    }
    
    return 1;  // Path valido
}

/*
 * FUNZIONE: print_usage
 * SCOPO: Stampa le istruzioni di utilizzo del programma
 * 
 * INCLUDE:
 * - Sintassi comando
 * - Esempi di utilizzo
 * - Descrizione funzionalità
 * 
 * PARAMETRI:
 * - program_name: Nome del programma (argv[0])
 */
void print_usage(const char *program_name) {
    printf("📖 === DIRECTORY CLIENT - GUIDA UTILIZZO ===\n\n");
    
    printf("🚀 SINTASSI:\n");
    printf("  %s <path_da_esplorare>   # Modalità singola query\n", program_name);
    printf("  %s --interactive         # Modalità interattiva\n", program_name);
    printf("  %s --help               # Mostra questo aiuto\n\n", program_name);
    
    printf("📝 ESEMPI:\n");
    printf("  %s /home/user            # Esplora /home/user ricorsivamente\n", program_name);
    printf("  %s .                     # Esplora directory corrente\n", program_name);
    printf("  %s /tmp                  # Esplora /tmp\n", program_name);
    printf("  %s --interactive         # Modalità interattiva (multiple query)\n\n", program_name);
    
    printf("📋 DESCRIZIONE:\n");
    printf("  Il client si connette al daemon directory server e richiede\n");
    printf("  l'esplorazione ricorsiva del path specificato.\n");
    printf("  Il server restituisce tutti i path delle directory contenute\n");
    printf("  nel path richiesto, separate da carattere <a capo>.\n\n");
    
    printf("⚠️  PREREQUISITI:\n");
    printf("  - Daemon server deve essere avviato (./directory_daemon)\n");
    printf("  - Server deve essere raggiungibile su %s:%d\n", SERVER_IP, SERVER_PORT);
    printf("  - Permessi di lettura sulle directory da esplorare\n\n");
    
    printf("🔧 TROUBLESHOOTING:\n");
    printf("  - 'Connection refused': Daemon non avviato\n");
    printf("  - 'Permission denied': Avvia daemon con permessi adeguati\n");
    printf("  - 'Path not found': Verifica che il path esista\n");
}

// ============================================================================
// MODALITÀ OPERATIVE
// ============================================================================

/*
 * FUNZIONE: interactive_mode
 * SCOPO: Modalità interattiva - permette multiple query senza riconnettere
 * 
 * VANTAGGI MODALITÀ INTERATTIVA:
 * - Una sola connessione TCP per multiple query
 * - Più efficiente (no overhead connessione)
 * - Utile per esplorare più directory
 * 
 * FLUSSO:
 * 1. Connetti al server una volta
 * 2. Loop infinito per input utente
 * 3. Per ogni input: valida → invia → ricevi → stampa
 * 4. Termina con "quit" o "exit"
 */
void interactive_mode() {
    int socket_fd;                           // Socket connesso al server
    char path_input[MAX_PATH_LENGTH];        // Buffer per input utente
    char response_buffer[BUFFER_SIZE];       // Buffer per risposta server
    
    printf("🎮 === MODALITÀ INTERATTIVA ===\n");
    printf("💡 Digita 'quit' o 'exit' per uscire\n");
    printf("💡 Una connessione TCP per multiple query\n\n");
    
    // === CONNESSIONE UNICA AL SERVER ===
    socket_fd = connect_to_server();
    if (socket_fd < 0) {
        printf("❌ Impossibile connettersi al server\n");
        return;
    }
    
    // === LOOP INTERATTIVO ===
    while (1) {
        // === INPUT UTENTE ===
        printf("📁 Inserisci path da esplorare: ");
        fflush(stdout);  // Forza stampa del prompt
        
        // fgets legge una linea completa (incluso \n)
        if (fgets(path_input, sizeof(path_input), stdin) == NULL) {
            // fgets ritorna NULL su EOF (Ctrl+D) o errore
            printf("\n🔚 Input terminato (EOF)\n");
            break;
        }
        
        // === RIMUOVI NEWLINE FINALE ===
        // strcspn(str, charset) restituisce lunghezza prima del primo char in charset
        path_input[strcspn(path_input, "\n")] = 0;
        
        // === CONTROLLA COMANDI USCITA ===
        if (strcmp(path_input, "quit") == 0 || strcmp(path_input, "exit") == 0) {
            printf("👋 Arrivederci!\n");
            break;
        }
        
        // === CONTROLLA INPUT VUOTO ===
        if (strlen(path_input) == 0) {
            printf("⚠️  Path vuoto, riprova\n");
            continue;
        }
        
        // === VALIDA PATH ===
        if (!validate_path(path_input)) {
            printf("⚠️  Path non valido, riprova\n");
            continue;
        }
        
        // === INVIA RICHIESTA AL SERVER ===
        if (send_message(socket_fd, path_input) < 0) {
            printf("❌ Errore comunicazione con server\n");
            break;
        }
        
        // === RICEVI RISPOSTA ===
        if (receive_message(socket_fd, response_buffer, BUFFER_SIZE) < 0) {
            printf("❌ Errore ricezione risposta\n");
            break;
        }
        
        // === STAMPA RISULTATO ===
        printf("\n📂 === DIRECTORY RICORSIVE IN '%s' ===\n", path_input);
        if (strlen(response_buffer) == 0) {
            printf("(Nessuna directory trovata)\n");
        } else {
            // La risposta è già formattata con \n per ogni directory
            printf("%s", response_buffer);
        }
        printf("========================================\n\n");
    }
    
    // === CLEANUP ===
    close(socket_fd);
    printf("🔌 Connessione chiusa\n");
}

/*
 * FUNZIONE: single_query_mode
 * SCOPO: Modalità singola query - esegue una richiesta e termina
 * 
 * QUESTA È LA MODALITÀ PRINCIPALE RICHIESTA DALLA TRACCIA:
 * - Client invia path
 * - Server restituisce directory ricorsive
 * - Client stampa risultato su stdout
 * 
 * PARAMETRI:
 * - path: Path da esplorare (da argv[1])
 */
void single_query_mode(const char *path) {
    int socket_fd;                           // Socket per connessione server
    char response_buffer[BUFFER_SIZE];       // Buffer per risposta
    
    printf("🎯 === MODALITÀ SINGOLA QUERY ===\n");
    printf("📁 Path richiesto: %s\n\n", path);
    
    // === VALIDAZIONE PATH ===
    if (!validate_path(path)) {
        printf("❌ Path non valido, operazione annullata\n");
        return;
    }
    
    // === CONNESSIONE AL SERVER ===
    socket_fd = connect_to_server();
    if (socket_fd < 0) {
        return;  // Errore già stampato in connect_to_server()
    }
    
    // === INVIO RICHIESTA ===
    if (send_message(socket_fd, path) < 0) {
        close(socket_fd);
        return;
    }
    
    // === RICEZIONE RISPOSTA ===
    if (receive_message(socket_fd, response_buffer, BUFFER_SIZE) < 0) {
        close(socket_fd);
        return;
    }
    
    // === STAMPA RISULTATO (COME RICHIESTO DALLA TRACCIA) ===
    printf("📂 === DIRECTORY RICORSIVE ===\n");
    
    // Controlla se ci sono risultati
    if (strlen(response_buffer) == 0) {
        printf("(Nessuna directory trovata in '%s')\n", path);
    } else if (strncmp(response_buffer, "ERRORE:", 7) == 0) {
        // Il server ha restituito un messaggio di errore
        printf("❌ %s", response_buffer);
    } else {
        // Stampa risultato normale - già formattato dal server
        printf("%s", response_buffer);
    }
    
    // === CLEANUP ===
    close(socket_fd);
}

// ============================================================================
// MAIN - PUNTO DI INGRESSO DEL CLIENT
// ============================================================================

/*
 * FUNZIONE: main
 * SCOPO: Punto di ingresso principale del client
 * 
 * GESTISCE:
 * - Parsing argomenti comando
 * - Scelta modalità operativa
 * - Chiamata funzione appropriata
 * 
 * MODALITÀ SUPPORTATE:
 * 1. Singola query: ./client /path
 * 2. Interattiva: ./client --interactive
 * 3. Help: ./client --help
 * 
 * PARAMETRI:
 * - argc: Numero argomenti (include nome programma)
 * - argv: Array argomenti
 *   - argv[0]: nome programma
 *   - argv[1]: primo argomento (path o opzione)
 */
int main(int argc, char *argv[]) {
    // === INTESTAZIONE PROGRAMMA ===
    printf("🚀 === CLIENT DIRECTORY TRAVERSAL ===\n");
    printf("🔗 Server: %s:%d\n\n", SERVER_IP, SERVER_PORT);
    
    // === CONTROLLO NUMERO ARGOMENTI ===
    if (argc < 2) {
        // Nessun argomento fornito
        printf("❌ Numero argomenti insufficiente\n\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    // === PARSING ARGOMENTI E SCELTA MODALITÀ ===
    
    // MODALITÀ INTERATTIVA
    if (strcmp(argv[1], "--interactive") == 0 || strcmp(argv[1], "-i") == 0) {
        interactive_mode();
        return EXIT_SUCCESS;
    }
    
    // HELP
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }
    
    // VERSION (opzionale)
    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
        printf("Directory Client v1.0\n");
        printf("Compatibile con Directory Daemon v1.0\n");
        return EXIT_SUCCESS;
    }
    
    // MODALITÀ SINGOLA QUERY (DEFAULT)
    // Se argv[1] non è un'opzione, lo trattiamo come path da esplorare
    single_query_mode(argv[1]);
    
    return EXIT_SUCCESS;
}

/*
 * ============================================================================
 * ISTRUZIONI COMPILAZIONE ED ESECUZIONE
 * ============================================================================
 * 
 * COMPILAZIONE:
 * gcc -Wall -g -o directory_client client_super_commentato.c
 * 
 * PREREQUISITI:
 * 1. Compilare e avviare il daemon server:
 *    gcc -Wall -g -o directory_daemon daemon_super_commentato.c
 *    ./directory_daemon
 * 
 * ESECUZIONE CLIENT:
 * 
 * # Modalità singola (come richiesto dalla traccia)
 * ./directory_client /home/user
 * ./directory_client .
 * ./directory_client /tmp
 * 
 * # Modalità interattiva
 * ./directory_client --interactive
 * 
 * # Help
 * ./directory_client --help
 * 
 * OUTPUT ATTESO:
 * 🚀 === CLIENT DIRECTORY TRAVERSAL ===
 * 🔗 Server: 127.0.0.1:8080
 * 
 * 🎯 === MODALITÀ SINGOLA QUERY ===
 * 📁 Path richiesto: /home
 * 
 * 🔗 Connessione al server 127.0.0.1:8080...
 * ✅ Connesso al server!
 * 📤 Invio messaggio: '/home' (5 bytes)
 * ✅ Messaggio inviato con successo
 * 📥 Attendo risposta dal server...
 * 📏 Lunghezza messaggio da ricevere: 42 bytes
 * ✅ Messaggio ricevuto: 42 bytes
 * 
 * 📂 === DIRECTORY RICORSIVE ===
 * /home/user1
 * /home/user2
 * /home/shared
 * 
 * TROUBLESHOOTING:
 * 
 * ERRORE "Connection refused":
 * - Verifica che il daemon sia avviato: ps aux | grep directory_daemon
 * - Verifica porta libera: netstat -tulpn | grep 8080
 * 
 * ERRORE "Permission denied":
 * - Avvia daemon da directory con permessi lettura
 * - Testa con directory pubbliche (/tmp, /var/tmp)
 * 
 * ERRORE "Path not found":
 * - Verifica che il path esista: ls -la /path/richiesto
 * - Usa path assoluti per evitare ambiguità
 * 
 * ============================================================================
 */