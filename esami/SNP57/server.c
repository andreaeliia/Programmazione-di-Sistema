# Sistema Feedback Lezione Real-Time

## Descrizione del Sistema

Sistema client-server per monitoraggio in tempo reale del feedback degli studenti durante una lezione tramite comunicazione multicast UDP.

**Componenti:**
- **Client**: Applicazione per studenti che inviano feedback anonimi premendo singoli tasti
- **Server**: Applicazione per docente che riceve e mostra statistiche in tempo reale

## Struttura del Progetto

```
client.c      - Programma client per studenti
server.c      - Programma server per docente  
README.md     - Questa documentazione
```

## Compilazione

Entrambi i programmi sono compatibili con standard C90:

```bash
# Compila client e server
gcc -ansi -Wall client.c -o client
gcc -ansi -Wall server.c -o server

# Compilazione con debug
gcc -ansi -Wall -g client.c -o client
gcc -ansi -Wall -g server.c -o server
```

## Utilizzo del Sistema

### 1. Avvio del Server (Docente)

```bash
./server <indirizzo_multicast>
```

Esempio:
```bash
./server 224.1.1.1
```

Il server mostra immediatamente un dashboard che si aggiorna ogni secondo con:
- Durata della lezione
- Statistiche per categoria (Totale, Ultimi 5 min, Ultimo min)
- Indicatori di gradimento
- Status della lezione

### 2. Avvio del Client (Studenti)

```bash
./client <indirizzo_multicast>
```

Esempio:
```bash
./client 224.1.1.1
```

**Comandi disponibili per gli studenti:**
- **A** = Applauso
- **R** = Ripetere  
- **I** = Incomprensibile
- **N** = Troppo noioso
- **C** = Continuare così
- **B** = Interrompere, inutile continuare
- **Q** = Quit (termina client)

**Caratteristiche:**
- Input immediato senza premere Invio
- Feedback anonimo tramite multicast
- Conferma visiva dell'invio

## Example Output

### Server (Dashboard Docente)
```
=== MONITORAGGIO FEEDBACK LEZIONE IN TEMPO REALE ===
Durata lezione: 15:23    Messaggi totali: 47
Aggiornamento: Mon Oct 30 14:25:15 2023

┌─────────────────────────────┬────────┬──────────┬─────────┐
│ Categoria                   │ Totale │ Ult.5min │ Ult.1min│
├─────────────────────────────┼────────┼──────────┼─────────┤
│ [A] Applauso               │     12 │        3 │       1 │
│ [R] Ripetere               │      8 │        2 │       0 │
│ [I] Incomprensibile        │      3 │        1 │       0 │
│ [N] Troppo noioso          │      5 │        0 │       0 │
│ [C] Continuare così        │     15 │        4 │       2 │
│ [B] Interrompere           │      4 │        0 │       0 │
├─────────────────────────────┼────────┼──────────┼─────────┤
│ TOTALE                      │     47 │       10 │       3 │
└─────────────────────────────┴────────┴──────────┴─────────┘

INDICATORI:
Feedback ultimo minuto: Positivo 100% | Negativo 0%
Status: ✓ OTTIMO - La lezione sta andando bene!

Premi Ctrl+C per terminare il monitoraggio
```

### Client (Interfaccia Studente)
```
=== CLIENT FEEDBACK STUDENTI ===
Premi uno dei seguenti tasti per inviare feedback:

  A = Applauso
  R = Ripetere
  I = Incomprensibile
  N = Troppo noioso
  C = Continuare così
  B = Interrompere, inutile continuare
  Q = Quit (termina client)

I messaggi sono inviati immediatamente senza premere Invio
Pronto per ricevere input...

Inviato: [A] Applauso (totale inviati: 1)
Inviato: [C] Continuare così (totale inviati: 2)
```

## Architettura Tecnica

### Comunicazione Multicast UDP

**Vantaggi del Multicast:**
- Un messaggio raggiunge tutti i subscriber (server + eventuali monitor)
- Efficiente per comunicazione uno-a-molti
- Anonimato degli studenti (nessun indirizzo IP tracciabile)
- Scalabilità per classi numerose

**Configurazione di Rete:**
- Porta fissa: 12345
- Range indirizzi multicast validi: 224.0.0.0 - 239.255.255.255
- Protocollo: UDP (veloce, senza connessione)

### Client (Studenti)

**Funzionalità principali:**
```c
/* Input raw senza invio */
int setup_raw_terminal()

/* Invio feedback multicast */
int send_feedback(int sock, struct sockaddr_in *addr, char feedback_type)

/* Validazione tasti */
int is_valid_feedback(char c)
```

**Caratteristiche implementate:**
- **Input raw**: Lettura tasti senza attendere Invio usando `termios`
- **Invio immediato**: Messaggi UDP inviati istantaneamente
- **Validazione input**: Solo tasti validi (A,R,I,N,C,B,Q) accettati
- **Feedback visivo**: Conferma di ogni messaggio inviato
- **Cleanup automatico**: Ripristino terminale alla chiusura

### Server (Docente)

**Strutture dati principali:**
```c
/* Messaggio ricevuto */
typedef struct {
    char type;      /* A, R, I, N, C, B */
    char padding;   /* Allineamento */
} FeedbackMessage;

/* Cronologia messaggi */
typedef struct {
    char type;
    time_t timestamp;
} MessageRecord;

/* Statistiche per categoria */
typedef struct {
    char type;
    const char *description;
    int total;          /* Dall'inizio */
    int last_5_min;     /* Ultimi 5 minuti */
    int last_1_min;     /* Ultimo minuto */
} CategoryStats;
```

**Funzionalità principali:**
```c
/* Setup socket multicast receiver */
int setup_multicast_socket(const char *multicast_addr)

/* Aggiorna contatori finestre temporali */
void update_statistics()

/* Display real-time dashboard */
void display_statistics()

/* Tracking messaggi nel tempo */
void add_message_to_history(char type)
```

## Funzionalità Avanzate

### Tracking Temporale
- **Cronologia completa**: Fino a 10.000 messaggi con timestamp
- **Finestre scorrevoli**: Calcolo dinamico per 1 min e 5 min
- **Buffer circolare**: Gestione efficiente memoria per sessioni lunghe

### Dashboard Real-Time
- **Aggiornamento automatico**: Refresh ogni secondo
- **Clear screen**: Display pulito con escape sequences ANSI
- **Tabella formattata**: Layout professionale con caratteri box-drawing
- **Indicatori visivi**: Status automatico basato su feedback positivo/negativo

### Analisi Automatica
```c
/* Calcolo sentiment */
int positive = stats[0].last_1_min + stats[4].last_1_min; /* A + C */
int negative = stats[1].last_1_min + stats[2].last_1_min + 
               stats[3].last_1_min + stats[5].last_1_min; /* R + I + N + B */

/* Status automatico */
if (positive_percent >= 70) -> "OTTIMO"
if (positive_percent >= 50) -> "ACCETTABILE"  
if (total_feedback >= 3)    -> "ATTENZIONE"
else                        -> "POCHI FEEDBACK"
```

## Configurazione di Rete

### Indirizzi Multicast Consigliati
```bash
# Per rete locale (non attraversa router)
224.1.1.1    # Uso generale
224.1.1.2    # Lezione backup

# Per rete dipartimentale  
239.255.1.1  # Range site-local
```

### Firewall e Routing
```bash
# Linux: Permetti multicast su interfaccia
sudo iptables -A INPUT -d 224.0.0.0/4 -j ACCEPT
sudo iptables -A OUTPUT -d 224.0.0.0/4 -j ACCEPT

# Verifica supporto multicast
ip maddr show
```

### Test di Connettività
```bash
# Test ricezione multicast (server)
tcpdump -i any -n host 224.1.1.1

# Test invio multicast (client)  
echo "test" | nc -u 224.1.1.1 12345
```

## Casi d'Uso

### Lezione Standard
1. **Setup**: Docente avvia server prima della lezione
2. **Distribuzione**: Studenti si connettono con client durante lezione
3. **Monitoring**: Docente monitora feedback in tempo reale
4. **Adattamento**: Modifica approccio basandosi sui feedback

### Lezione Online
- Funziona con VPN o rete locale virtuale
- Possibile integrazione con piattaforme videoconferenza
- Monitoring asincrono per lezioni registrate

### Esami/Test
- Feedback su difficoltà domande
- Richieste di chiarimento anonimi
- Monitoring stress degli studenti

## Limitazioni e Considerazioni

### Limitazioni Tecniche
- **Rete locale**: Multicast tipicamente non attraversa Internet
- **Affidabilità**: UDP può perdere pacchetti (rare in LAN)
- **Sicurezza**: Nessuna autenticazione (per anonimato)
- **Scala**: Testato fino a ~100 client simultanei

### Considerazioni Didattiche
- **Effetto novità**: Studenti potrebbero abusare inizialmente
- **Rappresentatività**: Solo studenti connessi forniscono feedback
- **Interpretazione**: Feedback negativi richiedono attenzione, non panico
- **Privacy**: Sistema anonimo, nessun tracking individuale

## Troubleshooting

### Problemi Comuni

**Client non invia messaggi**
```bash
# Verifica indirizzo multicast
ping 224.1.1.1  # Dovrebbe fallire (normale per multicast)

# Controlla permessi raw socket
# Alcuni sistemi richiedono privilegi per raw terminal
```

**Server non riceve messaggi**
```bash
# Verifica bind su porta
netstat -ulpn | grep 12345

# Test manuale invio
echo -n "A" | nc -u 224.1.1.1 12345
```

**Terminale client compromesso**
```bash
# Ripristina manualmente se client crash
reset
# oppure
stty sane
```

**Display server corrotto**
```bash
# Terminale troppo piccolo per tabella
# Ridimensiona finestra terminale o usa font più piccolo
export TERM=xterm-256color
```

### Debug

**Modalità Debug Server**
```c
/* Aggiungi per debug dettagliato */
#define DEBUG 1

#ifdef DEBUG
printf("DEBUG: Ricevuto messaggio tipo '%c' timestamp %ld\n", type, timestamp);
#endif
```

**Monitor Traffico di Rete**
```bash
# Monitora traffico multicast
sudo tcpdump -i any -n -v host 224.1.1.1 and port 12345

# Verifica membership gruppo multicast
cat /proc/net/igmp
```

## Note di Compatibilità C90

### Conformità Standard
- Tutte le dichiarazioni variabili all'inizio delle funzioni
- Solo commenti `/* */`
- Nessuna funzionalità C99 (VLA, dichiarazioni miste, compound literals)
- Compatibile con flag `-ansi`

### Dipendenze di Sistema
- **POSIX Sockets**: BSD socket API standard
- **POSIX Terminal**: `termios.h` per raw input
- **POSIX Time**: `time.h` per timestamp
- **System V**: `signal.h` per gestione segnali

## Estensioni Possibili

### Funzionalità Aggiuntive
- **Autenticazione**: Login studenti per statistiche non anonime
- **Persistenza**: Salvataggio cronologia su file
- **Web Interface**: Dashboard web per remote monitoring
- **Mobile App**: Client per smartphone
- **Analytics**: Correlazione temporale con slide/argomenti
- **Integration**: API per LMS (Moodle, Blackboard)

### Miglioramenti Tecnici
- **Reliable UDP**: Acknowledgment per garantire ricezione
- **Encryption**: Crittografia per prevenire spam
- **Load Balancing**: Multiple server per scalabilità
- **Cloud**: Deployment su infrastruttura cloud
- **Real-time WebSocket**: Dashboard web live