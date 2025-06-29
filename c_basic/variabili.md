# Guida Completa - Variabili in C

## Indice
1. [Tipi Base](#tipi-base)
2. [Modificatori di Tipo](#modificatori-di-tipo)
3. [Tipi Derivati](#tipi-derivati)
4. [Storage Class](#storage-class)
5. [Qualifiers](#qualifiers)
6. [Casting e Conversioni](#casting-e-conversioni)
7. [Validazioni](#validazioni)
8. [Limiti e Costanti](#limiti-e-costanti)
9. [Esempi Pratici](#esempi-pratici)

---

## Tipi Base

### **Tipi Interi**

#### **char**
```c
#include <stdio.h>
#include <limits.h>

int main() {
    char c1 = 'A';              // Carattere
    char c2 = 65;               // Valore ASCII
    char c3 = -128;             // Valore minimo (implementazione-dependente)
    
    printf("char: %c (%d)\n", c1, c1);
    printf("Range: %d to %d\n", CHAR_MIN, CHAR_MAX);
    printf("Size: %zu bytes\n", sizeof(char));
    
    return 0;
}
```

#### **signed char**
```c
signed char sc = -128;          // Esplicitamente signed
printf("signed char range: %d to %d\n", SCHAR_MIN, SCHAR_MAX);
```

#### **unsigned char**
```c
unsigned char uc = 255;         // Solo valori positivi
printf("unsigned char range: 0 to %d\n", UCHAR_MAX);
```

#### **short (short int)**
```c
short s1 = 32767;
short int s2 = -32768;
unsigned short us = 65535;

printf("short range: %d to %d\n", SHRT_MIN, SHRT_MAX);
printf("unsigned short range: 0 to %u\n", USHRT_MAX);
printf("Size: %zu bytes\n", sizeof(short));
```

#### **int**
```c
int i1 = 2147483647;
unsigned int ui = 4294967295U;

printf("int range: %d to %d\n", INT_MIN, INT_MAX);
printf("unsigned int range: 0 to %u\n", UINT_MAX);
printf("Size: %zu bytes\n", sizeof(int));
```

#### **long (long int)**
```c
long l1 = 2147483647L;
long int l2 = -2147483648L;
unsigned long ul = 4294967295UL;

printf("long range: %ld to %ld\n", LONG_MIN, LONG_MAX);
printf("unsigned long range: 0 to %lu\n", ULONG_MAX);
printf("Size: %zu bytes\n", sizeof(long));
```

#### **long long (long long int) - C99**
```c
long long ll = 9223372036854775807LL;
unsigned long long ull = 18446744073709551615ULL;

printf("long long range: %lld to %lld\n", LLONG_MIN, LLONG_MAX);
printf("unsigned long long range: 0 to %llu\n", ULLONG_MAX);
printf("Size: %zu bytes\n", sizeof(long long));
```

### **Tipi Floating Point**

#### **float**
```c
#include <float.h>

float f1 = 3.14159f;
float f2 = 1.23e-4f;            // Notazione scientifica
float f3 = 0.0f / 0.0f;         // NaN
float f4 = 1.0f / 0.0f;         // Infinito

printf("float: %.6f\n", f1);
printf("Range: %e to %e\n", FLT_MIN, FLT_MAX);
printf("Precision: %d decimal digits\n", FLT_DIG);
printf("Size: %zu bytes\n", sizeof(float));
```

#### **double**
```c
double d1 = 3.141592653589793;
double d2 = 1.23e-100;
double d3 = DBL_MAX;

printf("double: %.15f\n", d1);
printf("Range: %e to %e\n", DBL_MIN, DBL_MAX);
printf("Precision: %d decimal digits\n", DBL_DIG);
printf("Size: %zu bytes\n", sizeof(double));
```

#### **long double**
```c
long double ld = 3.141592653589793238L;

printf("long double: %.18Lf\n", ld);
printf("Range: %Le to %Le\n", LDBL_MIN, LDBL_MAX);
printf("Precision: %d decimal digits\n", LDBL_DIG);
printf("Size: %zu bytes\n", sizeof(long double));
```

### **Tipo void**
```c
void func(void);                // Funzione senza parametri
void *ptr;                      // Puntatore generico
// void var;                    // ERRORE: non si può dichiarare variabile void
```

---

## Modificatori di Tipo

### **Tabella Completa Modificatori**

| **Tipo Base** | **Modificatori Possibili** | **Esempio** |
|---------------|----------------------------|-------------|
| `char` | `signed`, `unsigned` | `unsigned char` |
| `int` | `short`, `long`, `long long`, `signed`, `unsigned` | `unsigned long int` |
| `float` | Nessuno | `float` |
| `double` | `long` | `long double` |

### **Esempi Combinazioni**
```c
// Tutte le combinazioni valide per int
signed short int ssi;           // signed short int
unsigned short int usi;         // unsigned short int
signed int si;                  // signed int (default)
unsigned int ui;                // unsigned int
signed long int sli;            // signed long int
unsigned long int uli;          // unsigned long int
signed long long int slli;      // signed long long int
unsigned long long int ulli;    // unsigned long long int

// Abbreviazioni equivalenti
short ss = ssi;                 // equivale a signed short int
unsigned short us = usi;        // equivale a unsigned short int
long sl = sli;                  // equivale a signed long int
unsigned long ul = uli;         // equivale a unsigned long int
long long sll = slli;           // equivale a signed long long int
unsigned long long ull = ulli;  // equivale a unsigned long long int
```

---

## Tipi Derivati

### **Array**

#### **Array Monodimensionali**
```c
// Dichiarazioni diverse
int arr1[10];                   // Array di 10 int
int arr2[] = {1, 2, 3, 4, 5};   // Size dedotto: 5
int arr3[5] = {1, 2};           // Resto inizializzato a 0
char str1[] = "Hello";          // Array di char con '\0'
char str2[10] = "Hi";           // Resto inizializzato a '\0'

// Informazioni array
printf("Size of arr1: %zu bytes\n", sizeof(arr1));
printf("Elements in arr1: %zu\n", sizeof(arr1) / sizeof(arr1[0]));
printf("Size of str1: %zu\n", sizeof(str1));  // Include '\0'
```

#### **Array Multidimensionali**
```c
// Matrice 2D
int matrix[3][4];               // 3 righe, 4 colonne
int mat2[][3] = {{1,2,3}, {4,5,6}, {7,8,9}};

// Matrice 3D
int cube[2][3][4];

// Inizializzazione
int grid[2][3] = {
    {1, 2, 3},
    {4, 5, 6}
};

// Accesso elementi
matrix[1][2] = 42;
cube[0][1][2] = 100;
```

#### **Array di Stringhe**
```c
// Array di puntatori a char
char *colors[] = {"red", "green", "blue", NULL};

// Array 2D di char
char names[][10] = {"Alice", "Bob", "Charlie"};

// Esempio uso
for (int i = 0; colors[i] != NULL; i++) {
    printf("Color %d: %s\n", i, colors[i]);
}
```

### **Puntatori**

#### **Puntatori Base**
```c
int x = 42;
int *ptr = &x;                  // Puntatore a int
int **ptr_to_ptr = &ptr;        // Puntatore a puntatore

printf("x = %d\n", x);
printf("*ptr = %d\n", *ptr);
printf("**ptr_to_ptr = %d\n", **ptr_to_ptr);
printf("Address of x: %p\n", (void*)&x);
printf("Value of ptr: %p\n", (void*)ptr);
```

#### **Puntatori a Funzioni**
```c
// Dichiarazione funzioni
int add(int a, int b) { return a + b; }
int subtract(int a, int b) { return a - b; }

// Puntatori a funzioni
int (*operation)(int, int);
int (*ops[])(int, int) = {add, subtract};

// Uso
operation = add;
printf("5 + 3 = %d\n", operation(5, 3));
printf("5 - 3 = %d\n", ops[1](5, 3));
```

#### **Puntatori void**
```c
void *generic_ptr;
int i = 42;
float f = 3.14f;

generic_ptr = &i;
printf("int value: %d\n", *(int*)generic_ptr);

generic_ptr = &f;
printf("float value: %.2f\n", *(float*)generic_ptr);
```

### **Strutture (struct)**

#### **Definizione e Uso**
```c
// Definizione struttura
struct Person {
    char name[50];
    int age;
    float height;
};

// Typedef per semplificare
typedef struct {
    int x, y;
} Point;

// Dichiarazione e inizializzazione
struct Person p1 = {"Alice", 25, 165.5};
Point origin = {0, 0};
Point p2 = {.x = 10, .y = 20};  // Designated initializers (C99)

// Accesso membri
printf("Name: %s, Age: %d\n", p1.name, p1.age);
printf("Point: (%d, %d)\n", p2.x, p2.y);
```

#### **Strutture con Puntatori**
```c
struct Person *ptr = &p1;
ptr->age = 26;                  // Equivale a (*ptr).age = 26
printf("New age: %d\n", ptr->age);
```

#### **Strutture Annidate**
```c
struct Address {
    char street[100];
    char city[50];
    int zip;
};

struct Employee {
    struct Person info;
    struct Address address;
    double salary;
};

struct Employee emp = {
    {"Bob", 30, 180.0},
    {"123 Main St", "NYC", 10001},
    50000.0
};

printf("Employee: %s, City: %s\n", emp.info.name, emp.address.city);
```

### **Union**

#### **Definizione e Uso**
```c
union Data {
    int i;
    float f;
    char str[20];
};

union Data data;

data.i = 42;
printf("data.i = %d\n", data.i);

data.f = 3.14f;
printf("data.f = %.2f\n", data.f);
printf("data.i = %d (corrupted)\n", data.i);  // Valore corrotto

printf("Size of union: %zu\n", sizeof(union Data));  // Size del membro più grande
```

#### **Union con Tag**
```c
typedef enum { TYPE_INT, TYPE_FLOAT, TYPE_STRING } DataType;

typedef struct {
    DataType type;
    union {
        int i;
        float f;
        char str[20];
    } value;
} TaggedData;

TaggedData td;
td.type = TYPE_FLOAT;
td.value.f = 3.14f;

switch (td.type) {
    case TYPE_INT: printf("Int: %d\n", td.value.i); break;
    case TYPE_FLOAT: printf("Float: %.2f\n", td.value.f); break;
    case TYPE_STRING: printf("String: %s\n", td.value.str); break;
}
```

### **Enumerazioni (enum)**

#### **Enum Base**
```c
enum Color { RED, GREEN, BLUE };        // RED=0, GREEN=1, BLUE=2
enum Status { READY=1, RUNNING=5, STOPPED=10 };

enum Color c = RED;
enum Status s = RUNNING;

printf("Color: %d, Status: %d\n", c, s);
```

#### **Typedef Enum**
```c
typedef enum {
    MONDAY, TUESDAY, WEDNESDAY, THURSDAY, FRIDAY, SATURDAY, SUNDAY
} Day;

Day today = FRIDAY;
printf("Today is day %d\n", today);
```

---

## Storage Class

### **auto (default)**
```c
void func() {
    auto int x = 10;            // Equivale a: int x = 10;
    // x esiste solo in questa funzione
}
```

### **static**

#### **Static Locale**
```c
void counter() {
    static int count = 0;       // Inizializzata solo la prima volta
    count++;
    printf("Called %d times\n", count);
}

int main() {
    counter();  // Called 1 times
    counter();  // Called 2 times
    counter();  // Called 3 times
    return 0;
}
```

#### **Static Globale**
```c
// file1.c
static int private_var = 42;   // Visibile solo in questo file

// file2.c
// extern int private_var;     // ERRORE: non può accedere
```

### **extern**
```c
// file1.c
int global_var = 100;

// file2.c
extern int global_var;         // Dichiarazione (non definizione)

int main() {
    printf("Global: %d\n", global_var);
    return 0;
}
```

### **register**
```c
void loop_function() {
    register int i;             // Suggerimento per usare registro CPU
    for (i = 0; i < 1000; i++) {
        // Operazioni intensive
    }
    // Non si può ottenere l'indirizzo di i: &i è illegale
}
```

---

## Qualifiers

### **const**

#### **Variabili Costanti**
```c
const int max_size = 100;
// max_size = 200;              // ERRORE: non modificabile

const int arr[] = {1, 2, 3};
// arr[0] = 5;                  // ERRORE: array costante
```

#### **Puntatori Const**
```c
int x = 10, y = 20;

// Puntatore a costante
const int *ptr1 = &x;
// *ptr1 = 30;                  // ERRORE: non può modificare valore puntato
ptr1 = &y;                      // OK: può cambiare indirizzo

// Puntatore costante
int *const ptr2 = &x;
*ptr2 = 30;                     // OK: può modificare valore puntato
// ptr2 = &y;                   // ERRORE: non può cambiare indirizzo

// Puntatore costante a costante
const int *const ptr3 = &x;
// *ptr3 = 30;                  // ERRORE: non può modificare valore
// ptr3 = &y;                   // ERRORE: non può cambiare indirizzo
```

### **volatile**
```c
volatile int hardware_register;    // Valore può cambiare esternamente
volatile sig_atomic_t flag = 0;    // Per signal handlers

void signal_handler(int sig) {
    flag = 1;                       // Modifica da signal handler
}

int main() {
    while (!flag) {                 // Compiler non ottimizza questo loop
        // Aspetta segnale
    }
    return 0;
}
```

### **restrict (C99)**
```c
void copy_array(int *restrict dest, const int *restrict src, size_t n) {
    // dest e src non si sovrappongono - permette ottimizzazioni
    for (size_t i = 0; i < n; i++) {
        dest[i] = src[i];
    }
}
```

---

## Casting e Conversioni

### **Conversioni Implicite**

#### **Promozione Interi**
```c
char c = 100;
short s = 200;
int result = c + s;             // c e s promossi a int

printf("char + short = int: %d\n", result);
```

#### **Conversioni Aritmetiche**
```c
int i = 5;
float f = 2.5f;
double d = i + f;               // i -> float, risultato -> double

printf("int + float = double: %.2f\n", d);
```

### **Conversioni Esplicite (Cast)**

#### **Cast Base**
```c
double d = 3.14159;
int i = (int)d;                 // Troncamento: i = 3
float f = (float)d;             // Perdita precisione possibile

printf("double %.5f -> int %d\n", d, i);
```

#### **Cast Puntatori**
```c
int x = 42;
void *vptr = &x;                // Conversione implicita
int *iptr = (int*)vptr;         // Cast esplicito

// Cast tra tipi diversi (pericoloso)
char *cptr = (char*)&x;
printf("int as bytes: ");
for (size_t i = 0; i < sizeof(int); i++) {
    printf("%02x ", (unsigned char)cptr[i]);
}
printf("\n");
```

#### **Cast Function Pointer**
```c
int add_int(int a, int b) { return a + b; }
double add_double(double a, double b) { return a + b; }

// Cast function pointer (pericoloso se firme diverse)
double (*fp)(double, double) = (double(*)(double, double))add_int;
```

### **Conversioni Sicure**

#### **Controllo Overflow**
```c
#include <limits.h>

int safe_int_multiply(int a, int b, int *result) {
    if (a > 0 && b > 0 && a > INT_MAX / b) return -1; // Overflow
    if (a < 0 && b < 0 && a < INT_MAX / b) return -1; // Overflow
    if (a > 0 && b < 0 && b < INT_MIN / a) return -1; // Underflow
    if (a < 0 && b > 0 && a < INT_MIN / b) return -1; // Underflow
    
    *result = a * b;
    return 0; // Success
}
```

#### **Conversione Stringa Safe**
```c
#include <errno.h>

int safe_str_to_long(const char *str, long *result) {
    char *endptr;
    errno = 0;
    
    *result = strtol(str, &endptr, 10);
    
    if (errno == ERANGE) return -1;        // Overflow/underflow
    if (endptr == str) return -2;          // Nessuna conversione
    if (*endptr != '\0') return -3;        // Caratteri extra
    
    return 0; // Success
}
```

---

## Validazioni

### **Validazione Input Numerici**

#### **Validazione Interi**
```c
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>

typedef enum {
    VALID_INT,
    INVALID_FORMAT,
    OUT_OF_RANGE,
    EMPTY_INPUT
} IntValidationResult;

IntValidationResult validate_int(const char *input, int *result) {
    if (!input || *input == '\0') return EMPTY_INPUT;
    
    char *endptr;
    errno = 0;
    long val = strtol(input, &endptr, 10);
    
    if (endptr == input) return INVALID_FORMAT;
    if (*endptr != '\0') return INVALID_FORMAT;
    if (errno == ERANGE || val < INT_MIN || val > INT_MAX) return OUT_OF_RANGE;
    
    *result = (int)val;
    return VALID_INT;
}

// Esempio uso
int main() {
    char input[100];
    int number;
    
    printf("Inserisci un numero: ");
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = '\0'; // Rimuovi newline
    
    switch (validate_int(input, &number)) {
        case VALID_INT:
            printf("Numero valido: %d\n", number);
            break;
        case INVALID_FORMAT:
            printf("Formato non valido\n");
            break;
        case OUT_OF_RANGE:
            printf("Numero fuori range\n");
            break;
        case EMPTY_INPUT:
            printf("Input vuoto\n");
            break;
    }
    
    return 0;
}
```

#### **Validazione Float/Double**
```c
#include <math.h>

typedef enum {
    VALID_FLOAT,
    INVALID_FORMAT,
    OUT_OF_RANGE,
    IS_NAN,
    IS_INFINITY
} FloatValidationResult;

FloatValidationResult validate_float(const char *input, float *result) {
    if (!input || *input == '\0') return INVALID_FORMAT;
    
    char *endptr;
    errno = 0;
    double val = strtod(input, &endptr);
    
    if (endptr == input) return INVALID_FORMAT;
    if (*endptr != '\0') return INVALID_FORMAT;
    if (errno == ERANGE) return OUT_OF_RANGE;
    if (isnan(val)) return IS_NAN;
    if (isinf(val)) return IS_INFINITY;
    if (val < -FLT_MAX || val > FLT_MAX) return OUT_OF_RANGE;
    
    *result = (float)val;
    return VALID_FLOAT;
}
```

### **Validazione Stringhe**

#### **Validazione Lunghezza e Caratteri**
```c
#include <ctype.h>

typedef enum {
    VALID_STRING,
    TOO_SHORT,
    TOO_LONG,
    INVALID_CHARS,
    NULL_STRING
} StringValidationResult;

StringValidationResult validate_string(const char *str, size_t min_len, size_t max_len) {
    if (!str) return NULL_STRING;
    
    size_t len = strlen(str);
    if (len < min_len) return TOO_SHORT;
    if (len > max_len) return TOO_LONG;
    
    // Controlla caratteri validi (solo lettere e spazi)
    for (size_t i = 0; i < len; i++) {
        if (!isalpha(str[i]) && !isspace(str[i])) {
            return INVALID_CHARS;
        }
    }
    
    return VALID_STRING;
}

// Validazione email semplice
int is_valid_email(const char *email) {
    if (!email) return 0;
    
    const char *at = strchr(email, '@');
    if (!at) return 0;                      // Nessuna @
    if (at == email) return 0;              // @ all'inizio
    if (strchr(at + 1, '@')) return 0;      // Più di una @
    if (!strchr(at + 1, '.')) return 0;     // Nessun . dopo @
    
    return 1;
}
```

### **Validazione Array e Puntatori**

#### **Controllo Bounds Array**
```c
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

int safe_array_access(int *arr, size_t arr_size, size_t index, int *value) {
    if (!arr) return -1;                    // Puntatore NULL
    if (index >= arr_size) return -2;       // Index out of bounds
    
    *value = arr[index];
    return 0;
}

// Macro per accesso sicuro
#define SAFE_ARRAY_GET(arr, index, value) \
    safe_array_access(arr, ARRAY_SIZE(arr), index, value)
```

#### **Validazione Puntatori**
```c
int is_valid_pointer(const void *ptr, size_t expected_alignment) {
    if (!ptr) return 0;                     // NULL pointer
    
    // Controlla allineamento
    uintptr_t addr = (uintptr_t)ptr;
    if (addr % expected_alignment != 0) return 0;
    
    return 1;
}

// Esempio uso
int *int_ptr = malloc(sizeof(int));
if (is_valid_pointer(int_ptr, sizeof(int))) {
    *int_ptr = 42;
} else {
    printf("Puntatore non valido\n");
}
```

---

## Limiti e Costanti

### **Include Necessari**
```c
#include <limits.h>     // Limiti tipi interi
#include <float.h>      // Limiti tipi floating point
#include <stdint.h>     // Tipi fixed-width
#include <stdbool.h>    // Tipo bool (C99)
```

### **Limiti Tipi Interi**
```c
void print_integer_limits() {
    printf("=== LIMITI TIPI INTERI ===\n");
    printf("CHAR_BIT: %d bits per char\n", CHAR_BIT);
    
    printf("char: %d to %d\n", CHAR_MIN, CHAR_MAX);
    printf("signed char: %d to %d\n", SCHAR_MIN, SCHAR_MAX);
    printf("unsigned char: 0 to %u\n", UCHAR_MAX);
    
    printf("short: %d to %d\n", SHRT_MIN, SHRT_MAX);
    printf("unsigned short: 0 to %u\n", USHRT_MAX);
    
    printf("int: %d to %d\n", INT_MIN, INT_MAX);
    printf("unsigned int: 0 to %u\n", UINT_MAX);
    
    printf("long: %ld to %ld\n", LONG_MIN, LONG_MAX);
    printf("unsigned long: 0 to %lu\n", ULONG_MAX);
    
    printf("long long: %lld to %lld\n", LLONG_MIN, LLONG_MAX);
    printf("unsigned long long: 0 to %llu\n", ULLONG_MAX);
}
```

### **Limiti Floating Point**
```c
void print_float_limits() {
    printf("=== LIMITI FLOATING POINT ===\n");
    
    printf("FLT_RADIX: %d\n", FLT_RADIX);
    printf("FLT_ROUNDS: %d\n", FLT_ROUNDS);
    
    printf("float:\n");
    printf("  Range: %e to %e\n", FLT_MIN, FLT_MAX);
    printf("  Precision: %d digits\n", FLT_DIG);
    printf("  Epsilon: %e\n", FLT_EPSILON);
    
    printf("double:\n");
    printf("  Range: %e to %e\n", DBL_MIN, DBL_MAX);
    printf("  Precision: %d digits\n", DBL_DIG);
    printf("  Epsilon: %e\n", DBL_EPSILON);
    
    printf("long double:\n");
    printf("  Range: %Le to %Le\n", LDBL_MIN, LDBL_MAX);
    printf("  Precision: %d digits\n", LDBL_DIG);
    printf("  Epsilon: %Le\n", LDBL_EPSILON);
}
```

### **Tipi Fixed-Width (C99)**
```c
#include <stdint.h>
#include <inttypes.h>

void print_fixed_width_types() {
    printf("=== TIPI FIXED-WIDTH ===\n");
    
    int8_t i8 = INT8_MAX;
    uint8_t u8 = UINT8_MAX;
    int16_t i16 = INT16_MAX;
    uint16_t u16 = UINT16_MAX;
    int32_t i32 = INT32_MAX;
    uint32_t u32 = UINT32_MAX;
    int64_t i64 = INT64_MAX;
    uint64_t u64 = UINT64_MAX;
    
    printf("int8_t: %" PRId8 " (size: %zu)\n", i8, sizeof(i8));
    printf("uint8_t: %" PRIu8 " (size: %zu)\n", u8, sizeof(u8));
    printf("int16_t: %" PRId16 " (size: %zu)\n", i16, sizeof(i16));
    printf("uint16_t: %" PRIu16 " (size: %zu)\n", u16, sizeof(u16));
    printf("int32_t: %" PRId32 " (size: %zu)\n", i32, sizeof(i32));
    printf("uint32_t: %" PRIu32 " (size: %zu)\n", u32, sizeof(u32));
    printf("int64_t: %" PRId64 " (size: %zu)\n", i64, sizeof(i64));
    printf("uint64_t: %" PRIu64 " (size: %zu)\n", u64, sizeof(u64));
    
    // Tipi per puntatori
    intptr_t iptr = (intptr_t)&i8;
    uintptr_t uptr = (uintptr_t)&u8;
    printf("intptr_t: %" PRIdPTR "\n", iptr);
    printf("uintptr_t: %" PRIuPTR "\n", uptr);
}
```

### **Tipo Bool (C99)**
```c
#include <stdbool.h>

void bool_examples() {
    bool flag = true;
    bool result = false;
    
    printf("true: %d\n", true);     // 1
    printf("false: %d\n", false);   // 0
    printf("flag: %s\n", flag ? "true" : "false");
    
    // Conversioni
    int x = 5;
    bool is_positive = (x > 0);     // true
    printf("5 > 0: %s\n", is_positive ? "true" : "false");
}
```

---

## Esempi Pratici

### **Sistema di Tipo Dinamico**
```c
typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_BOOL
} ValueType;

typedef struct {
    ValueType type;
    union {
        int i;
        float f;
        char *s;
        bool b;
    } data;
} DynamicValue;

DynamicValue create_int(int value) {
    DynamicValue dv = {TYPE_INT, .data.i = value};
    return dv;
}

DynamicValue create_float(float value) {
    DynamicValue dv = {TYPE_FLOAT, .data.f = value};
    return dv;
}

DynamicValue create_string(const char *value) {
    DynamicValue dv = {TYPE_STRING};
    dv.data.s = malloc(strlen(value) + 1);
    strcpy(dv.data.s, value);
    return dv;
}

void print_dynamic_value(const DynamicValue *dv) {
    switch (dv->type) {
        case TYPE_INT:
            printf("int: %d\n", dv->data.i);
            break;
        case TYPE_FLOAT:
            printf("float: %.2f\n", dv->data.f);
            break;
        case TYPE_STRING:
            printf("string: %s\n", dv->data.s);
            break;
        case TYPE_BOOL:
            printf("bool: %s\n", dv->data.b ? "true" : "false");
            break;
    }
}

void free_dynamic_value(DynamicValue *dv) {
    if (dv->type == TYPE_STRING && dv->data.s) {
        free(dv->data.s);
        dv->data.s = NULL;
    }
}
```

### **Array Dinamico Generico**
```c
typedef struct {
    void *data;
    size_t element_size;
    size_t count;
    size_t capacity;
} DynamicArray;

DynamicArray* da_create(size_t element_size) {
    DynamicArray *da = malloc(sizeof(DynamicArray));
    da->data = malloc(element_size * 4);
    da->element_size = element_size;
    da->count = 0;
    da->capacity = 4;
    return da;
}

void da_push(DynamicArray *da, const void *element) {
    if (da->count >= da->capacity) {
        da->capacity *= 2;
        da->data = realloc(da->data, da->element_size * da->capacity);
    }
    
    char *byte_data = (char*)da->data;
    memcpy(byte_data + (da->count * da->element_size), element, da->element_size);
    da->count++;
}

void* da_get(DynamicArray *da, size_t index) {
    if (index >= da->count) return NULL;
    
    char *byte_data = (char*)da->data;
    return byte_data + (index * da->element_size);
}

void da_free(DynamicArray *da) {
    free(da->data);
    free(da);
}

// Esempio uso
int main() {
    DynamicArray *int_array = da_create(sizeof(int));
    
    for (int i = 0; i < 10; i++) {
        da_push(int_array, &i);
    }
    
    for (size_t i = 0; i < int_array->count; i++) {
        int *value = (int*)da_get(int_array, i);
        printf("Element %zu: %d\n", i, *value);
    }
    
    da_free(int_array);
    return 0;
}
```

### **Validatore Universale**
```c
typedef enum {
    VALIDATOR_SUCCESS,
    VALIDATOR_ERROR,
    VALIDATOR_OUT_OF_RANGE,
    VALIDATOR_INVALID_FORMAT
} ValidatorResult;

typedef struct {
    ValidatorResult (*validate)(const char *input, void *output);
    size_t output_size;
    const char *type_name;
} Validator;

ValidatorResult validate_int_wrapper(const char *input, void *output) {
    int *result = (int*)output;
    
    char *endptr;
    long val = strtol(input, &endptr, 10);
    
    if (endptr == input || *endptr != '\0') return VALIDATOR_INVALID_FORMAT;
    if (val < INT_MIN || val > INT_MAX) return VALIDATOR_OUT_OF_RANGE;
    
    *result = (int)val;
    return VALIDATOR_SUCCESS;
}

ValidatorResult validate_float_wrapper(const char *input, void *output) {
    float *result = (float*)output;
    
    char *endptr;
    double val = strtod(input, &endptr);
    
    if (endptr == input || *endptr != '\0') return VALIDATOR_INVALID_FORMAT;
    if (val < -FLT_MAX || val > FLT_MAX) return VALIDATOR_OUT_OF_RANGE;
    
    *result = (float)val;
    return VALIDATOR_SUCCESS;
}

// Tabella validatori
Validator validators[] = {
    {validate_int_wrapper, sizeof(int), "int"},
    {validate_float_wrapper, sizeof(float), "float"},
};

ValidatorResult universal_validate(const char *input, const char *type, void *output) {
    for (size_t i = 0; i < sizeof(validators) / sizeof(validators[0]); i++) {
        if (strcmp(validators[i].type_name, type) == 0) {
            return validators[i].validate(input, output);
        }
    }
    return VALIDATOR_ERROR;
}
```

---

## Checklist Variabili C

### Dichiarazione e Inizializzazione
- [ ] Scegli il tipo appropriato per i dati
- [ ] Inizializza sempre le variabili
- [ ] Usa const per valori immutabili
- [ ] Considera l'allineamento per struct

### Validazione Input
- [ ] Controlla sempre i valori di ritorno di strtol/strtod
- [ ] Valida range per evitare overflow
- [ ] Gestisci input malformati
- [ ] Usa errno per rilevare errori

### Casting
- [ ] Evita cast non necessari
- [ ] Controlla perdita di precisione nei cast
- [ ] Usa cast espliciti per chiarezza
- [ ] Attenzione ai cast tra signed/unsigned

### Memory Management
- [ ] Libera sempre la memoria allocata
- [ ] Controlla i valori di ritorno di malloc
- [ ] Usa sizeof() invece di costanti hardcoded
- [ ] Inizializza puntatori a NULL

---

## Compilazione e Test

```bash
# Compila con tutti i warning
gcc -Wall -Wextra -Werror -std=c99 -o test test.c

# Debug con informazioni complete
gcc -g -O0 -DDEBUG -o test_debug test.c

# Test con different standards
gcc -std=c89 -o test_c89 test.c
gcc -std=c99 -o test_c99 test.c
gcc -std=c11 -o test_c11 test.c

# Static analysis
gcc -fanalyzer -o test test.c

# Memory check
valgrind --leak-check=full ./test

# Test su diverse architetture
gcc -m32 -o test_32bit test.c  # 32-bit
gcc -m64 -o test_64bit test.c  # 64-bit
```

Questa guida copre **TUTTI** i tipi di variabili in C con esempi pratici, validazioni e best practices per un uso sicuro e corretto!