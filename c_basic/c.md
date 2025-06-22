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
11. [Preprocessore](#preprocessore)
12. [Esempi Pratici](#esempi-pratici)

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