# Comunicazione TCP con Thread - Sincronizzazione Variabile di Ambiente

## Traccia dell'Esercizio

Un processo client ha due thread che cambiano di continuo a intervalli di tempo casuali il valore di una variabile d'ambiente A, assegnandole un numero intero casuale.

**Requisiti:**
1. Gestire la concorrenza delle operazioni svolte dalle thread
2. Un'altra thread comunichi via TCP a un server ogni valore modificato della variabile d'ambiente A
3. Il server deve aggiornare ad ogni ricezione il valore della sua variabile d'ambiente A
4. I due processi devono procedere fino a quando il client non sia interrotto
5. Non appena il client viene interrotto, anche il server deve essere interrotto

## Architettura del Sistema

### Client (Multi-Thread)
```
┌─────────────────────────────────────┐
│               CLIENT                │
├─────────────────────────────────────┤
│ Thread 1: Ricezione dal server      │
│ Thread 2: Modificatore 1 (rand A)   │
│ Thread 3: Modificatore 2 (rand A)   │
│ Thread 4: TCP Sender (invia A)      │
├─────────────────────────────────────┤
│ Variabile globale: A                │
│ Mutex: A_mutex, print_mutex         │
│ Flags: A_changed, client_running    │
└─────────────────────────────────────┘
```

### Server (Sequenziale)
```
┌─────────────────────────────────────┐
│               SERVER                │
├─────────────────────────────────────┤
│ Main loop: accept + recv/send       │
│ Gestisce un client alla volta       │
├─────────────────────────────────────┤
│ Variabile locale: A                 │
│ Aggiorna variabile ambiente         │
└─────────────────────────────────────┘
```

## Funzionamento Dettagliato

### Client

#### Thread Modificatori (2 thread)
- **Funzione:** `thread_modificatore1()` e `thread_modificatore2()`
- **Comportamento:**
  - Generano numeri casuali (1-1000)
  - Chiamano `update_A()` per aggiornare la variabile
  - Dormono per tempo casuale (700-4000ms)
- **Sincronizzazione:** Usano mutex `A_mutex` tramite `update_A()`

#### Funzione update_A()
```c
void update_A(int new_value, int thread_id) {
    pthread_mutex_lock(&A_mutex);
    A = new_value;                    // Aggiorna variabile globale
    sprintf(A_string, "%d", A);
    setenv("A", A_string, 1);         // Aggiorna variabile ambiente
    A_changed = 1;                    // Flag per notificare cambio
    pthread_mutex_unlock(&A_mutex);
}
```

#### Thread TCP Sender
- **Funzione:** `thread_tcp_sender()`
- **Comportamento:**
  - Monitora il flag `A_changed`
  - Quando A cambia, invia il nuovo valore al server
  - Previene invii duplicati con `last_A_send`
- **Polling:** Ogni 100ms

#### Thread Ricezione
- **Funzione:** `thread_ricezione()`
- **Comportamento:**
  - Riceve messaggi dal server (echo dei valori)
  - Gestisce disconnessioni
  - Primo messaggio = benvenuto, successivi = echo numeri

### Server

#### Main Loop
1. **Accept:** Aspetta connessioni client
2. **Receive:** Riceve valori numerici dal client
3. **Update:** Aggiorna variabile A locale e di ambiente
4. **Echo:** Rimanda il valore al client
5. **Disconnection:** Quando client si disconnette, server termina

## Gestione della Concorrenza

### Mutex Utilizzati
- **`A_mutex`**: Protegge accesso alla variabile globale A
- **`print_mutex`**: Protegge le operazioni di stampa

### Flags di Sincronizzazione
- **`A_changed`**: Notifica quando A è stata modificata
- **`client_running`**: Flag globale per terminazione coordinata

### Race Conditions Evitate
1. **Accesso concorrente ad A**: Risolto con `A_mutex`
2. **Invii duplicati**: Risolto con `last_A_send`
3. **Stampe sovrapposte**: Risolto con `print_mutex`

## Terminazione Coordinata

### Scenario di Terminazione
1. Client riceve SIGINT (Ctrl+C)
2. `client_running = 0` viene impostato
3. Tutti i thread client terminano i loro loop
4. Connection TCP si chiude
5. Server rileva disconnessione e termina

### Cleanup Resources
```c
// Nel client
pthread_join() per tutti i thread
close(socket_fd)
pthread_mutex_destroy()
```

## Punti Critici dell'Implementazione

### Gestione Errori
- **Conversione stringhe**: `safe_string_to_int()` con controlli errno
- **Operazioni socket**: Controllo valore di ritorno recv/send
- **Thread creation**: Controllo fallimento pthread_create

### Problemi Potenziali
1. **Deadlock**: Evitato con ordine coerente acquisizione mutex
2. **Starvation**: Possibile con thread modificatori molto attivi
3. **Buffer overflow**: Evitato con bounds checking

### Ottimizzazioni Possibili
- Usare condition variables invece di polling
- Implementare backpressure se server è lento
- Aggiungere timeout sui socket

## Compilazione ed Esecuzione

```bash
# Compilazione
gcc -o server server.c
gcc -o client client.c -lpthread

# Esecuzione
./server                    # Terminale 1
./client                    # Terminale 2
```

## Variabili di Ambiente

Entrambi i processi mantengono sincronizzata la variabile di ambiente "A":
```bash
echo $A    # Mostra valore corrente
```

Questo permette ad altri processi di leggere il valore aggiornato di A senza comunicazione diretta.