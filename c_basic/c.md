# C - Riferimento Completo per Esame

## 📚 INDICE RAPIDO
1. [Tipi di Dati e Variabili](#tipi-di-dati)
2. [Input/Output Base](#input-output-base)
3. [Array e Matrici](#array-e-matrici)
4. [Stringhe](#stringhe)
5. [Puntatori](#puntatori)
6. [Funzioni](#funzioni)
7. [Struct](#struct)
8. [File I/O](#file-io)
9. [Gestione Tempo](#gestione-tempo)
10. [Allocazione Dinamica](#allocazione-dinamica)
11. [**Liste Concatenate**](#liste-concatenate) 
12. [Preprocessore](#preprocessore)
13. [Esempi Pratici](#esempi-pratici)
14. [**Validazione e Controllo Variabili**](#validazione-e-controllo-variabili) 
15. [**Casting e Conversione**](#casting-e-conversione) 


---

## TIPI DI DATI

### Tipi Base
```c
// Interi
int numero = 42;              // 32 bit, -2^31 a 2^31-1
short piccolo = 100;          // 16 bit
long grande = 1000000L;       // 64 bit
long long enorme = 123456789LL;

// Unsigned (solo positivi)
unsigned int positivo = 42U;
unsigned char byte = 255;     // 0-255

// Caratteri
char lettera = 'A';           // 8 bit
char stringa[] = "Ciao";      // Array di char

// Decimali
float decimale = 3.14f;       // 32 bit, 6-7 cifre
double preciso = 3.141592653; // 64 bit, 15-17 cifre

// Booleani (C99+)
#include <stdbool.h>
bool vero = true;
bool falso = false;
```

### Costanti e Modificatori
```c
const int COSTANTE = 100;     // Non modificabile
static int locale = 0;        // Variabile statica
extern int globale;           // Dichiarazione esterna
volatile int hardware = 0;    // Può cambiare esternamente

#define PI 3.14159           // Macro costante
#define MAX_SIZE 1000        // Macro numerica
```

---

## INPUT/OUTPUT BASE

### Printf - Formattazione Output
```c
#include <stdio.h>

int main() {
    int numero = 42;
    float decimale = 3.14f;
    char carattere = 'A';
    char stringa[] = "Mondo";
    
    // Formati base
    printf("Intero: %d\n", numero);           // 42
    printf("Intero (hex): %x\n", numero);     // 2a
    printf("Intero (oct): %o\n", numero);     // 52
    printf("Float: %f\n", decimale);          // 3.140000
    printf("Float (2 dec): %.2f\n", decimale); // 3.14
    printf("Carattere: %c\n", carattere);     // A
    printf("Stringa: %s\n", stringa);         // Mondo
    printf("Puntatore: %p\n", &numero);       // 0x7fff...
    
    // Larghezza campo
    printf("Numero: %5d\n", numero);          // "   42"
    printf("Numero: %-5d|\n", numero);        // "42   |"
    printf("Float: %8.2f\n", decimale);       // "    3.14"
    
    // Con zero padding
    printf("Zero pad: %05d\n", numero);       // "00042"
    
    return 0;
}
```

### Scanf - Input da Tastiera
```c
#include <stdio.h>

int main() {
    int numero;
    float decimale;
    char carattere;
    char stringa[100];
    
    // Input base
    printf("Inserisci un numero: ");
    scanf("%d", &numero);
    
    printf("Inserisci un decimale: ");
    scanf("%f", &decimale);
    
    printf("Inserisci un carattere: ");
    scanf(" %c", &carattere);  // Spazio prima di %c!
    
    printf("Inserisci una parola: ");
    scanf("%s", stringa);       // NO & per stringhe!
    
    // Input multiplo
    printf("Inserisci 3 numeri: ");
    int a, b, c;
    scanf("%d %d %d", &a, &b, &c);
    
    // Limitare lunghezza stringa
    printf("Inserisci nome (max 99 char): ");
    scanf("%99s", stringa);
    
    return 0;
}
```

### Fgets - Input Linea Completa (Più Sicuro)
```c
#include <stdio.h>
#include <string.h>

int main() {
    char linea[256];
    
    printf("Inserisci una frase: ");
    fgets(linea, sizeof(linea), stdin);
    
    // Rimuovi newline finale
    linea[strcspn(linea, "\n")] = 0;
    
    printf("Hai scritto: '%s'\n", linea);
    
    // Convertire stringa in numero
    int numero = atoi(linea);
    float decimale = atof(linea);
    
    return 0;
}
```

---

## ARRAY E MATRICI

### Array Monodimensionali
```c
#include <stdio.h>

int main() {
    // Dichiarazione e inizializzazione
    int numeri[5] = {10, 20, 30, 40, 50};
    int altri[] = {1, 2, 3};  // Dimensione automatica
    int vuoto[10];            // Non inizializzato
    int zeri[10] = {0};       // Tutti a zero
    
    // Accesso elementi
    printf("Primo: %d\n", numeri[0]);     // 10
    printf("Ultimo: %d\n", numeri[4]);    // 50
    
    // Modificare elementi
    numeri[0] = 100;
    
    // Dimensione array
    int dimensione = sizeof(numeri) / sizeof(numeri[0]);
    printf("Dimensione: %d\n", dimensione);  // 5
    
    // Scorrere array
    for (int i = 0; i < 5; i++) {
        printf("numeri[%d] = %d\n", i, numeri[i]);
    }
    
    // Input array
    printf("Inserisci 5 numeri:\n");
    for (int i = 0; i < 5; i++) {
        printf("Numero %d: ", i+1);
        scanf("%d", &numeri[i]);
    }
    
    return 0;
}
```

### Matrici (Array Bidimensionali)
```c
#include <stdio.h>

int main() {
    // Dichiarazione matrici
    int matrice[3][4];                    // 3 righe, 4 colonne
    int inizializzata[2][3] = {
        {1, 2, 3},
        {4, 5, 6}
    };
    
    // Accesso elementi
    matrice[0][0] = 10;  // Prima riga, prima colonna
    matrice[2][3] = 99;  // Terza riga, quarta colonna
    
    // Input matrice
    printf("Inserisci elementi matrice 3x4:\n");
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 4; j++) {
            printf("Elemento [%d][%d]: ", i, j);
            scanf("%d", &matrice[i][j]);
        }
    }
    
    // Stampa matrice
    printf("\nMatrice:\n");
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 4; j++) {
            printf("%4d ", matrice[i][j]);
        }
        printf("\n");
    }
    
    // Trovare elemento in matrice
    int cerca = 5;
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 3; j++) {
            if (inizializzata[i][j] == cerca) {
                printf("Trovato %d in posizione [%d][%d]\n", cerca, i, j);
            }
        }
    }
    
    return 0;
}
```

### Operazioni Array Comuni
```c
#include <stdio.h>

// Trova massimo in array
int trova_massimo(int arr[], int size) {
    int max = arr[0];
    for (int i = 1; i < size; i++) {
        if (arr[i] > max) {
            max = arr[i];
        }
    }
    return max;
}

// Somma elementi array
int somma_array(int arr[], int size) {
    int somma = 0;
    for (int i = 0; i < size; i++) {
        somma += arr[i];
    }
    return somma;
}

// Cerca elemento in array
int cerca_elemento(int arr[], int size, int elemento) {
    for (int i = 0; i < size; i++) {
        if (arr[i] == elemento) {
            return i;  // Ritorna indice
        }
    }
    return -1;  // Non trovato
}

int main() {
    int numeri[] = {5, 2, 8, 1, 9, 3};
    int size = sizeof(numeri) / sizeof(numeri[0]);
    
    printf("Massimo: %d\n", trova_massimo(numeri, size));
    printf("Somma: %d\n", somma_array(numeri, size));
    
    int pos = cerca_elemento(numeri, size, 8);
    if (pos != -1) {
        printf("Elemento 8 trovato in posizione %d\n", pos);
    }
    
    return 0;
}
```

---

## STRINGHE

### Operazioni Base
```c
#include <stdio.h>
#include <string.h>

int main() {
    // Dichiarazione stringhe
    char str1[100] = "Ciao";
    char str2[] = "Mondo";
    char str3[100];
    
    // Lunghezza stringa
    printf("Lunghezza str1: %zu\n", strlen(str1));  // 4
    
    // Copia stringhe
    strcpy(str3, str1);           // str3 = "Ciao"
    strncpy(str3, str1, 99);      // Copia max 99 caratteri
    
    // Concatenazione
    strcat(str1, " ");            // str1 = "Ciao "
    strcat(str1, str2);           // str1 = "Ciao Mondo"
    strncat(str1, "!", 1);        // str1 = "Ciao Mondo!"
    
    // Confronto stringhe
    if (strcmp(str1, str2) == 0) {
        printf("Stringhe uguali\n");
    }
    if (strncmp(str1, "Ciao", 4) == 0) {
        printf("str1 inizia con 'Ciao'\n");
    }
    
    // Cerca carattere
    char *pos = strchr(str1, 'M');
    if (pos != NULL) {
        printf("'M' trovato in posizione %ld\n", pos - str1);
    }
    
    // Cerca sottostringa
    char *sub = strstr(str1, "Mondo");
    if (sub != NULL) {
        printf("'Mondo' trovato\n");
    }
    
    return 0;
}
```

### Input/Output Stringhe
```c
#include <stdio.h>
#include <string.h>

int main() {
    char nome[50];
    char cognome[50];
    char frase[200];
    
    // Input parola singola
    printf("Inserisci nome: ");
    scanf("%49s", nome);  // Limita lunghezza
    
    // Pulisci buffer
    while (getchar() != '\n');
    
    // Input frase completa
    printf("Inserisci una frase: ");
    fgets(frase, sizeof(frase), stdin);
    
    // Rimuovi newline finale
    frase[strcspn(frase, "\n")] = 0;
    
    printf("Nome: %s\n", nome);
    printf("Frase: %s\n", frase);
    
    // Conversione case
    for (int i = 0; nome[i]; i++) {
        if (nome[i] >= 'a' && nome[i] <= 'z') {
            nome[i] = nome[i] - 'a' + 'A';  // Maiuscolo
        }
    }
    
    printf("Nome maiuscolo: %s\n", nome);
    
    return 0;
}
```

---

## PUNTATORI

### Concetti Base
```c
#include <stdio.h>

int main() {
    int numero = 42;
    int *puntatore;              // Dichiarazione puntatore
    
    puntatore = &numero;         // Assegna indirizzo di numero
    
    printf("Valore numero: %d\n", numero);           // 42
    printf("Indirizzo numero: %p\n", &numero);       // 0x7fff...
    printf("Valore puntatore: %p\n", puntatore);     // 0x7fff...
    printf("Valore puntato: %d\n", *puntatore);      // 42
    
    // Modifica tramite puntatore
    *puntatore = 100;
    printf("Nuovo valore numero: %d\n", numero);     // 100
    
    // Puntatori e array
    int array[] = {10, 20, 30, 40, 50};
    int *ptr_array = array;      // array è già un puntatore
    
    printf("Primo elemento: %d\n", *ptr_array);      // 10
    printf("Secondo elemento: %d\n", *(ptr_array + 1)); // 20
    printf("Terzo elemento: %d\n", ptr_array[2]);    // 30
    
    // Aritmetica puntatori
    ptr_array++;                 // Punta al secondo elemento
    printf("Nuovo primo: %d\n", *ptr_array);         // 20
    
    return 0;
}
```

### Puntatori e Funzioni
```c
#include <stdio.h>

// Passaggio per valore (non modifica originale)
void incrementa_valore(int x) {
    x++;
    printf("Dentro funzione: %d\n", x);
}

// Passaggio per riferimento (modifica originale)
void incrementa_riferimento(int *x) {
    (*x)++;
    printf("Dentro funzione: %d\n", *x);
}

// Swap di due variabili
void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

int main() {
    int numero = 5;
    
    printf("Prima: %d\n", numero);           // 5
    incrementa_valore(numero);               // Dentro: 6
    printf("Dopo valore: %d\n", numero);    // 5 (non cambiato)
    
    incrementa_riferimento(&numero);         // Dentro: 6
    printf("Dopo riferimento: %d\n", numero); // 6 (cambiato!)
    
    int x = 10, y = 20;
    printf("Prima swap: x=%d, y=%d\n", x, y);
    swap(&x, &y);
    printf("Dopo swap: x=%d, y=%d\n", x, y);
    
    return 0;
}
```

---

## FUNZIONI

### Definizione e Chiamata
```c
#include <stdio.h>

// Dichiarazione (prototipo)
int somma(int a, int b);
float media(int arr[], int size);
void stampa_messaggio(char *msg);

// Definizione
int somma(int a, int b) {
    return a + b;
}

float media(int arr[], int size) {
    int totale = 0;
    for (int i = 0; i < size; i++) {
        totale += arr[i];
    }
    return (float)totale / size;
}

void stampa_messaggio(char *msg) {
    printf("Messaggio: %s\n", msg);
}

// Funzione ricorsiva
int fattoriale(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * fattoriale(n - 1);
}

// Funzione con parametri opzionali (variadic)
#include <stdarg.h>
int somma_variabile(int count, ...) {
    va_list args;
    va_start(args, count);
    
    int totale = 0;
    for (int i = 0; i < count; i++) {
        totale += va_arg(args, int);
    }
    
    va_end(args);
    return totale;
}

int main() {
    // Chiamate funzioni
    int risultato = somma(5, 3);
    printf("Somma: %d\n", risultato);
    
    int numeri[] = {1, 2, 3, 4, 5};
    float med = media(numeri, 5);
    printf("Media: %.2f\n", med);
    
    stampa_messaggio("Ciao mondo!");
    
    printf("5! = %d\n", fattoriale(5));
    
    printf("Somma variabile: %d\n", somma_variabile(4, 1, 2, 3, 4));
    
    return 0;
}
```

---

## STRUCT

### Definizione e Uso
```c
#include <stdio.h>
#include <string.h>

// Definizione struct
struct Persona {
    char nome[50];
    char cognome[50];
    int eta;
    float altezza;
};

// Typedef per semplificare
typedef struct {
    int x;
    int y;
} Punto;

typedef struct {
    char titolo[100];
    char autore[50];
    int pagine;
    float prezzo;
} Libro;

int main() {
    // Dichiarazione e inizializzazione
    struct Persona p1;
    strcpy(p1.nome, "Mario");
    strcpy(p1.cognome, "Rossi");
    p1.eta = 30;
    p1.altezza = 1.75f;
    
    // Inizializzazione diretta
    struct Persona p2 = {"Luigi", "Verdi", 25, 1.80f};
    
    // Con typedef
    Punto origine = {0, 0};
    Punto centro = {100, 50};
    
    Libro libro1 = {
        "Il Signore degli Anelli",
        "Tolkien",
        1200,
        25.99f
    };
    
    // Accesso membri
    printf("Persona 1: %s %s, %d anni, %.2fm\n", 
           p1.nome, p1.cognome, p1.eta, p1.altezza);
    
    printf("Punto centro: (%d, %d)\n", centro.x, centro.y);
    
    printf("Libro: '%s' di %s - %d pagine - €%.2f\n",
           libro1.titolo, libro1.autore, libro1.pagine, libro1.prezzo);
    
    return 0;
}
```

### Array di Struct
```c
#include <stdio.h>
#include <string.h>

typedef struct {
    int id;
    char nome[50];
    float voto;
} Studente;

int main() {
    Studente classe[3] = {
        {1, "Mario", 8.5f},
        {2, "Luigi", 7.0f},
        {3, "Peach", 9.5f}
    };
    
    // Stampa tutti gli studenti
    printf("Lista studenti:\n");
    for (int i = 0; i < 3; i++) {
        printf("ID: %d, Nome: %s, Voto: %.1f\n",
               classe[i].id, classe[i].nome, classe[i].voto);
    }
    
    // Trova studente con voto più alto
    float voto_max = classe[0].voto;
    int indice_max = 0;
    
    for (int i = 1; i < 3; i++) {
        if (classe[i].voto > voto_max) {
            voto_max = classe[i].voto;
            indice_max = i;
        }
    }
    
    printf("\nMigliore studente: %s con voto %.1f\n",
           classe[indice_max].nome, voto_max);
    
    return 0;
}
```

---

## FILE I/O

### Operazioni Base su File
```c
#include <stdio.h>
#include <stdlib.h>

int main() {
    FILE *file;
    char buffer[256];
    
    // SCRITTURA FILE
    file = fopen("output.txt", "w");
    if (file == NULL) {
        printf("Errore apertura file in scrittura!\n");
        return 1;
    }
    
    fprintf(file, "Prima riga\n");
    fprintf(file, "Numero: %d\n", 42);
    fprintf(file, "Decimale: %.2f\n", 3.14f);
    fclose(file);
    printf("File scritto con successo!\n");
    
    // LETTURA FILE
    file = fopen("output.txt", "r");
    if (file == NULL) {
        printf("Errore apertura file in lettura!\n");
        return 1;
    }
    
    printf("\nContenuto file:\n");
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        printf("%s", buffer);
    }
    fclose(file);
    
    // APPEND (aggiungere al file)
    file = fopen("output.txt", "a");
    if (file != NULL) {
        fprintf(file, "Riga aggiunta\n");
        fclose(file);
    }
    
    return 0;
}
```

### Lettura File con Controllo Errori
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Conta righe in un file
int conta_righe(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return -1;
    }
    
    int count = 0;
    char buffer[1024];
    
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        count++;
    }
    
    fclose(file);
    return count;
}

// Legge riga specifica
int leggi_riga(const char *filename, int numero_riga, char *output, int max_len) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return -1;
    }
    
    char buffer[1024];
    int riga_corrente = 1;
    
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        if (riga_corrente == numero_riga) {
            strncpy(output, buffer, max_len - 1);
            output[max_len - 1] = '\0';
            // Rimuovi newline finale
            output[strcspn(output, "\n")] = 0;
            fclose(file);
            return strlen(output);
        }
        riga_corrente++;
    }
    
    fclose(file);
    return -1;  // Riga non trovata
}

int main() {
    const char *filename = "test.txt";
    
    // Crea file di esempio
    FILE *file = fopen(filename, "w");
    if (file) {
        fprintf(file, "Prima riga del file\n");
        fprintf(file, "Seconda riga importante\n");
        fprintf(file, "Terza riga finale\n");
        fclose(file);
    }
    
    // Conta righe
    int num_righe = conta_righe(filename);
    printf("Il file ha %d righe\n", num_righe);
    
    // Leggi riga specifica
    char riga[256];
    if (leggi_riga(filename, 2, riga, sizeof(riga)) > 0) {
        printf("Riga 2: '%s'\n", riga);
    }
    
    return 0;
}
```

### File Binari
```c
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int id;
    char nome[50];
    float voto;
} Studente;

int main() {
    Studente studenti[] = {
        {1, "Mario", 8.5f},
        {2, "Luigi", 7.0f},
        {3, "Peach", 9.5f}
    };
    
    // SCRITTURA FILE BINARIO
    FILE *file = fopen("studenti.dat", "wb");
    if (file == NULL) {
        printf("Errore apertura file binario!\n");
        return 1;
    }
    
    fwrite(studenti, sizeof(Studente), 3, file);
    fclose(file);
    printf("File binario scritto!\n");
    
    // LETTURA FILE BINARIO
    Studente letti[3];
    file = fopen("studenti.dat", "rb");
    if (file == NULL) {
        printf("Errore lettura file binario!\n");
        return 1;
    }
    
    int elementi_letti = fread(letti, sizeof(Studente), 3, file);
    fclose(file);
    
    printf("Letti %d studenti:\n", elementi_letti);
    for (int i = 0; i < elementi_letti; i++) {
        printf("ID: %d, Nome: %s, Voto: %.1f\n",
               letti[i].id, letti[i].nome, letti[i].voto);
    }
    
    return 0;
}
```

---

## GESTIONE TEMPO

### Time Base
```c
#include <stdio.h>
#include <time.h>
#include <unistd.h>

int main() {
    // Tempo corrente
    time_t ora_corrente = time(NULL);
    printf("Timestamp: %ld\n", ora_corrente);
    
    // Converti in stringa
    printf("Data/ora: %s", ctime(&ora_corrente));
    
    // Formattazione personalizzata
    struct tm *info_tempo = localtime(&ora_corrente);
    
    printf("Anno: %d\n", info_tempo->tm_year + 1900);
    printf("Mese: %d\n", info_tempo->tm_mon + 1);
    printf("Giorno: %d\n", info_tempo->tm_mday);
    printf("Ora: %d\n", info_tempo->tm_hour);
    printf("Minuti: %d\n", info_tempo->tm_min);
    printf("Secondi: %d\n", info_tempo->tm_sec);
    
    // Formattazione con strftime
    char buffer[100];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", info_tempo);
    printf("Formato custom: %s\n", buffer);
    
    strftime(buffer, sizeof(buffer), "%A, %d %B %Y", info_tempo);
    printf("Formato completo: %s\n", buffer);
    
    return 0;
}
```

### Misurare Tempo di Esecuzione
```c
#include <stdio.h>
#include <time.h>
#include <unistd.h>

// Funzione che richiede tempo
void operazione_lenta() {
    for (int i = 0; i < 1000000; i++) {
        // Simula lavoro
    }
    sleep(1);  // Pausa 1 secondo
}

int main() {
    // Metodo 1: clock()
    clock_t inizio = clock();
    
    operazione_lenta();
    
    clock_t fine = clock();
    double tempo_cpu = ((double)(fine - inizio)) / CLOCKS_PER_SEC;
    printf("Tempo CPU: %.2f secondi\n", tempo_cpu);
    
    // Metodo 2: time()
    time_t inizio_wall = time(NULL);
    
    operazione_lenta();
    
    time_t fine_wall = time(NULL);
    double tempo_wall = difftime(fine_wall, inizio_wall);
    printf("Tempo reale: %.0f secondi\n", tempo_wall);
    
    return 0;
}
```

### Timer e Delay
```c
#include <stdio.h>
#include <time.h>
#include <unistd.h>

// Delay in millisecondi (Unix/Linux)
void delay_ms(int milliseconds) {
    usleep(milliseconds * 1000);
}

// Delay in secondi
void delay_sec(int seconds) {
    sleep(seconds);
}

int main() {
    printf("Inizio...\n");
    
    // Countdown
    for (int i = 5; i > 0; i--) {
        printf("Countdown: %d\n", i);
        delay_sec(1);
    }
    
    printf("Via!\n");
    
    // Delay più preciso con nanosleep
    struct timespec delay = {0, 500000000};  // 0.5 secondi
    nanosleep(&delay, NULL);
    
    printf("Fatto!\n");
    
    return 0;
}
```

---

## ALLOCAZIONE DINAMICA

### Malloc, Free, Realloc
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    // Allocazione array dinamico
    int n;
    printf("Quanti numeri vuoi inserire? ");
    scanf("%d", &n);
    
    int *array = malloc(n * sizeof(int));
    if (array == NULL) {
        printf("Errore allocazione memoria!\n");
        return 1;
    }
    
    // Input numeri
    for (int i = 0; i < n; i++) {
        printf("Numero %d: ", i + 1);
        scanf("%d", &array[i]);
    }
    
    // Stampa numeri
    printf("Hai inserito: ");
    for (int i = 0; i < n; i++) {
        printf("%d ", array[i]);
    }
    printf("\n");
    
    // Ridimensiona array (aggiungi 5 elementi)
    array = realloc(array, (n + 5) * sizeof(int));
    if (array == NULL) {
        printf("Errore realloc!\n");
        return 1;
    }
    
    // Inizializza nuovi elementi
    for (int i = n; i < n + 5; i++) {
        array[i] = 0;
    }
    
    printf("Array esteso: ");
    for (int i = 0; i < n + 5; i++) {
        printf("%d ", array[i]);
    }
    printf("\n");
    
    // Libera memoria
    free(array);
    array = NULL;  // Buona pratica
    
    return 0;
}
```

### Stringhe Dinamiche
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* crea_stringa(const char* input) {
    int lunghezza = strlen(input);
    char* nuova = malloc(lunghezza + 1);  // +1 per \0
    
    if (nuova != NULL) {
        strcpy(nuova, input);
    }
    
    return nuova;
}

char* concatena_stringhe(const char* str1, const char* str2) {
    int len1 = strlen(str1);
    int len2 = strlen(str2);
    
    char* risultato = malloc(len1 + len2 + 1);
    if (risultato != NULL) {
        strcpy(risultato, str1);
        strcat(risultato, str2);
    }
    
    return risultato;
}

int main() {
    // Crea stringhe dinamiche
    char* saluto = crea_stringa("Ciao ");
    char* nome = crea_stringa("Mario");
    
    if (saluto && nome) {
        char* messaggio = concatena_stringhe(saluto, nome);
        
        if (messaggio) {
            printf("Messaggio: %s\n", messaggio);
            free(messaggio);
        }
        
        free(saluto);
        free(nome);
    }
    
    return 0;
}
```

---

## LISTE CONCATENATE

### Lista Concatenata Base (con puntatori)
```c
#include <stdio.h>
#include <stdlib.h>

// Nodo della lista
typedef struct Node {
    int data;
    struct Node* next;
} Node;

// Crea nuovo nodo
Node* crea_nodo(int data) {
    Node* nuovo = malloc(sizeof(Node));
    if (nuovo != NULL) {
        nuovo->data = data;
        nuovo->next = NULL;
    }
    return nuovo;
}

// Inserisce all'inizio
Node* inserisci_inizio(Node* head, int data) {
    Node* nuovo = crea_nodo(data);
    if (nuovo != NULL) {
        nuovo->next = head;
        head = nuovo;
    }
    return head;
}

// Inserisce alla fine
Node* inserisci_fine(Node* head, int data) {
    Node* nuovo = crea_nodo(data);
    if (nuovo == NULL) return head;
    
    if (head == NULL) {
        return nuovo;
    }
    
    Node* current = head;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = nuovo;
    
    return head;
}

// Rimuovi primo elemento
Node* rimuovi_primo(Node* head) {
    if (head == NULL) return NULL;
    
    Node* nuovo_head = head->next;
    free(head);
    return nuovo_head;
}

// Cerca elemento
Node* cerca(Node* head, int data) {
    Node* current = head;
    while (current != NULL) {
        if (current->data == data) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Conta elementi
int conta_elementi(Node* head) {
    int count = 0;
    Node* current = head;
    while (current != NULL) {
        count++;
        current = current->next;
    }
    return count;
}

// Stampa lista
void stampa_lista(Node* head) {
    printf("Lista: ");
    Node* current = head;
    while (current != NULL) {
        printf("%d", current->data);
        if (current->next != NULL) {
            printf(" -> ");
        }
        current = current->next;
    }
    printf(" -> NULL\n");
}

// Libera tutta la lista
void libera_lista(Node* head) {
    while (head != NULL) {
        Node* temp = head;
        head = head->next;
        free(temp);
    }
}

int main() {
    Node* lista = NULL;
    
    printf("=== LISTA CONCATENATA BASE ===\n");
    
    // Inserimenti
    lista = inserisci_inizio(lista, 10);
    lista = inserisci_inizio(lista, 20);
    lista = inserisci_fine(lista, 5);
    stampa_lista(lista);  // 20 -> 10 -> 5 -> NULL
    
    // Conta elementi
    printf("Elementi: %d\n", conta_elementi(lista));
    
    // Cerca elemento
    Node* trovato = cerca(lista, 10);
    if (trovato) {
        printf("Elemento 10 trovato!\n");
    }
    
    // Rimuovi primo
    lista = rimuovi_primo(lista);
    stampa_lista(lista);  // 10 -> 5 -> NULL
    
    // Libera memoria
    libera_lista(lista);
    
    return 0;
}
```

### Lista Concatenata in Memoria Condivisa (con offset)
```c
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>

#define MAX_NODES 100
#define SHM_NAME "/lista_condivisa"

// Nodo con offset invece di puntatori
typedef struct {
    int data;
    int next_offset;  // ⭐ Offset invece di puntatore!
    int is_free;      // Per gestire allocazione
} Node;

// Memoria condivisa
typedef struct {
    Node nodes[MAX_NODES];
    int head_offset;      // Offset del primo nodo (-1 se vuota)
    int count;           // Numero elementi
    sem_t mutex;         // Sincronizzazione
} SharedList;

// Converte offset in puntatore
Node* get_node(SharedList* list, int offset) {
    if (offset == -1 || offset >= MAX_NODES) return NULL;
    return &list->nodes[offset];
}

// Converte puntatore in offset
int get_offset(SharedList* list, Node* node) {
    if (node == NULL) return -1;
    return node - list->nodes;
}

// Alloca nuovo nodo dal pool
int alloca_nodo(SharedList* list) {
    for (int i = 0; i < MAX_NODES; i++) {
        if (list->nodes[i].is_free) {
            list->nodes[i].is_free = 0;
            list->nodes[i].next_offset = -1;
            return i;
        }
    }
    return -1;  // Nessun nodo disponibile
}

// Libera nodo
void libera_nodo(SharedList* list, int offset) {
    if (offset >= 0 && offset < MAX_NODES) {
        list->nodes[offset].is_free = 1;
        list->nodes[offset].next_offset = -1;
    }
}

// Inserisce all'inizio (thread-safe)
int inserisci_inizio_safe(SharedList* list, int data) {
    sem_wait(&list->mutex);
    
    int new_offset = alloca_nodo(list);
    if (new_offset == -1) {
        sem_post(&list->mutex);
        return -1;  // Errore
    }
    
    Node* nuovo = &list->nodes[new_offset];
    nuovo->data = data;
    nuovo->next_offset = list->head_offset;
    
    list->head_offset = new_offset;
    list->count++;
    
    sem_post(&list->mutex);
    return new_offset;
}

// Rimuove primo elemento (thread-safe)
int rimuovi_primo_safe(SharedList* list) {
    sem_wait(&list->mutex);
    
    if (list->head_offset == -1) {
        sem_post(&list->mutex);
        return -1;  // Lista vuota
    }
    
    Node* head = get_node(list, list->head_offset);
    int data = head->data;
    int old_head = list->head_offset;
    
    list->head_offset = head->next_offset;
    list->count--;
    
    libera_nodo(list, old_head);
    
    sem_post(&list->mutex);
    return data;
}

// Conta elementi (thread-safe)
int conta_safe(SharedList* list) {
    sem_wait(&list->mutex);
    int count = list->count;
    sem_post(&list->mutex);
    return count;
}

// Stampa lista (thread-safe)
void stampa_lista_safe(SharedList* list, const char* nome_processo) {
    sem_wait(&list->mutex);
    
    printf("%s: Lista [", nome_processo);
    int current_offset = list->head_offset;
    
    while (current_offset != -1) {
        Node* current = get_node(list, current_offset);
        printf("%d", current->data);
        current_offset = current->next_offset;
        if (current_offset != -1) printf(" -> ");
    }
    
    printf("] (count: %d)\n", list->count);
    
    sem_post(&list->mutex);
}

// Inizializza lista condivisa
SharedList* crea_lista_condivisa() {
    // Rimuovi memoria precedente
    shm_unlink(SHM_NAME);
    
    // Crea memoria condivisa
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) return NULL;
    
    if (ftruncate(shm_fd, sizeof(SharedList)) == -1) {
        close(shm_fd);
        return NULL;
    }
    
    // Mappa memoria
    SharedList* list = mmap(NULL, sizeof(SharedList), 
                           PROT_READ | PROT_WRITE, 
                           MAP_SHARED, shm_fd, 0);
    if (list == MAP_FAILED) {
        close(shm_fd);
        return NULL;
    }
    
    // Inizializza
    list->head_offset = -1;
    list->count = 0;
    
    // Inizializza pool nodi
    for (int i = 0; i < MAX_NODES; i++) {
        list->nodes[i].is_free = 1;
        list->nodes[i].next_offset = -1;
    }
    
    // Inizializza semaforo
    sem_init(&list->mutex, 1, 1);
    
    close(shm_fd);
    return list;
}

// Apre lista condivisa esistente
SharedList* apri_lista_condivisa() {
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) return NULL;
    
    SharedList* list = mmap(NULL, sizeof(SharedList), 
                           PROT_READ | PROT_WRITE, 
                           MAP_SHARED, shm_fd, 0);
    close(shm_fd);
    
    if (list == MAP_FAILED) return NULL;
    return list;
}

// Chiude lista condivisa
void chiudi_lista_condivisa(SharedList* list, int rimuovi) {
    if (list) {
        if (rimuovi) {
            sem_destroy(&list->mutex);
            shm_unlink(SHM_NAME);
        }
        munmap(list, sizeof(SharedList));
    }
}

int main() {
    printf("=== LISTA CONCATENATA IN MEMORIA CONDIVISA ===\n");
    
    // Processo padre crea la lista
    SharedList* lista = crea_lista_condivisa();
    if (!lista) {
        printf("Errore creazione lista condivisa!\n");
        return 1;
    }
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // === PROCESSO FIGLIO ===
        printf("\n🟢 FIGLIO avviato\n");
        
        inserisci_inizio_safe(lista, 10);
        stampa_lista_safe(lista, "FIGLIO");
        
        inserisci_inizio_safe(lista, 20);
        stampa_lista_safe(lista, "FIGLIO");
        
        printf("🟢 FIGLIO: Elementi totali: %d\n", conta_safe(lista));
        
        chiudi_lista_condivisa(lista, 0);  // Non rimuovere
        exit(0);
        
    } else {
        // === PROCESSO PADRE ===
        printf("\n🟡 PADRE avviato\n");
        
        sleep(1);  // Lascia lavorare il figlio
        
        inserisci_inizio_safe(lista, 30);
        stampa_lista_safe(lista, "PADRE");
        
        int rimosso = rimuovi_primo_safe(lista);
        printf("🟡 PADRE: Rimosso elemento: %d\n", rimosso);
        stampa_lista_safe(lista, "PADRE");
        
        wait(NULL);  // Aspetta figlio
        
        printf("\n=== STATO FINALE ===\n");
        stampa_lista_safe(lista, "FINALE");
        
        chiudi_lista_condivisa(lista, 1);  // Rimuovi memoria
    }
    
    return 0;
}
```

### Lista Bidirezionale
```c
#include <stdio.h>
#include <stdlib.h>

// Nodo bidirezionale
typedef struct DNode {
    int data;
    struct DNode* next;
    struct DNode* prev;
} DNode;

// Struttura lista bidirezionale
typedef struct {
    DNode* head;
    DNode* tail;
    int count;
} DoublyLinkedList;

// Inizializza lista
DoublyLinkedList* crea_lista_bidirezionale() {
    DoublyLinkedList* lista = malloc(sizeof(DoublyLinkedList));
    if (lista) {
        lista->head = NULL;
        lista->tail = NULL;
        lista->count = 0;
    }
    return lista;
}

// Crea nuovo nodo
DNode* crea_nodo_bidir(int data) {
    DNode* nuovo = malloc(sizeof(DNode));
    if (nuovo) {
        nuovo->data = data;
        nuovo->next = NULL;
        nuovo->prev = NULL;
    }
    return nuovo;
}

// Inserisce all'inizio
void inserisci_inizio_bidir(DoublyLinkedList* lista, int data) {
    DNode* nuovo = crea_nodo_bidir(data);
    if (!nuovo || !lista) return;
    
    if (lista->head == NULL) {
        // Lista vuota
        lista->head = nuovo;
        lista->tail = nuovo;
    } else {
        nuovo->next = lista->head;
        lista->head->prev = nuovo;
        lista->head = nuovo;
    }
    
    lista->count++;
}

// Inserisce alla fine
void inserisci_fine_bidir(DoublyLinkedList* lista, int data) {
    DNode* nuovo = crea_nodo_bidir(data);
    if (!nuovo || !lista) return;
    
    if (lista->tail == NULL) {
        // Lista vuota
        lista->head = nuovo;
        lista->tail = nuovo;
    } else {
        nuovo->prev = lista->tail;
        lista->tail->next = nuovo;
        lista->tail = nuovo;
    }
    
    lista->count++;
}

// Rimuove nodo specifico
void rimuovi_nodo_bidir(DoublyLinkedList* lista, DNode* nodo) {
    if (!lista || !nodo) return;
    
    // Aggiorna puntatori
    if (nodo->prev) {
        nodo->prev->next = nodo->next;
    } else {
        lista->head = nodo->next;  // Era il primo
    }
    
    if (nodo->next) {
        nodo->next->prev = nodo->prev;
    } else {
        lista->tail = nodo->prev;  // Era l'ultimo
    }
    
    free(nodo);
    lista->count--;
}

// Stampa avanti
void stampa_avanti(DoublyLinkedList* lista) {
    printf("Avanti: ");
    DNode* current = lista->head;
    while (current) {
        printf("%d", current->data);
        if (current->next) printf(" <-> ");
        current = current->next;
    }
    printf(" | count: %d\n", lista->count);
}

// Stampa indietro
void stampa_indietro(DoublyLinkedList* lista) {
    printf("Indietro: ");
    DNode* current = lista->tail;
    while (current) {
        printf("%d", current->data);
        if (current->prev) printf(" <-> ");
        current = current->prev;
    }
    printf("\n");
}

// Libera lista bidirezionale
void libera_lista_bidir(DoublyLinkedList* lista) {
    if (!lista) return;
    
    DNode* current = lista->head;
    while (current) {
        DNode* next = current->next;
        free(current);
        current = next;
    }
    
    free(lista);
}

int main() {
    printf("=== LISTA BIDIREZIONALE ===\n");
    
    DoublyLinkedList* lista = crea_lista_bidirezionale();
    
    // Inserimenti
    inserisci_inizio_bidir(lista, 20);
    inserisci_inizio_bidir(lista, 10);
    inserisci_fine_bidir(lista, 30);
    inserisci_fine_bidir(lista, 40);
    
    stampa_avanti(lista);   // 10 <-> 20 <-> 30 <-> 40
    stampa_indietro(lista); // 40 <-> 30 <-> 20 <-> 10
    
    // Rimuovi elemento centrale
    DNode* current = lista->head->next;  // Secondo elemento (20)
    rimuovi_nodo_bidir(lista, current);
    
    stampa_avanti(lista);   // 10 <-> 30 <-> 40
    
    libera_lista_bidir(lista);
    return 0;
}
```

### Operazioni Avanzate su Liste
```c
#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int data;
    struct Node* next;
} Node;

// Inserimento ordinato
Node* inserisci_ordinato(Node* head, int data) {
    Node* nuovo = malloc(sizeof(Node));
    if (!nuovo) return head;
    
    nuovo->data = data;
    nuovo->next = NULL;
    
    // Se lista vuota o nuovo è il più piccolo
    if (head == NULL || head->data > data) {
        nuovo->next = head;
        return nuovo;
    }
    
    // Trova posizione corretta
    Node* current = head;
    while (current->next != NULL && current->next->data < data) {
        current = current->next;
    }
    
    nuovo->next = current->next;
    current->next = nuovo;
    
    return head;
}

// Rimuovi duplicati (lista ordinata)
Node* rimuovi_duplicati(Node* head) {
    if (head == NULL) return head;
    
    Node* current = head;
    while (current->next != NULL) {
        if (current->data == current->next->data) {
            Node* temp = current->next;
            current->next = current->next->next;
            free(temp);
        } else {
            current = current->next;
        }
    }
    
    return head;
}

// Inverti lista
Node* inverti_lista(Node* head) {
    Node* prev = NULL;
    Node* current = head;
    Node* next = NULL;
    
    while (current != NULL) {
        next = current->next;  // Salva prossimo
        current->next = prev;  // Inverti puntatore
        prev = current;        // Avanza prev
        current = next;        // Avanza current
    }
    
    return prev;  // Nuovo head
}

// Trova il middle (algoritmo two pointers)
Node* trova_middle(Node* head) {
    if (head == NULL) return NULL;
    
    Node* slow = head;
    Node* fast = head;
    
    while (fast->next != NULL && fast->next->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
    }
    
    return slow;
}

// Rileva ciclo (algoritmo Floyd)
int ha_ciclo(Node* head) {
    if (head == NULL) return 0;
    
    Node* slow = head;
    Node* fast = head;
    
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
        
        if (slow == fast) {
            return 1;  // Ciclo rilevato
        }
    }
    
    return 0;  // Nessun ciclo
}

// Merge di due liste ordinate
Node* merge_liste_ordinate(Node* l1, Node* l2) {
    Node dummy = {0, NULL};
    Node* tail = &dummy;
    
    while (l1 != NULL && l2 != NULL) {
        if (l1->data <= l2->data) {
            tail->next = l1;
            l1 = l1->next;
        } else {
            tail->next = l2;
            l2 = l2->next;
        }
        tail = tail->next;
    }
    
    // Aggiungi elementi rimanenti
    if (l1 != NULL) tail->next = l1;
    if (l2 != NULL) tail->next = l2;
    
    return dummy.next;
}

// Utility: stampa lista
void stampa_lista_util(Node* head) {
    while (head) {
        printf("%d ", head->data);
        head = head->next;
    }
    printf("\n");
}

// Utility: crea nodo
Node* crea_nodo_util(int data) {
    Node* nuovo = malloc(sizeof(Node));
    if (nuovo) {
        nuovo->data = data;
        nuovo->next = NULL;
    }
    return nuovo;
}

int main() {
    printf("=== OPERAZIONI AVANZATE SU LISTE ===\n");
    
    // Test inserimento ordinato
    Node* lista = NULL;
    lista = inserisci_ordinato(lista, 30);
    lista = inserisci_ordinato(lista, 10);
    lista = inserisci_ordinato(lista, 20);
    lista = inserisci_ordinato(lista, 40);
    lista = inserisci_ordinato(lista, 20);  // Duplicato
    
    printf("Lista ordinata: ");
    stampa_lista_util(lista);
    
    // Rimuovi duplicati
    lista = rimuovi_duplicati(lista);
    printf("Senza duplicati: ");
    stampa_lista_util(lista);
    
    // Trova middle
    Node* middle = trova_middle(lista);
    printf("Elemento centrale: %d\n", middle->data);
    
    // Inverti lista
    lista = inverti_lista(lista);
    printf("Lista invertita: ");
    stampa_lista_util(lista);
    
    // Test merge
    Node* lista2 = NULL;
    lista2 = inserisci_ordinato(lista2, 15);
    lista2 = inserisci_ordinato(lista2, 25);
    lista2 = inserisci_ordinato(lista2, 35);
    
    printf("Lista 2: ");
    stampa_lista_util(lista2);
    
    Node* merged = merge_liste_ordinate(lista, lista2);
    printf("Liste unite: ");
    stampa_lista_util(merged);
    
    return 0;
}
```

---

## PREPROCESSORE

### Macro e Define
```c
#include <stdio.h>

// Costanti
#define PI 3.14159
#define MAX_SIZE 1000
#define MESSAGGIO "Ciao Mondo"

// Macro funzioni
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define QUADRATO(x) ((x) * (x))
#define ABS(x) ((x) < 0 ? -(x) : (x))

// Macro multilinea
#define SWAP(a, b, tipo) do { \
    tipo temp = a; \
    a = b; \
    b = temp; \
} while(0)

// Compilazione condizionale
#define DEBUG 1

#ifdef DEBUG
    #define DBG_PRINT(fmt, ...) printf("DEBUG: " fmt, ##__VA_ARGS__)
#else
    #define DBG_PRINT(fmt, ...)
#endif

int main() {
    printf("PI = %.5f\n", PI);
    printf("Messaggio: %s\n", MESSAGGIO);
    
    int a = 10, b = 20;
    printf("MAX(%d, %d) = %d\n", a, b, MAX(a, b));
    printf("MIN(%d, %d) = %d\n", a, b, MIN(a, b));
    printf("QUADRATO(%d) = %d\n", a, QUADRATO(a));
    printf("ABS(-5) = %d\n", ABS(-5));
    
    DBG_PRINT("Valori: a=%d, b=%d\n", a, b);
    
    SWAP(a, b, int);
    printf("Dopo SWAP: a=%d, b=%d\n", a, b);
    
    return 0;
}
```

### Include e Header File
```c
// file: matematica.h
#ifndef MATEMATICA_H
#define MATEMATICA_H

#define PI 3.14159

int somma(int a, int b);
float area_cerchio(float raggio);
int fattoriale(int n);

#endif

// file: matematica.c
#include "matematica.h"

int somma(int a, int b) {
    return a + b;
}

float area_cerchio(float raggio) {
    return PI * raggio * raggio;
}

int fattoriale(int n) {
    if (n <= 1) return 1;
    return n * fattoriale(n - 1);
}

// file: main.c
#include <stdio.h>
#include "matematica.h"

int main() {
    printf("Somma: %d\n", somma(5, 3));
    printf("Area cerchio: %.2f\n", area_cerchio(5.0f));
    printf("Fattoriale: %d\n", fattoriale(5));
    
    return 0;
}
```

---

## ESEMPI PRATICI

### Calcolatrice Semplice
```c
#include <stdio.h>

int main() {
    float num1, num2, risultato;
    char operatore;
    
    printf("Inserisci calcolo (es: 5 + 3): ");
    scanf("%f %c %f", &num1, &operatore, &num2);
    
    switch (operatore) {
        case '+':
            risultato = num1 + num2;
            break;
        case '-':
            risultato = num1 - num2;
            break;
        case '*':
            risultato = num1 * num2;
            break;
        case '/':
            if (num2 != 0) {
                risultato = num1 / num2;
            } else {
                printf("Errore: divisione per zero!\n");
                return 1;
            }
            break;
        default:
            printf("Operatore non valido!\n");
            return 1;
    }
    
    printf("%.2f %c %.2f = %.2f\n", num1, operatore, num2, risultato);
    return 0;
}
```

### Gestione Array di Studenti
```c
#include <stdio.h>
#include <string.h>

#define MAX_STUDENTI 50

typedef struct {
    int id;
    char nome[50];
    char cognome[50];
    float voto;
} Studente;

int main() {
    Studente studenti[MAX_STUDENTI];
    int count = 0;
    int scelta;
    
    do {
        printf("\n--- GESTIONE STUDENTI ---\n");
        printf("1. Aggiungi studente\n");
        printf("2. Visualizza studenti\n");
        printf("3. Cerca studente\n");
        printf("4. Media voti\n");
        printf("0. Esci\n");
        printf("Scelta: ");
        scanf("%d", &scelta);
        
        switch (scelta) {
            case 1:
                if (count < MAX_STUDENTI) {
                    printf("ID: ");
                    scanf("%d", &studenti[count].id);
                    printf("Nome: ");
                    scanf("%49s", studenti[count].nome);
                    printf("Cognome: ");
                    scanf("%49s", studenti[count].cognome);
                    printf("Voto: ");
                    scanf("%f", &studenti[count].voto);
                    count++;
                    printf("Studente aggiunto!\n");
                } else {
                    printf("Massimo studenti raggiunto!\n");
                }
                break;
                
            case 2:
                printf("\n--- LISTA STUDENTI ---\n");
                for (int i = 0; i < count; i++) {
                    printf("ID: %d - %s %s - Voto: %.1f\n",
                           studenti[i].id, studenti[i].nome,
                           studenti[i].cognome, studenti[i].voto);
                }
                break;
                
            case 3: {
                char cerca_nome[50];
                printf("Nome da cercare: ");
                scanf("%49s", cerca_nome);
                
                int trovato = 0;
                for (int i = 0; i < count; i++) {
                    if (strcmp(studenti[i].nome, cerca_nome) == 0) {
                        printf("Trovato: ID %d - %s %s - Voto %.1f\n",
                               studenti[i].id, studenti[i].nome,
                               studenti[i].cognome, studenti[i].voto);
                        trovato = 1;
                    }
                }
                if (!trovato) {
                    printf("Studente non trovato!\n");
                }
                break;
            }
            
            case 4:
                if (count > 0) {
                    float somma = 0;
                    for (int i = 0; i < count; i++) {
                        somma += studenti[i].voto;
                    }
                    printf("Media voti: %.2f\n", somma / count);
                } else {
                    printf("Nessuno studente inserito!\n");
                }
                break;
        }
    } while (scelta != 0);
    
    return 0;
}
```

### Gioco Indovina Numero
```c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {
    srand(time(NULL));  // Inizializza random
    
    int numero_segreto = rand() % 100 + 1;  // 1-100
    int tentativo;
    int tentativi = 0;
    int max_tentativi = 7;
    
    printf("=== INDOVINA IL NUMERO ===\n");
    printf("Ho pensato un numero tra 1 e 100.\n");
    printf("Hai %d tentativi!\n\n", max_tentativi);
    
    do {
        tentativi++;
        printf("Tentativo %d/%d - Inserisci numero: ", tentativi, max_tentativi);
        scanf("%d", &tentativo);
        
        if (tentativo == numero_segreto) {
            printf("\n🎉 BRAVO! Hai indovinato in %d tentativi!\n", tentativi);
            break;
        } else if (tentativo < numero_segreto) {
            printf("Troppo basso! ");
        } else {
            printf("Troppo alto! ");
        }
        
        if (tentativi < max_tentativi) {
            printf("Riprova!\n");
        }
        
    } while (tentativi < max_tentativi);
    
    if (tentativo != numero_segreto) {
        printf("\n😞 Hai esaurito i tentativi! Il numero era %d\n", numero_segreto);
    }
    
    return 0;
}
```

---

## COMPILAZIONE E MAKEFILE

### Comandi Base GCC
```bash
# Compilazione semplice
gcc programma.c -o programma

# Con debug
gcc -g -Wall programma.c -o programma

# Con ottimizzazioni
gcc -O2 programma.c -o programma

# Linkare librerie
gcc programma.c -o programma -lm  # math library
gcc programma.c -o programma -lrt -lpthread  # POSIX IPC

# Compilazione multipli file
gcc main.c matematica.c -o programma

# Solo compilazione (no linking)
gcc -c matematica.c  # crea matematica.o
gcc main.c matematica.o -o programma
```

### Makefile Esempio
```makefile
# Variabili
CC = gcc
CFLAGS = -Wall -g
TARGET = programma
SOURCES = main.c matematica.c utils.c
OBJECTS = $(SOURCES:.c=.o)

# Target principale
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)

# Compilazione oggetti
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Pulizia
clean:
	rm -f $(OBJECTS) $(TARGET)

# Target phony
.PHONY: clean
```



## VALIDAZIONE E CONTROLLO VARIABILI

### Controllo Tipo di Dato e Range
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <float.h>

// Verifica se una stringa è un numero intero valido
int is_valid_integer(const char* str) {
    if (str == NULL || *str == '\0') return 0;
    
    int i = 0;
    // Gestisci segno
    if (str[0] == '+' || str[0] == '-') i = 1;
    
    // Deve avere almeno una cifra dopo il segno
    if (str[i] == '\0') return 0;
    
    // Controlla che tutti i caratteri siano cifre
    for (; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) return 0;
    }
    
    return 1;
}

// Verifica se una stringa è un numero float valido
int is_valid_float(const char* str) {
    if (str == NULL || *str == '\0') return 0;
    
    int i = 0;
    int has_dot = 0;
    
    // Gestisci segno
    if (str[0] == '+' || str[0] == '-') i = 1;
    
    for (; str[i] != '\0'; i++) {
        if (str[i] == '.') {
            if (has_dot) return 0;  // Più di un punto
            has_dot = 1;
        } else if (!isdigit(str[i])) {
            return 0;
        }
    }
    
    return 1;
}

// Input sicuro di intero con controllo range
int input_int_safe(const char* prompt, int min, int max) {
    char buffer[100];
    int numero;
    
    while (1) {
        printf("%s (%d-%d): ", prompt, min, max);
        
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            printf("Errore lettura input!\n");
            continue;
        }
        
        // Rimuovi newline
        buffer[strcspn(buffer, "\n")] = 0;
        
        // Verifica se è un numero valido
        if (!is_valid_integer(buffer)) {
            printf("❌ Errore: inserire un numero intero valido!\n");
            continue;
        }
        
        numero = atoi(buffer);
        
        // Controllo range
        if (numero < min || numero > max) {
            printf("❌ Errore: numero fuori range [%d-%d]!\n", min, max);
            continue;
        }
        
        return numero;
    }
}

// Input sicuro di float con controllo range
float input_float_safe(const char* prompt, float min, float max) {
    char buffer[100];
    float numero;
    
    while (1) {
        printf("%s (%.2f-%.2f): ", prompt, min, max);
        
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            printf("Errore lettura input!\n");
            continue;
        }
        
        buffer[strcspn(buffer, "\n")] = 0;
        
        if (!is_valid_float(buffer)) {
            printf("❌ Errore: inserire un numero decimale valido!\n");
            continue;
        }
        
        numero = atof(buffer);
        
        if (numero < min || numero > max) {
            printf("❌ Errore: numero fuori range [%.2f-%.2f]!\n", min, max);
            continue;
        }
        
        return numero;
    }
}

int main() {
    printf("=== CONTROLLO TIPO E RANGE ===\n");
    
    // Test controllo stringhe
    char* test_strings[] = {"123", "-456", "12.34", "abc", "12.34.56", "+789"};
    int num_tests = sizeof(test_strings) / sizeof(test_strings[0]);
    
    printf("\nTest validazione:\n");
    for (int i = 0; i < num_tests; i++) {
        printf("'%s' - Int: %s, Float: %s\n", 
               test_strings[i],
               is_valid_integer(test_strings[i]) ? "✅" : "❌",
               is_valid_float(test_strings[i]) ? "✅" : "❌");
    }
    
    // Input sicuro
    int eta = input_int_safe("Inserisci età", 0, 150);
    float altezza = input_float_safe("Inserisci altezza (m)", 0.5f, 3.0f);
    
    printf("\n✅ Dati validati: Età=%d, Altezza=%.2fm\n", eta, altezza);
    
    return 0;
}
```

### Validazione Stringhe e Caratteri
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Verifica se una stringa contiene solo lettere
int is_alpha_string(const char* str) {
    if (str == NULL || *str == '\0') return 0;
    
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isalpha(str[i])) return 0;
    }
    return 1;
}

// Verifica se una stringa contiene solo cifre
int is_digit_string(const char* str) {
    if (str == NULL || *str == '\0') return 0;
    
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) return 0;
    }
    return 1;
}

// Verifica se una stringa è alfanumerica
int is_alphanumeric_string(const char* str) {
    if (str == NULL || *str == '\0') return 0;
    
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isalnum(str[i])) return 0;
    }
    return 1;
}

// Verifica se una stringa è un nome valido (solo lettere e spazi)
int is_valid_name(const char* str) {
    if (str == NULL || *str == '\0') return 0;
    
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isalpha(str[i]) && str[i] != ' ') return 0;
    }
    return 1;
}

// Verifica se un carattere è un operatore matematico
int is_math_operator(char c) {
    return (c == '+' || c == '-' || c == '*' || c == '/' || c == '%');
}

// Verifica formato email base (contiene @ e .)
int is_valid_email_basic(const char* email) {
    if (email == NULL || strlen(email) < 5) return 0;
    
    int has_at = 0;
    int has_dot_after_at = 0;
    int at_position = -1;
    
    for (int i = 0; email[i] != '\0'; i++) {
        if (email[i] == '@') {
            if (has_at) return 0;  // Più di una @
            has_at = 1;
            at_position = i;
        } else if (email[i] == '.' && has_at && i > at_position) {
            has_dot_after_at = 1;
        }
    }
    
    return has_at && has_dot_after_at;
}

// Input stringa con validazione lunghezza e contenuto
void input_string_safe(const char* prompt, char* output, int max_len, 
                      int (*validator)(const char*), const char* error_msg) {
    char buffer[256];
    
    while (1) {
        printf("%s (max %d caratteri): ", prompt, max_len - 1);
        
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            printf("Errore lettura input!\n");
            continue;
        }
        
        // Rimuovi newline
        buffer[strcspn(buffer, "\n")] = 0;
        
        // Controllo lunghezza
        if (strlen(buffer) >= max_len) {
            printf("❌ Errore: stringa troppo lunga (max %d caratteri)!\n", max_len - 1);
            continue;
        }
        
        // Controllo validatore personalizzato
        if (validator != NULL && !validator(buffer)) {
            printf("❌ Errore: %s\n", error_msg);
            continue;
        }
        
        strcpy(output, buffer);
        break;
    }
}

int main() {
    printf("=== VALIDAZIONE STRINGHE ===\n");
    
    // Test validatori
    char* test_strings[] = {
        "Mario", "123", "Mario123", "mario.rossi@email.com", 
        "Test@", "invalidemail", "Nome Con Spazi"
    };
    
    printf("\nTest validazione stringhe:\n");
    for (int i = 0; i < 7; i++) {
        printf("'%s':\n", test_strings[i]);
        printf("  Solo lettere: %s\n", is_alpha_string(test_strings[i]) ? "✅" : "❌");
        printf("  Solo cifre: %s\n", is_digit_string(test_strings[i]) ? "✅" : "❌");
        printf("  Alfanumerico: %s\n", is_alphanumeric_string(test_strings[i]) ? "✅" : "❌");
        printf("  Nome valido: %s\n", is_valid_name(test_strings[i]) ? "✅" : "❌");
        printf("  Email valida: %s\n", is_valid_email_basic(test_strings[i]) ? "✅" : "❌");
        printf("\n");
    }
    
    // Input sicuro
    char nome[50];
    char email[100];
    
    input_string_safe("Inserisci nome", nome, sizeof(nome), 
                     is_valid_name, "il nome deve contenere solo lettere e spazi");
    
    input_string_safe("Inserisci email", email, sizeof(email), 
                     is_valid_email_basic, "formato email non valido");
    
    printf("\n✅ Dati validati:\n");
    printf("Nome: '%s'\n", nome);
    printf("Email: '%s'\n", email);
    
    return 0;
}
```

### Controllo Overflow e Underflow
```c
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <float.h>
#include <errno.h>
#include <string.h>

// Conversione sicura stringa -> intero con controllo overflow
int safe_string_to_int(const char* str, int* result) {
    if (str == NULL || result == NULL) return -1;
    
    errno = 0;
    char* endptr;
    long val = strtol(str, &endptr, 10);
    
    // Controllo errori di conversione
    if (errno == ERANGE) {
        printf("❌ Errore: numero fuori range per int!\n");
        return -1;
    }
    
    // Controlla che tutta la stringa sia stata convertita
    if (endptr == str || *endptr != '\0') {
        printf("❌ Errore: formato numero non valido!\n");
        return -1;
    }
    
    // Controllo range specifico per int
    if (val < INT_MIN || val > INT_MAX) {
        printf("❌ Errore: numero troppo grande per int!\n");
        return -1;
    }
    
    *result = (int)val;
    return 0;  // Successo
}

// Conversione sicura stringa -> float con controllo overflow
int safe_string_to_float(const char* str, float* result) {
    if (str == NULL || result == NULL) return -1;
    
    errno = 0;
    char* endptr;
    double val = strtod(str, &endptr);
    
    if (errno == ERANGE) {
        if (val == 0.0) {
            printf("❌ Errore: underflow - numero troppo piccolo!\n");
        } else {
            printf("❌ Errore: overflow - numero troppo grande!\n");
        }
        return -1;
    }
    
    if (endptr == str || *endptr != '\0') {
        printf("❌ Errore: formato float non valido!\n");
        return -1;
    }
    
    // Controllo range per float
    if (val > FLT_MAX || val < -FLT_MAX) {
        printf("❌ Errore: numero fuori range per float!\n");
        return -1;
    }
    
    *result = (float)val;
    return 0;
}

// Operazioni aritmetiche sicure con controllo overflow
int safe_add_int(int a, int b, int* result) {
    // Controllo overflow per addizione
    if (a > 0 && b > 0 && a > INT_MAX - b) {
        printf("❌ Overflow: %d + %d troppo grande!\n", a, b);
        return -1;
    }
    if (a < 0 && b < 0 && a < INT_MIN - b) {
        printf("❌ Underflow: %d + %d troppo piccolo!\n", a, b);
        return -1;
    }
    
    *result = a + b;
    return 0;
}

int safe_multiply_int(int a, int b, int* result) {
    // Controllo overflow per moltiplicazione
    if (a != 0 && b != 0) {
        if (a > 0) {
            if (b > 0 && a > INT_MAX / b) {
                printf("❌ Overflow: %d * %d troppo grande!\n", a, b);
                return -1;
            }
            if (b < 0 && b < INT_MIN / a) {
                printf("❌ Underflow: %d * %d troppo piccolo!\n", a, b);
                return -1;
            }
        } else {
            if (b > 0 && a < INT_MIN / b) {
                printf("❌ Underflow: %d * %d troppo piccolo!\n", a, b);
                return -1;
            }
            if (b < 0 && a > INT_MAX / b) {
                printf("❌ Overflow: %d * %d troppo grande!\n", a, b);
                return -1;
            }
        }
    }
    
    *result = a * b;
    return 0;
}

int main() {
    printf("=== CONTROLLO OVERFLOW/UNDERFLOW ===\n");
    
    // Test conversioni sicure
    char* test_numbers[] = {
        "123", "-456", "2147483647", "2147483648",  // INT_MAX + 1
        "99999999999999999999", "abc", "12.34", "1.7e308"
    };
    
    printf("\nTest conversioni sicure:\n");
    for (int i = 0; i < 8; i++) {
        printf("Stringa: '%s'\n", test_numbers[i]);
        
        int int_val;
        if (safe_string_to_int(test_numbers[i], &int_val) == 0) {
            printf("  ✅ Int: %d\n", int_val);
        } else {
            printf("  ❌ Conversione int fallita\n");
        }
        
        float float_val;
        if (safe_string_to_float(test_numbers[i], &float_val) == 0) {
            printf("  ✅ Float: %.2f\n", float_val);
        } else {
            printf("  ❌ Conversione float fallita\n");
        }
        printf("\n");
    }
    
    // Test operazioni sicure
    printf("Test operazioni sicure:\n");
    
    int result;
    
    // Test addizione
    if (safe_add_int(INT_MAX, 1, &result) == 0) {
        printf("✅ INT_MAX + 1 = %d\n", result);
    }
    
    if (safe_add_int(100, 200, &result) == 0) {
        printf("✅ 100 + 200 = %d\n", result);
    }
    
    // Test moltiplicazione
    if (safe_multiply_int(INT_MAX, 2, &result) == 0) {
        printf("✅ INT_MAX * 2 = %d\n", result);
    }
    
    if (safe_multiply_int(1000, 2000, &result) == 0) {
        printf("✅ 1000 * 2000 = %d\n", result);
    }
    
    return 0;
}
```

### Sistema di Validazione Unificato
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Enumerazione dei tipi di validazione
typedef enum {
    VALIDATE_INT,
    VALIDATE_FLOAT,
    VALIDATE_CHAR,
    VALIDATE_STRING,
    VALIDATE_EMAIL,
    VALIDATE_ALPHA,
    VALIDATE_NUMERIC,
    VALIDATE_ALPHANUMERIC
} ValidationType;

// Struttura per definire regole di validazione
typedef struct {
    ValidationType type;
    union {
        struct {
            int min;
            int max;
        } int_range;
        struct {
            float min;
            float max;
        } float_range;
        struct {
            int min_len;
            int max_len;
        } string_range;
    } rules;
    char error_message[100];
} ValidationRule;

// Validatore generico
int validate_input(const char* input, ValidationRule* rule) {
    if (input == NULL || rule == NULL) return 0;
    
    switch (rule->type) {
        case VALIDATE_INT: {
            for (int i = 0; input[i]; i++) {
                if (!isdigit(input[i]) && input[i] != '-' && input[i] != '+') {
                    strcpy(rule->error_message, "Deve essere un numero intero");
                    return 0;
                }
            }
            int val = atoi(input);
            if (val < rule->rules.int_range.min || val > rule->rules.int_range.max) {
                snprintf(rule->error_message, sizeof(rule->error_message),
                        "Deve essere tra %d e %d", 
                        rule->rules.int_range.min, rule->rules.int_range.max);
                return 0;
            }
            break;
        }
        
        case VALIDATE_FLOAT: {
            int has_dot = 0;
            for (int i = 0; input[i]; i++) {
                if (input[i] == '.') {
                    if (has_dot) {
                        strcpy(rule->error_message, "Più di un punto decimale");
                        return 0;
                    }
                    has_dot = 1;
                } else if (!isdigit(input[i]) && input[i] != '-' && input[i] != '+') {
                    strcpy(rule->error_message, "Deve essere un numero decimale");
                    return 0;
                }
            }
            float val = atof(input);
            if (val < rule->rules.float_range.min || val > rule->rules.float_range.max) {
                snprintf(rule->error_message, sizeof(rule->error_message),
                        "Deve essere tra %.2f e %.2f", 
                        rule->rules.float_range.min, rule->rules.float_range.max);
                return 0;
            }
            break;
        }
        
        case VALIDATE_ALPHA:
            for (int i = 0; input[i]; i++) {
                if (!isalpha(input[i]) && input[i] != ' ') {
                    strcpy(rule->error_message, "Deve contenere solo lettere e spazi");
                    return 0;
                }
            }
            break;
            
        case VALIDATE_NUMERIC:
            for (int i = 0; input[i]; i++) {
                if (!isdigit(input[i])) {
                    strcpy(rule->error_message, "Deve contenere solo cifre");
                    return 0;
                }
            }
            break;
            
        case VALIDATE_ALPHANUMERIC:
            for (int i = 0; input[i]; i++) {
                if (!isalnum(input[i])) {
                    strcpy(rule->error_message, "Deve contenere solo lettere e numeri");
                    return 0;
                }
            }
            break;
            
        case VALIDATE_EMAIL: {
            int has_at = 0, has_dot_after_at = 0, at_pos = -1;
            for (int i = 0; input[i]; i++) {
                if (input[i] == '@') {
                    if (has_at) {
                        strcpy(rule->error_message, "Email non può avere più di una @");
                        return 0;
                    }
                    has_at = 1;
                    at_pos = i;
                } else if (input[i] == '.' && has_at && i > at_pos) {
                    has_dot_after_at = 1;
                }
            }
            if (!has_at || !has_dot_after_at) {
                strcpy(rule->error_message, "Formato email non valido");
                return 0;
            }
            break;
        }
        
        case VALIDATE_STRING: {
            int len = strlen(input);
            if (len < rule->rules.string_range.min_len || len > rule->rules.string_range.max_len) {
                snprintf(rule->error_message, sizeof(rule->error_message),
                        "Lunghezza deve essere tra %d e %d caratteri", 
                        rule->rules.string_range.min_len, rule->rules.string_range.max_len);
                return 0;
            }
            break;
        }
    }
    
    return 1;  // Validazione passata
}

// Input generico con validazione
void input_with_validation(const char* prompt, char* output, int max_len, ValidationRule* rule) {
    char buffer[256];
    
    while (1) {
        printf("%s: ", prompt);
        
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            printf("Errore lettura input!\n");
            continue;
        }
        
        buffer[strcspn(buffer, "\n")] = 0;  // Rimuovi newline
        
        if (strlen(buffer) >= max_len) {
            printf("❌ Input troppo lungo (max %d caratteri)!\n", max_len - 1);
            continue;
        }
        
        if (validate_input(buffer, rule)) {
            strcpy(output, buffer);
            printf("✅ Input valido!\n");
            break;
        } else {
            printf("❌ %s\n", rule->error_message);
        }
    }
}

int main() {
    printf("=== SISTEMA VALIDAZIONE UNIFICATO ===\n");
    
    char input_buffer[100];
    
    // Validazione età (intero 0-150)
    ValidationRule age_rule = {
        .type = VALIDATE_INT,
        .rules.int_range = {0, 150}
    };
    input_with_validation("Inserisci età", input_buffer, sizeof(input_buffer), &age_rule);
    int eta = atoi(input_buffer);
    
    // Validazione altezza (float 0.5-3.0)
    ValidationRule height_rule = {
        .type = VALIDATE_FLOAT,
        .rules.float_range = {0.5f, 3.0f}
    };
    input_with_validation("Inserisci altezza (m)", input_buffer, sizeof(input_buffer), &height_rule);
    float altezza = atof(input_buffer);
    
    // Validazione nome (solo lettere)
    ValidationRule name_rule = {
        .type = VALIDATE_ALPHA
    };
    char nome[50];
    input_with_validation("Inserisci nome", nome, sizeof(nome), &name_rule);
    
    // Validazione email
    ValidationRule email_rule = {
        .type = VALIDATE_EMAIL
    };
    char email[100];
    input_with_validation("Inserisci email", email, sizeof(email), &email_rule);
    
    // Validazione codice (alfanumerico 5-10 caratteri)
    ValidationRule code_rule = {
        .type = VALIDATE_ALPHANUMERIC,
        .rules.string_range = {5, 10}
    };
    char codice[20];
    input_with_validation("Inserisci codice", codice, sizeof(codice), &code_rule);
    
    printf("\n=== DATI VALIDATI ===\n");
    printf("Età: %d anni\n", eta);
    printf("Altezza: %.2f m\n", altezza);
    printf("Nome: %s\n", nome);
    printf("Email: %s\n", email);
    printf("Codice: %s\n", codice);
    
    return 0;
}
```

### Macro per Validazione Rapida
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Macro per controlli rapidi
#define VALIDATE_RANGE(var, min, max, type) \
    do { \
        if ((var) < (min) || (var) > (max)) { \
            printf("❌ Errore: " #var " deve essere tra " #min " e " #max "\n"); \
            return -1; \
        } \
        printf("✅ " #var " = " type " valido\n", (var)); \
    } while(0)

#define VALIDATE_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            printf("❌ Errore: " #ptr " è NULL!\n"); \
            return -1; \
        } \
    } while(0)

#define VALIDATE_STRING_LENGTH(str, min_len, max_len) \
    do { \
        int len = strlen(str); \
        if (len < (min_len) || len > (max_len)) { \
            printf("❌ Errore: " #str " deve avere lunghezza tra %d e %d (attuale: %d)\n", \
                   (min_len), (max_len), len); \
            return -1; \
        } \
        printf("✅ " #str " = '%s' (lunghezza %d) valida\n", (str), len); \
    } while(0)

#define VALIDATE_POSITIVE(var) \
    do { \
        if ((var) <= 0) { \
            printf("❌ Errore: " #var " deve essere positivo!\n"); \
            return -1; \
        } \
    } while(0)

// Funzione di esempio che usa le macro di validazione
int process_user_data(const char* nome, int eta, float stipendio, char* codice_fiscale) {
    printf("=== VALIDAZIONE CON MACRO ===\n");
    
    // Validazioni con macro
    VALIDATE_NOT_NULL(nome);
    VALIDATE_NOT_NULL(codice_fiscale);
    
    VALIDATE_STRING_LENGTH(nome, 2, 50);
    VALIDATE_STRING_LENGTH(codice_fiscale, 16, 16);  // Codice fiscale italiano
    
    VALIDATE_RANGE(eta, 18, 100, "%d");
    VALIDATE_RANGE(stipendio, 500.0f, 100000.0f, "%.2f");
    
    VALIDATE_POSITIVE(stipendio);
    
    printf("\n✅ Tutti i dati sono validi!\n");
    return 0;
}

int main() {
    // Test con dati validi
    printf("Test 1 - Dati validi:\n");
    if (process_user_data("Mario Rossi", 30, 2500.50f, "RSSMRA80A01H501X") == 0) {
        printf("Elaborazione completata con successo!\n");
    }
    
    printf("\n" "="*50 "\n");
    
    // Test con dati non validi
    printf("Test 2 - Età non valida:\n");
    process_user_data("Luigi Verdi", 200, 3000.0f, "VRDLGU75B02H501Y");
    
    printf("\n" "="*50 "\n");
    
    printf("Test 3 - Nome troppo corto:\n");
    process_user_data("A", 25, 2000.0f, "BNCMRC85C03H501Z");
    
    printf("\n" "="*50 "\n");
    
    printf("Test 4 - Codice fiscale lunghezza sbagliata:\n");
    process_user_data("Anna Bianchi", 35, 2800.0f, "BNCANN85");
    
    return 0;
}
```

## CASTING E CONVERSIONI

### Casting Base tra Tipi Primitivi
```c
#include <stdio.h>

int main() {
    // Conversioni numeriche esplicite
    int intero = 42;
    float decimale = 3.14f;
    double preciso = 2.718281828;
    char carattere = 'A';
    
    // Int -> Float/Double
    float f1 = (float)intero;           // 42.0f
    double d1 = (double)intero;         // 42.0
    
    // Float/Double -> Int (troncamento)
    int i1 = (int)decimale;             // 3 (perde parte decimale)
    int i2 = (int)preciso;              // 2
    
    // Char -> Int (valore ASCII)
    int ascii = (int)carattere;         // 65
    
    // Int -> Char (modulo 256)
    char c1 = (char)65;                 // 'A'
    char c2 = (char)300;                // 44 (300 % 256)
    
    // Unsigned/Signed conversions
    int negativo = -1;
    unsigned int positivo = (unsigned int)negativo;  // 4294967295
    
    printf("Conversioni base:\n");
    printf("int %d -> float %.2f\n", intero, f1);
    printf("float %.2f -> int %d\n", decimale, i1);
    printf("char '%c' -> int %d\n", carattere, ascii);
    printf("int %d -> unsigned %u\n", negativo, positivo);
    
    return 0;
}
```

### Casting di Puntatori
```c
#include <stdio.h>
#include <stdlib.h>

int main() {
    // Puntatori a tipi diversi
    int numero = 0x41424344;           // "ABCD" in little-endian
    int* ptr_int = &numero;
    
    // Casting puntatori per accesso byte-level
    char* ptr_char = (char*)ptr_int;
    unsigned char* ptr_uchar = (unsigned char*)ptr_int;
    void* ptr_void = (void*)ptr_int;
    
    printf("Valore originale: 0x%08X\n", numero);
    printf("Bytes individuali:\n");
    for (int i = 0; i < sizeof(int); i++) {
        printf("  Byte %d: 0x%02X ('%c')\n", i, ptr_uchar[i], 
               (ptr_char[i] >= 32 && ptr_char[i] <= 126) ? ptr_char[i] : '.');
    }
    
    // Casting void* -> tipo specifico
    int* restored = (int*)ptr_void;
    printf("Restored value: 0x%08X\n", *restored);
    
    // Array casting
    float array_float[] = {1.0f, 2.0f, 3.0f, 4.0f};
    int* array_as_int = (int*)array_float;
    
    printf("\nFloat array come int:\n");
    for (int i = 0; i < 4; i++) {
        printf("float[%d] = %.1f -> int[%d] = 0x%08X\n", 
               i, array_float[i], i, array_as_int[i]);
    }
    
    return 0;
}
```

### Casting per Networking (Essenziale per TCP/UDP)
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main() {
    // === CASTING SOCKADDR ===
    struct sockaddr_in server_addr;
    struct sockaddr* generic_addr;
    
    // Inizializza sockaddr_in
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);        // Host to Network Short
    server_addr.sin_addr.s_addr = INADDR_ANY;
    
    // ⭐ CASTING FONDAMENTALE per bind/connect/accept
    generic_addr = (struct sockaddr*)&server_addr;
    
    printf("=== CASTING NETWORKING ===\n");
    
    // === CONVERSIONI BYTE ORDER ===
    uint16_t port_host = 8080;
    uint32_t ip_host = 0x7F000001;  // 127.0.0.1
    
    uint16_t port_network = htons(port_host);   // Host to Network Short
    uint32_t ip_network = htonl(ip_host);       // Host to Network Long
    
    printf("Porta - Host: %u, Network: %u\n", port_host, port_network);
    printf("IP - Host: 0x%08X, Network: 0x%08X\n", ip_host, ip_network);
    
    // Reverse conversion
    uint16_t port_back = ntohs(port_network);   // Network to Host Short
    uint32_t ip_back = ntohl(ip_network);       // Network to Host Long
    
    printf("Reverse - Porta: %u, IP: 0x%08X\n", port_back, ip_back);
    
    // === CONVERSIONI IP STRING ===
    char ip_string[] = "192.168.1.100";
    struct in_addr addr;
    
    // String -> Binary
    if (inet_aton(ip_string, &addr)) {
        printf("IP '%s' -> Binary: 0x%08X\n", ip_string, ntohl(addr.s_addr));
    }
    
    // Binary -> String
    char* ip_converted = inet_ntoa(addr);
    printf("Binary -> IP: '%s'\n", ip_converted);
    
    // === CASTING PER ACCEPT ===
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    // Simulazione accept (normalmente restituisce fd)
    printf("\nCasting per accept:\n");
    printf("accept(server_fd, (struct sockaddr*)&client_addr, &client_len);\n");
    
    // === BUFFER CASTING PER RECV/SEND ===
    int data_to_send = 0x12345678;
    char* buffer = (char*)&data_to_send;
    
    printf("Invio int come buffer: ");
    for (int i = 0; i < sizeof(int); i++) {
        printf("%02X ", (unsigned char)buffer[i]);
    }
    printf("\n");
    
    return 0;
}
```

### Casting per Stringhe e Buffer (TCP/UDP)
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int main() {
    printf("=== CASTING STRINGHE E BUFFER ===\n");
    
    // === RIMOZIONE NEWLINE (comune in server TCP) ===
    char buffer[256] = "Hello World\n";
    int len = strlen(buffer);
    
    printf("Prima: '%s' (len=%d)\n", buffer, len);
    
    // Metodo 1: strcspn (trova primo \n)
    buffer[strcspn(buffer, "\n")] = '\0';
    printf("Dopo strcspn: '%s'\n", buffer);
    
    // Metodo 2: per recv() con bytes_read
    char buffer2[256];
    int bytes_read = 12; // Simulazione recv()
    strcpy(buffer2, "Hello World\n");
    
    buffer2[bytes_read - 1] = '\0';  // Rimuovi ultimo char se \n
    printf("Con bytes_read: '%s'\n", buffer2);
    
    // Metodo 3: manuale
    char buffer3[] = "Test message\n\r";
    len = strlen(buffer3);
    while (len > 0 && (buffer3[len-1] == '\n' || buffer3[len-1] == '\r')) {
        buffer3[--len] = '\0';
    }
    printf("Pulizia manuale: '%s'\n", buffer3);
    
    // === CONVERSIONI STRINGA <-> NUMERO ===
    char num_str[] = "12345";
    char float_str[] = "3.14159";
    
    // String -> Number
    int numero = atoi(num_str);
    long lungo = atol(num_str);
    float decimale = atof(float_str);
    
    printf("\nConversioni string->number:\n");
    printf("'%s' -> int: %d\n", num_str, numero);
    printf("'%s' -> long: %ld\n", num_str, lungo);
    printf("'%s' -> float: %.3f\n", float_str, decimale);
    
    // Number -> String
    char result[100];
    sprintf(result, "%d", numero);
    printf("int %d -> string: '%s'\n", numero, result);
    
    snprintf(result, sizeof(result), "%.2f", decimale);
    printf("float %.3f -> string: '%s'\n", decimale, result);
    
    // === CASTING CHAR/UNSIGNED CHAR ===
    char signed_char = -1;
    unsigned char unsigned_char = (unsigned char)signed_char;
    
    printf("\nCasting char:\n");
    printf("signed char: %d, unsigned char: %u\n", signed_char, unsigned_char);
    
    // Utile per protocolli binari
    unsigned char protocol_byte = 0xFF;
    printf("Protocol byte: 0x%02X (%u)\n", protocol_byte, protocol_byte);
    
    return 0;
}
```

### Casting per Memoria e File Descriptor
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

int main() {
    printf("=== CASTING MEMORIA E FILE ===\n");
    
    // === CASTING MALLOC/MEMORY ===
    void* raw_memory = malloc(1024);
    if (raw_memory == NULL) {
        perror("malloc failed");
        return 1;
    }
    
    // Cast per diversi tipi di accesso
    char* char_ptr = (char*)raw_memory;
    int* int_ptr = (int*)raw_memory;
    float* float_ptr = (float*)raw_memory;
    
    // Scrittura come int
    *int_ptr = 0x41424344;
    
    // Lettura come char array
    printf("Memory as chars: ");
    for (int i = 0; i < 4; i++) {
        printf("'%c' ", char_ptr[i]);
    }
    printf("\n");
    
    // Lettura come float (interpretazione binaria)
    printf("Memory as float: %f\n", *float_ptr);
    
    free(raw_memory);
    
    // === CASTING FILE DESCRIPTOR ===
    int fd = open("/dev/null", O_WRONLY);
    if (fd != -1) {
        // File descriptor è un int, ma spesso serve casting per certe funzioni
        printf("File descriptor: %d\n", fd);
        
        // Simulazione uso in select()
        fd_set write_fds;
        FD_ZERO(&write_fds);
        FD_SET(fd, &write_fds);  // Automatico, ma fd è trattato come int
        
        close(fd);
    }
    
    // === CASTING PER MMAP ===
    size_t page_size = 4096;
    void* mapped = mmap(NULL, page_size, PROT_READ | PROT_WRITE, 
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (mapped != MAP_FAILED) {
        // Cast per uso specifico
        int* mapped_ints = (int*)mapped;
        char* mapped_chars = (char*)mapped;
        
        // Uso come array di int
        mapped_ints[0] = 100;
        mapped_ints[1] = 200;
        
        // Lettura come char
        printf("Mapped memory: %d %d\n", mapped_chars[0], mapped_chars[4]);
        
        munmap(mapped, page_size);
    }
    
    return 0;
}
```

### Casting per Struct e Union
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Esempio pratico: protocollo di rete
typedef struct {
    uint8_t type;       // Tipo messaggio
    uint8_t flags;      // Flag
    uint16_t length;    // Lunghezza payload
    uint32_t sequence;  // Numero sequenza
} PacketHeader;

typedef struct {
    PacketHeader header;
    char payload[256];
} Packet;

// Union per conversioni rapide
typedef union {
    uint32_t as_int;
    float as_float;
    char as_bytes[4];
} Converter;

int main() {
    printf("=== CASTING STRUCT E UNION ===\n");
    
    // === CASTING STRUCT <-> BUFFER ===
    Packet packet;
    packet.header.type = 1;
    packet.header.flags = 0x80;
    packet.header.length = htons(10);  // Network byte order
    packet.header.sequence = htonl(12345);
    strcpy(packet.payload, "Hello");
    
    // Cast struct -> buffer per invio
    char* send_buffer = (char*)&packet;
    int packet_size = sizeof(PacketHeader) + strlen(packet.payload);
    
    printf("Packet size: %d bytes\n", packet_size);
    printf("Header bytes: ");
    for (int i = 0; i < sizeof(PacketHeader); i++) {
        printf("%02X ", (unsigned char)send_buffer[i]);
    }
    printf("\n");
    
    // === RICEZIONE E CASTING INVERSO ===
    char received_buffer[sizeof(Packet)];
    memcpy(received_buffer, send_buffer, packet_size);
    
    // Cast buffer -> struct
    PacketHeader* received_header = (PacketHeader*)received_buffer;
    char* received_payload = received_buffer + sizeof(PacketHeader);
    
    printf("Received - Type: %u, Flags: 0x%02X\n", 
           received_header->type, received_header->flags);
    printf("Length: %u, Sequence: %u\n", 
           ntohs(received_header->length), ntohl(received_header->sequence));
    printf("Payload: '%s'\n", received_payload);
    
    // === UNION PER CONVERSIONI ===
    Converter conv;
    
    // Float -> bytes
    conv.as_float = 3.14159f;
    printf("\nFloat 3.14159 as bytes: ");
    for (int i = 0; i < 4; i++) {
        printf("0x%02X ", (unsigned char)conv.as_bytes[i]);
    }
    printf("\n");
    
    // Int -> float (interpretazione binaria)
    conv.as_int = 0x40490FDB;  // IEEE 754 per π
    printf("0x40490FDB as float: %f\n", conv.as_float);
    
    // === CASTING ARRAY DI STRUCT ===
    PacketHeader headers[3] = {
        {1, 0x01, htons(10), htonl(100)},
        {2, 0x02, htons(20), htonl(200)},
        {3, 0x03, htons(30), htonl(300)}
    };
    
    // Cast array -> buffer continuo
    char* headers_buffer = (char*)headers;
    int total_size = sizeof(headers);
    
    printf("Headers array as buffer (%d bytes):\n", total_size);
    for (int i = 0; i < total_size; i++) {
        if (i % 16 == 0) printf("\n%04X: ", i);
        printf("%02X ", (unsigned char)headers_buffer[i]);
    }
    printf("\n");
    
    return 0;
}
```

### Casting Avanzato per Sistemi
```c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

// Casting per manipolazione bit-level
typedef union {
    struct {
        unsigned bit0 : 1;
        unsigned bit1 : 1;
        unsigned bit2 : 1;
        unsigned bit3 : 1;
        unsigned bit4 : 1;
        unsigned bit5 : 1;
        unsigned bit6 : 1;
        unsigned bit7 : 1;
    } bits;
    uint8_t byte;
} BitField;

int main() {
    printf("=== CASTING AVANZATO ===\n");
    
    // === CASTING FUNCTION POINTERS ===
    int (*func_ptr)(int, int) = NULL;
    void* generic_func = NULL;
    
    // Simulazione casting function pointer
    printf("Function pointer size: %zu bytes\n", sizeof(func_ptr));
    
    // === CASTING PER BIT MANIPULATION ===
    BitField bf;
    bf.byte = 0b10101010;  // 170 in decimale
    
    printf("Byte 0x%02X as bits: ", bf.byte);
    printf("%d%d%d%d %d%d%d%d\n", 
           bf.bits.bit7, bf.bits.bit6, bf.bits.bit5, bf.bits.bit4,
           bf.bits.bit3, bf.bits.bit2, bf.bits.bit1, bf.bits.bit0);
    
    // === CASTING ARRAY MULTIDIMENSIONALE ===
    int matrix[3][4] = {
        {1, 2, 3, 4},
        {5, 6, 7, 8},
        {9, 10, 11, 12}
    };
    
    // Cast a array monodimensionale
    int* linear = (int*)matrix;
    printf("Matrix as linear array: ");
    for (int i = 0; i < 12; i++) {
        printf("%d ", linear[i]);
    }
    printf("\n");
    
    // === CASTING TIME_T ===
    time_t timestamp = time(NULL);
    long long timestamp_ll = (long long)timestamp;
    
    printf("Timestamp: %ld (as long long: %lld)\n", timestamp, timestamp_ll);
    
    // === CASTING PER PROTOCOLLI ===
    // Simulazione header TCP (semplificato)
    struct tcp_header {
        uint16_t source_port;
        uint16_t dest_port;
        uint32_t seq_num;
        uint32_t ack_num;
        uint8_t flags;
    } __attribute__((packed));  // Evita padding
    
    struct tcp_header tcp;
    tcp.source_port = htons(8080);
    tcp.dest_port = htons(80);
    tcp.seq_num = htonl(12345);
    tcp.ack_num = htonl(67890);
    tcp.flags = 0x18;  // PSH + ACK
    
    // Cast per invio raw
    unsigned char* raw_tcp = (unsigned char*)&tcp;
    printf("TCP header (%zu bytes): ", sizeof(tcp));
    for (size_t i = 0; i < sizeof(tcp); i++) {
        printf("%02X ", raw_tcp[i]);
    }
    printf("\n");
    
    // === CASTING VOLATILE (per hardware) ===
    volatile uint32_t* hardware_register = (volatile uint32_t*)0x40000000;
    // In un sistema reale, questo accede a un registro hardware
    printf("Hardware register cast: %p\n", (void*)hardware_register);
    
    return 0;
}
```

### Macro per Casting Comune
```c
#include <stdio.h>
#include <stdint.h>

// Macro per casting frequenti in networking
#define SOCKADDR_CAST(addr) ((struct sockaddr*)(addr))
#define SOCKADDR_IN_CAST(addr) ((struct sockaddr_in*)(addr))

// Macro per conversione byte order
#define HOST_TO_NET16(x) htons(x)
#define HOST_TO_NET32(x) htonl(x)
#define NET_TO_HOST16(x) ntohs(x)
#define NET_TO_HOST32(x) ntohl(x)

// Macro per casting buffer
#define BUFFER_TO_INT(buf) (*((int*)(buf)))
#define BUFFER_TO_FLOAT(buf) (*((float*)(buf)))
#define INT_TO_BUFFER(val) ((char*)&(val))

// Macro per pulizia stringhe
#define REMOVE_NEWLINE(str) do { \
    (str)[strcspn((str), "\n\r")] = '\0'; \
} while(0)

#define NULL_TERMINATE(buf, len) do { \
    (buf)[(len)] = '\0'; \
} while(0)

// Macro per casting sicuro
#define SAFE_CAST(type, value) ((type)(value))
#define PTR_CAST(type, ptr) ((type*)(ptr))
#define VOID_CAST(ptr) ((void*)(ptr))

int main() {
    printf("=== MACRO PER CASTING ===\n");
    
    // Esempio uso macro networking
    struct sockaddr_in addr;
    struct sockaddr* generic = SOCKADDR_CAST(&addr);
    printf("Sockaddr cast: %p -> %p\n", (void*)&addr, (void*)generic);
    
    // Esempio byte order
    uint16_t port = 8080;
    uint16_t net_port = HOST_TO_NET16(port);
    printf("Port %u -> network: %u\n", port, net_port);
    
    // Esempio buffer casting
    int value = 0x12345678;
    char* buffer = INT_TO_BUFFER(value);
    int restored = BUFFER_TO_INT(buffer);
    printf("Value: 0x%08X -> buffer -> restored: 0x%08X\n", value, restored);
    
    // Esempio pulizia stringa
    char test_str[] = "Hello World\n";
    printf("Before: '%s'\n", test_str);
    REMOVE_NEWLINE(test_str);
    printf("After: '%s'\n", test_str);
    
    return 0;
}
```

### Casting per Debugging e Ispezione
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Funzione per dump hex di qualsiasi tipo
void hex_dump(const void* data, size_t size, const char* label) {
    const unsigned char* bytes = (const unsigned char*)data;
    
    printf("%s (%zu bytes):\n", label, size);
    for (size_t i = 0; i < size; i++) {
        if (i % 16 == 0) printf("%04zX: ", i);
        printf("%02X ", bytes[i]);
        if ((i + 1) % 16 == 0 || i == size - 1) {
            // Aggiungi rappresentazione ASCII
            size_t start = i - (i % 16);
            printf(" |");
            for (size_t j = start; j <= i; j++) {
                char c = bytes[j];
                printf("%c", (c >= 32 && c <= 126) ? c : '.');
            }
            printf("|\n");
        }
    }
}

// Casting per ispezione strutture
typedef struct {
    int id;
    float value;
    char name[16];
} TestStruct;

int main() {
    printf("=== CASTING PER DEBUGGING ===\n");
    
    // Test con vari tipi
    int number = 0x12345678;
    float pi = 3.14159f;
    char text[] = "Hello";
    
    hex_dump(&number, sizeof(number), "Integer 0x12345678");
    hex_dump(&pi, sizeof(pi), "Float 3.14159");
    hex_dump(text, strlen(text), "String 'Hello'");
    
    // Test con struct
    TestStruct ts = {42, 2.71828f, "TestName"};
    hex_dump(&ts, sizeof(ts), "TestStruct");
    
    // Casting per accesso diretto ai campi
    char* struct_bytes = (char*)&ts;
    int* id_ptr = (int*)struct_bytes;
    float* value_ptr = (float*)(struct_bytes + sizeof(int));
    char* name_ptr = struct_bytes + sizeof(int) + sizeof(float);
    
    printf("\nDirect field access via casting:\n");
    printf("ID: %d (at offset 0)\n", *id_ptr);
    printf("Value: %.5f (at offset %zu)\n", *value_ptr, sizeof(int));
    printf("Name: '%.16s' (at offset %zu)\n", name_ptr, sizeof(int) + sizeof(float));
    
    // Verifica allineamento memoria
    printf("\nMemory alignment:\n");
    printf("Struct size: %zu bytes\n", sizeof(ts));
    printf("Fields sum: %zu bytes\n", sizeof(int) + sizeof(float) + 16);
    printf("Padding: %zu bytes\n", sizeof(ts) - (sizeof(int) + sizeof(float) + 16));
    
    return 0;
}
```