/*
 * Server per sistema di monitoraggio feedback studenti
 * Uso: ./server <indirizzo_multicast> <porta>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>
#include <sys/select.h>

#define BUFFER_SIZE 256
#define MAX_MESSAGGI 10000

/* Struttura per il messaggio */
typedef struct {
    char tipo;
    time_t timestamp;
} messaggio_t;

/* Struttura per memorizzare i messaggi ricevuti */
typedef struct {
    char tipo;
    time_t timestamp;
} messaggio_storico_t;

/* Contatori per ogni categoria */
typedef struct {
    int totale;
    int ultimi_5_min;
    int ultimo_min;
} contatori_t;

/* Variabili globali */
contatori_t contatori[6]; /* A, R, I, N, C, B */
messaggio_storico_t messaggi[MAX_MESSAGGI];
int num_messaggi = 0;
time_t inizio_lezione;
int continua_esecuzione = 1;

/* Mappa caratteri agli indici */
int char_to_index(char c) {
    switch(c) {
        case 'A': return 0;
        case 'R': return 1;
        case 'I': return 2;
        case 'N': return 3;
        case 'C': return 4;
        case 'B': return 5;
        default: return -1;
    }
}

/* Nomi delle categorie */
const char* nomi_categorie[] = {
    "Applauso         ",
    "Ripetere         ",
    "Incomprensibile  ",
    "Troppo noioso    ",
    "Continuare così  ",
    "Interrompere     "
};

/* Aggiorna i contatori basandosi sui timestamp */
void aggiorna_contatori() {
    time_t ora_corrente = time(NULL);
    time_t limite_5_min = ora_corrente - 300;  /* 5 minuti = 300 secondi */
    time_t limite_1_min = ora_corrente - 60;   /* 1 minuto = 60 secondi */
    int i, idx;
    
    /* Reset contatori temporali */
    for (i = 0; i < 6; i++) {
        contatori[i].ultimi_5_min = 0;
        contatori[i].ultimo_min = 0;
    }
    
    /* Conta messaggi negli ultimi 5 minuti e nell'ultimo minuto */
    for (i = 0; i < num_messaggi; i++) {
        idx = char_to_index(messaggi[i].tipo);
        if (idx >= 0) {
            if (messaggi[i].timestamp >= limite_5_min) {
                contatori[idx].ultimi_5_min++;
            }
            if (messaggi[i].timestamp >= limite_1_min) {
                contatori[idx].ultimo_min++;
            }
        }
    }
}

/* Rimuove messaggi più vecchi di 5 minuti per liberare memoria */
void pulisci_messaggi_vecchi() {
    time_t ora_corrente = time(NULL);
    time_t limite = ora_corrente - 300;
    int i, j = 0;
    
    for (i = 0; i < num_messaggi; i++) {
        if (messaggi[i].timestamp >= limite) {
            messaggi[j] = messaggi[i];
            j++;
        }
    }
    num_messaggi = j;
}

/* Mostra le statistiche */
void mostra_statistiche() {
    time_t ora_corrente = time(NULL);
    int durata_lezione = (int)(ora_corrente - inizio_lezione);
    int i;
    
    /* Pulisce lo schermo */
    printf("\033[2J\033[H");
    
    printf("=== MONITORAGGIO FEEDBACK LEZIONE ===\n");
    printf("Durata lezione: %02d:%02d:%02d\n", 
           durata_lezione/3600, (durata_lezione%3600)/60, durata_lezione%60);
    printf("Aggiornato: %s", ctime(&ora_corrente));
    printf("\n");
    printf("%-17s | %-8s | %-12s | %-12s\n", 
           "CATEGORIA", "TOTALE", "ULTIMI 5 MIN", "ULTIMO MIN");
    printf("-------------------|----------|--------------|-------------\n");
    
    for (i = 0; i < 6; i++) {
        printf("%-17s | %-8d | %-12d | %-12d\n",
               nomi_categorie[i],
               contatori[i].totale,
               contatori[i].ultimi_5_min,
               contatori[i].ultimo_min);
    }
    
    printf("\nPremi Ctrl+C per terminare\n");
    fflush(stdout);
}

/* Gestione segnale di interruzione */
void signal_handler(int sig) {
    continua_esecuzione = 0;
    printf("\n\nRicevuto segnale di interruzione. Terminazione...\n");
}

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    struct ip_mreq mreq;
    messaggio_t msg;
    socklen_t client_len;
    fd_set readfds;
    struct timeval timeout;
    int result, idx;
    
    /* Controllo argomenti */
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <indirizzo_multicast> <porta>\n", argv[0]);
        exit(1);
    }
    
    /* Inizializzazione */
    memset(contatori, 0, sizeof(contatori));
    inizio_lezione = time(NULL);
    
    /* Gestione segnali */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* Creazione socket UDP */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Errore nella creazione del socket");
        exit(1);
    }
    
    /* Permette riuso dell'indirizzo */
    int reuse = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("Errore setsockopt SO_REUSEADDR");
        close(sockfd);
        exit(1);
    }
    
    /* Configurazione indirizzo server */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    /* Binding del socket */
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Errore nel binding del socket");
        close(sockfd);
        exit(1);
    }
    
    /* Join al gruppo multicast */
    mreq.imr_multiaddr.s_addr = inet_addr(argv[1]);
    mreq.imr_interface.s_addr = INADDR_ANY;
    
    if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("Errore nel join al gruppo multicast");
        close(sockfd);
        exit(1);
    }
    
    printf("Server avviato su %s:%s\n", argv[1], argv[2]);
    printf("In attesa di messaggi...\n\n");
    
    /* Mostra statistiche iniziali */
    mostra_statistiche();
    
    /* Loop principale */
    while (continua_esecuzione) {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        
        /* Timeout di 1 secondo per aggiornare periodicamente il display */
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        result = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
        
        if (result < 0) {
            if (continua_esecuzione) {
                perror("Errore in select");
            }
            break;
        }
        else if (result > 0 && FD_ISSET(sockfd, &readfds)) {
            /* Dati disponibili sul socket */
            client_len = sizeof(client_addr);
            
            if (recvfrom(sockfd, &msg, sizeof(msg), 0, 
                        (struct sockaddr*)&client_addr, &client_len) > 0) {
                
                idx = char_to_index(msg.tipo);
                if (idx >= 0) {
                    /* Aggiorna contatore totale */
                    contatori[idx].totale++;
                    
                    /* Memorizza il messaggio se c'è spazio */
                    if (num_messaggi < MAX_MESSAGGI) {
                        messaggi[num_messaggi].tipo = msg.tipo;
                        messaggi[num_messaggi].timestamp = msg.timestamp;
                        num_messaggi++;
                    }
                    
                    /* Pulisce messaggi vecchi se necessario */
                    if (num_messaggi >= MAX_MESSAGGI - 100) {
                        pulisci_messaggi_vecchi();
                    }
                }
            }
        }
        
        /* Aggiorna contatori e display */
        aggiorna_contatori();
        mostra_statistiche();
    }
    
    /* Cleanup */
    setsockopt(sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
    close(sockfd);
    
    printf("Server terminato.\n");
    return 0;
}