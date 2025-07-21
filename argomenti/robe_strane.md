SETTARE IL TERMINALE IN MODALITA RAW (non c'e' bisogno di invio)

/* Funzione per impostare il terminale in modalità raw */

void set_raw_mode(struct termios *orig_termios) {
    struct termios raw;
    
    tcgetattr(STDIN_FILENO, orig_termios);
    raw = *orig_termios;
    
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}




/* Funzione per ripristinare il terminale */
void restore_terminal(struct termios *orig_termios) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, orig_termios);
}




APPUNTI SU GETRUSAGE E PRECISIONE DEL TEMPO
PROBLEMA DELLA PRECISIONE

times() ha precisione limitata da _SC_CLK_TCK (di solito 100 Hz = 10ms)
Per ottenere precisione del microsecondo servono altre funzioni
gettimeofday() per wall time (precisione microsecondo)
getrusage() per user/system time (precisione microsecondo)

GETRUSAGE
Funzione: int getrusage(int who, struct rusage *usage)
Parametro who:

RUSAGE_SELF: statistiche del processo corrente
RUSAGE_CHILDREN: statistiche cumulative di tutti i processi figli terminati

Struct rusage contiene:

ru_utime: tempo CPU in modalita utente (struct timeval)
ru_stime: tempo CPU in modalita sistema (struct timeval)
Altri campi per memoria, I/O, ecc.

Struct timeval:

tv_sec: secondi (time_t)
tv_usec: microsecondi (suseconds_t)

PROBLEMA CUMULATIVO
getrusage(RUSAGE_CHILDREN) restituisce SEMPRE valori cumulativi di tutti i figli terminati:

1° comando: rusage restituisce tempi del 1° comando
2° comando: rusage restituisce tempi del 1° + 2° comando
3° comando: rusage restituisce tempi del 1° + 2° + 3° comando

SOLUZIONE: MISURARE DIFFERENZE
Per ottenere tempi del singolo comando:

Misurare getrusage() PRIMA del comando
Eseguire il comando
Misurare getrusage() DOPO il comando
Calcolare la differenza: dopo - prima

CODICE ESEMPIO:
struct rusage before, after;
getrusage(RUSAGE_CHILDREN, &before);
system(comando);
getrusage(RUSAGE_CHILDREN, &after);
double user_time = (after.ru_utime.tv_sec - before.ru_utime.tv_sec) +
(after.ru_utime.tv_usec - before.ru_utime.tv_usec) / 1000000.0;
double sys_time = (after.ru_stime.tv_sec - before.ru_stime.tv_sec) +
(after.ru_stime.tv_usec - before.ru_stime.tv_usec) / 1000000.0;
GETTIMEOFDAY
Funzione: int gettimeofday(struct timeval *tv, struct timezone *tz)

Restituisce tempo di parete corrente con precisione microsecondo
Parametro tz normalmente NULL

COMBINAZIONE PER PRECISIONE COMPLETA:

Wall time: gettimeofday() prima e dopo il comando
User/sys time: getrusage() prima e dopo il comando con calcolo differenze

CONVERSIONE TIMEVAL A SECONDI:
double time_in_seconds = tv.tv_sec + tv.tv_usec / 1000000.0;
ERRORI COMUNI:

Usare getrusage() senza calcolare differenze (valori cumulativi)
Dimenticare di includere <sys/resource.h> per getrusage
Dimenticare di includere <sys/time.h> per gettimeofday
Fare divisione intera invece di 1000000.0 per i microsecond
