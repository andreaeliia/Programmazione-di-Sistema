# Guida Completa - Confronto Variabili in C

## Indice
1. [Tipi Base](#tipi-base)
2. [Stringhe](#stringhe)
3. [Array](#array)
4. [Strutture](#strutture)
5. [Puntatori](#puntatori)
6. [Tipi Avanzati](#tipi-avanzati)
7. [Confronti Generici](#confronti-generici)
8. [Funzioni di Utilità](#funzioni-di-utilità)

---

## Tipi Base

### Interi (int, long, short, char)

```c
#include <stdio.h>

// Confronto interi semplice
int compare_int(int a, int b) {
    if (a == b) return 0;       // Uguali
    if (a < b) return -1;       // a minore di b
    return 1;                   // a maggiore di b
}

// Confronto con tolleranza per valori grandi
int compare_long_safe(long a, long b) {
    if (a == b) return 0;
    return (a < b) ? -1 : 1;
}

// Esempio uso
void test_int_comparison() {
    int x = 10, y = 20;
    
    // Metodo diretto
    if (x == y) {
        printf("x e y sono uguali\n");
    } else if (x < y) {
        printf("x è minore di y\n");
    } else {
        printf("x è maggiore di y\n");
    }
    
    // Metodo con funzione
    int result = compare_int(x, y);
    switch (result) {
        case -1: printf("x < y\n"); break;
        case 0:  printf("x == y\n"); break;
        case 1:  printf("x > y\n"); break;
    }
}
```

### Numeri in Virgola Mobile (float, double)

```c
#include <math.h>
#include <float.h>

// ATTENZIONE: Mai confrontare float/double con ==
// Usa sempre tolleranza (epsilon)

#define FLOAT_EPSILON 1e-6
#define DOUBLE_EPSILON 1e-15

// Confronto float con tolleranza
int compare_float(float a, float b) {
    float diff = fabsf(a - b);
    
    if (diff < FLOAT_EPSILON) return 0;    // Uguali
    return (a < b) ? -1 : 1;
}

// Confronto double con tolleranza
int compare_double(double a, double b) {
    double diff = fabs(a - b);
    
    if (diff < DOUBLE_EPSILON) return 0;   // Uguali
    return (a < b) ? -1 : 1;
}

// Confronto con tolleranza relativa (più accurato)
int compare_double_relative(double a, double b, double tolerance) {
    if (a == b) return 0;  // Gestisce 0.0 e infinito
    
    double diff = fabs(a - b);
    double largest = fmax(fabs(a), fabs(b));
    
    if (diff <= tolerance * largest) return 0;
    return (a < b) ? -1 : 1;
}

// Esempio uso
void test_float_comparison() {
    float f1 = 0.1f + 0.2f;
    float f2 = 0.3f;
    
    // SBAGLIATO - può fallire!
    // if (f1 == f2) printf("Uguali\n");
    
    // CORRETTO
    if (compare_float(f1, f2) == 0) {
        printf("f1 e f2 sono uguali (con tolleranza)\n");
    }
    
    double d1 = 1.0/3.0;
    double d2 = 0.333333333333333;
    
    int result = compare_double_relative(d1, d2, 1e-10);
    printf("Confronto double: %d\n", result);
}
```

### Caratteri

```c
#include <ctype.h>

// Confronto caratteri semplice
int compare_char(char a, char b) {
    if (a == b) return 0;
    return (a < b) ? -1 : 1;
}

// Confronto case-insensitive
int compare_char_ignore_case(char a, char b) {
    char lower_a = tolower(a);
    char lower_b = tolower(b);
    
    if (lower_a == lower_b) return 0;
    return (lower_a < lower_b) ? -1 : 1;
}

// Esempio uso
void test_char_comparison() {
    char c1 = 'A';
    char c2 = 'a';
    
    printf("Confronto normale: %d\n", compare_char(c1, c2));
    printf("Confronto ignore case: %d\n", compare_char_ignore_case(c1, c2));
}
```

---

## Stringhe

### Confronto Stringhe Standard

```c
#include <string.h>

// Confronto stringhe esatto
int compare_strings(const char *str1, const char *str2) {
    if (str1 == NULL && str2 == NULL) return 0;
    if (str1 == NULL) return -1;
    if (str2 == NULL) return 1;
    
    return strcmp(str1, str2);
}

// Confronto case-insensitive
int compare_strings_ignore_case(const char *str1, const char *str2) {
    if (str1 == NULL && str2 == NULL) return 0;
    if (str1 == NULL) return -1;
    if (str2 == NULL) return 1;
    
    return strcasecmp(str1, str2);  // Linux/Unix
    // return _stricmp(str1, str2);  // Windows
}

// Confronto primi n caratteri
int compare_strings_n(const char *str1, const char *str2, size_t n) {
    if (str1 == NULL && str2 == NULL) return 0;
    if (str1 == NULL) return -1;
    if (str2 == NULL) return 1;
    
    return strncmp(str1, str2, n);
}

// Controlla se stringa contiene sottostringa
int string_contains(const char *haystack, const char *needle) {
    if (!haystack || !needle) return 0;
    return (strstr(haystack, needle) != NULL);
}

// Esempio uso
void test_string_comparison() {
    char *str1 = "Hello";
    char *str2 = "World";
    char *str3 = "hello";
    
    printf("strcmp(Hello, World): %d\n", compare_strings(str1, str2));
    printf("strcmp(Hello, hello): %d\n", compare_strings(str1, str3));
    printf("strcasecmp(Hello, hello): %d\n", compare_strings_ignore_case(str1, str3));
    
    if (string_contains("Hello World", "World")) {
        printf("'Hello World' contiene 'World'\n");
    }
}
```

### Confronto Stringhe Avanzato

```c
// Confronto con wildcards semplici (* e ?)
int string_match_wildcard(const char *str, const char *pattern) {
    if (*pattern == '\0') return (*str == '\0');
    
    if (*pattern == '*') {
        // * matcha zero o più caratteri
        while (*str) {
            if (string_match_wildcard(str, pattern + 1)) return 1;
            str++;
        }
        return string_match_wildcard(str, pattern + 1);
    }
    
    if (*str == '\0') return 0;
    
    if (*pattern == '?' || *pattern == *str) {
        return string_match_wildcard(str + 1, pattern + 1);
    }
    
    return 0;
}

// Distanza di Levenshtein (edit distance)
int string_edit_distance(const char *str1, const char *str2) {
    int len1 = strlen(str1);
    int len2 = strlen(str2);
    
    // Matrice per programmazione dinamica
    int **matrix = malloc((len1 + 1) * sizeof(int*));
    for (int i = 0; i <= len1; i++) {
        matrix[i] = malloc((len2 + 1) * sizeof(int));
    }
    
    // Inizializzazione
    for (int i = 0; i <= len1; i++) matrix[i][0] = i;
    for (int j = 0; j <= len2; j++) matrix[0][j] = j;
    
    // Calcolo distanza
    for (int i = 1; i <= len1; i++) {
        for (int j = 1; j <= len2; j++) {
            int cost = (str1[i-1] == str2[j-1]) ? 0 : 1;
            
            int delete_cost = matrix[i-1][j] + 1;
            int insert_cost = matrix[i][j-1] + 1;
            int replace_cost = matrix[i-1][j-1] + cost;
            
            matrix[i][j] = fmin(fmin(delete_cost, insert_cost), replace_cost);
        }
    }
    
    int result = matrix[len1][len2];
    
    // Cleanup
    for (int i = 0; i <= len1; i++) free(matrix[i]);
    free(matrix);
    
    return result;
}
```

---

## Array

### Array di Interi

```c
// Confronto array di interi
int compare_int_arrays(const int *arr1, const int *arr2, size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (arr1[i] != arr2[i]) {
            return (arr1[i] < arr2[i]) ? -1 : 1;
        }
    }
    return 0;  // Uguali
}

// Controllo se array è ordinato
int is_array_sorted(const int *arr, size_t size) {
    for (size_t i = 1; i < size; i++) {
        if (arr[i] < arr[i-1]) return 0;  // Non ordinato
    }
    return 1;  // Ordinato
}

// Trova differenze tra array
typedef struct {
    size_t index;
    int old_value;
    int new_value;
} ArrayDifference;

int find_array_differences(const int *arr1, const int *arr2, size_t size, 
                          ArrayDifference *diffs, size_t max_diffs) {
    int diff_count = 0;
    
    for (size_t i = 0; i < size && diff_count < max_diffs; i++) {
        if (arr1[i] != arr2[i]) {
            diffs[diff_count].index = i;
            diffs[diff_count].old_value = arr1[i];
            diffs[diff_count].new_value = arr2[i];
            diff_count++;
        }
    }
    
    return diff_count;
}

// Esempio uso
void test_array_comparison() {
    int arr1[] = {1, 2, 3, 4, 5};
    int arr2[] = {1, 2, 9, 4, 5};
    size_t size = sizeof(arr1) / sizeof(arr1[0]);
    
    int result = compare_int_arrays(arr1, arr2, size);
    printf("Confronto array: %d\n", result);
    
    ArrayDifference diffs[10];
    int diff_count = find_array_differences(arr1, arr2, size, diffs, 10);
    
    printf("Trovate %d differenze:\n", diff_count);
    for (int i = 0; i < diff_count; i++) {
        printf("  Indice %zu: %d -> %d\n", 
               diffs[i].index, diffs[i].old_value, diffs[i].new_value);
    }
}
```

### Array Generici

```c
// Confronto array generico con funzione di confronto custom
typedef int (*compare_func_t)(const void *a, const void *b);

int compare_arrays_generic(const void *arr1, const void *arr2, 
                          size_t element_count, size_t element_size,
                          compare_func_t compare_func) {
    const char *ptr1 = (const char*)arr1;
    const char *ptr2 = (const char*)arr2;
    
    for (size_t i = 0; i < element_count; i++) {
        int result = compare_func(ptr1, ptr2);
        if (result != 0) return result;
        
        ptr1 += element_size;
        ptr2 += element_size;
    }
    
    return 0;
}

// Funzioni di confronto per diversi tipi
int compare_ints(const void *a, const void *b) {
    int ia = *(const int*)a;
    int ib = *(const int*)b;
    return (ia > ib) - (ia < ib);
}

int compare_doubles(const void *a, const void *b) {
    double da = *(const double*)a;
    double db = *(const double*)b;
    
    if (fabs(da - db) < 1e-15) return 0;
    return (da > db) ? 1 : -1;
}

// Esempio uso array generico
void test_generic_array_comparison() {
    int arr1[] = {1, 2, 3};
    int arr2[] = {1, 2, 4};
    
    int result = compare_arrays_generic(arr1, arr2, 3, sizeof(int), compare_ints);
    printf("Confronto array generico: %d\n", result);
}
```

---

## Strutture

### Strutture Semplici

```c
typedef struct {
    int id;
    char name[50];
    double salary;
} Employee;

// Confronto strutture - campo per campo
int compare_employees(const Employee *emp1, const Employee *emp2) {
    // Confronta ID prima
    if (emp1->id != emp2->id) {
        return (emp1->id < emp2->id) ? -1 : 1;
    }
    
    // Confronta nome
    int name_cmp = strcmp(emp1->name, emp2->name);
    if (name_cmp != 0) return name_cmp;
    
    // Confronta salary con tolleranza
    if (fabs(emp1->salary - emp2->salary) > 0.01) {
        return (emp1->salary < emp2->salary) ? -1 : 1;
    }
    
    return 0;  // Uguali
}

// Confronto solo per ID (key comparison)
int compare_employees_by_id(const Employee *emp1, const Employee *emp2) {
    if (emp1->id == emp2->id) return 0;
    return (emp1->id < emp2->id) ? -1 : 1;
}

// Confronto memcmp (attenzione al padding!)
int compare_employees_binary(const Employee *emp1, const Employee *emp2) {
    return memcmp(emp1, emp2, sizeof(Employee));
}
```

### Strutture Complesse

```c
typedef struct {
    char *dynamic_string;
    int *dynamic_array;
    size_t array_size;
} ComplexStruct;

// Confronto struttura con puntatori
int compare_complex_structs(const ComplexStruct *s1, const ComplexStruct *s2) {
    // Confronta stringhe dinamiche
    if (s1->dynamic_string == NULL && s2->dynamic_string == NULL) {
        // Entrambe NULL - continua
    } else if (s1->dynamic_string == NULL || s2->dynamic_string == NULL) {
        return (s1->dynamic_string == NULL) ? -1 : 1;
    } else {
        int str_cmp = strcmp(s1->dynamic_string, s2->dynamic_string);
        if (str_cmp != 0) return str_cmp;
    }
    
    // Confronta dimensioni array
    if (s1->array_size != s2->array_size) {
        return (s1->array_size < s2->array_size) ? -1 : 1;
    }
    
    // Confronta contenuto array
    if (s1->array_size > 0) {
        if (s1->dynamic_array == NULL || s2->dynamic_array == NULL) {
            return (s1->dynamic_array == NULL) ? -1 : 1;
        }
        
        return compare_int_arrays(s1->dynamic_array, s2->dynamic_array, s1->array_size);
    }
    
    return 0;
}

// Copia profonda per confronti
ComplexStruct* deep_copy_complex_struct(const ComplexStruct *src) {
    ComplexStruct *copy = malloc(sizeof(ComplexStruct));
    
    // Copia stringa
    if (src->dynamic_string) {
        copy->dynamic_string = malloc(strlen(src->dynamic_string) + 1);
        strcpy(copy->dynamic_string, src->dynamic_string);
    } else {
        copy->dynamic_string = NULL;
    }
    
    // Copia array
    copy->array_size = src->array_size;
    if (src->array_size > 0 && src->dynamic_array) {
        copy->dynamic_array = malloc(src->array_size * sizeof(int));
        memcpy(copy->dynamic_array, src->dynamic_array, src->array_size * sizeof(int));
    } else {
        copy->dynamic_array = NULL;
    }
    
    return copy;
}
```

---

## Puntatori

### Confronto Puntatori Base

```c
// Confronto indirizzi di puntatori
int compare_pointer_addresses(const void *ptr1, const void *ptr2) {
    if (ptr1 == ptr2) return 0;
    return (ptr1 < ptr2) ? -1 : 1;
}

// Confronto valori puntati
int compare_pointed_ints(const int *ptr1, const int *ptr2) {
    if (ptr1 == NULL && ptr2 == NULL) return 0;
    if (ptr1 == NULL) return -1;
    if (ptr2 == NULL) return 1;
    
    return compare_int(*ptr1, *ptr2);
}

// Controllo validità puntatori
int pointers_are_valid(const void *ptr1, const void *ptr2) {
    return (ptr1 != NULL && ptr2 != NULL);
}

// Esempio uso
void test_pointer_comparison() {
    int x = 10, y = 20;
    int *ptr1 = &x;
    int *ptr2 = &y;
    int *ptr3 = &x;
    
    printf("Confronto indirizzi ptr1 vs ptr2: %d\n", 
           compare_pointer_addresses(ptr1, ptr2));
    printf("Confronto indirizzi ptr1 vs ptr3: %d\n", 
           compare_pointer_addresses(ptr1, ptr3));
    printf("Confronto valori puntati: %d\n", 
           compare_pointed_ints(ptr1, ptr2));
}
```

### Puntatori a Funzioni

```c
typedef int (*operation_func_t)(int a, int b);

int add(int a, int b) { return a + b; }
int multiply(int a, int b) { return a * b; }

// Confronto puntatori a funzioni
int compare_function_pointers(operation_func_t func1, operation_func_t func2) {
    if (func1 == func2) return 0;
    return (func1 < func2) ? -1 : 1;
}

// Confronto risultati di funzioni
int compare_function_results(operation_func_t func1, operation_func_t func2, 
                           int arg1, int arg2) {
    if (func1 == NULL || func2 == NULL) {
        if (func1 == func2) return 0;
        return (func1 == NULL) ? -1 : 1;
    }
    
    int result1 = func1(arg1, arg2);
    int result2 = func2(arg1, arg2);
    
    return compare_int(result1, result2);
}

void test_function_pointer_comparison() {
    operation_func_t op1 = add;
    operation_func_t op2 = multiply;
    operation_func_t op3 = add;
    
    printf("Confronto puntatori funzione add vs multiply: %d\n", 
           compare_function_pointers(op1, op2));
    printf("Confronto puntatori funzione add vs add: %d\n", 
           compare_function_pointers(op1, op3));
    
    printf("Confronto risultati add(2,3) vs multiply(2,3): %d\n",
           compare_function_results(op1, op2, 2, 3));
}
```

---

## Tipi Avanzati

### Union

```c
typedef union {
    int as_int;
    float as_float;
    char as_bytes[4];
} DataUnion;

// Confronto union richiede conoscenza del tipo attivo
typedef enum { TYPE_INT, TYPE_FLOAT, TYPE_BYTES } UnionType;

typedef struct {
    DataUnion data;
    UnionType active_type;
} TypedUnion;

int compare_typed_unions(const TypedUnion *u1, const TypedUnion *u2) {
    if (u1->active_type != u2->active_type) {
        return (u1->active_type < u2->active_type) ? -1 : 1;
    }
    
    switch (u1->active_type) {
        case TYPE_INT:
            return compare_int(u1->data.as_int, u2->data.as_int);
        case TYPE_FLOAT:
            return compare_float(u1->data.as_float, u2->data.as_float);
        case TYPE_BYTES:
            return memcmp(u1->data.as_bytes, u2->data.as_bytes, 4);
    }
    
    return 0;
}
```

### Bit Fields

```c
typedef struct {
    unsigned int flag1 : 1;
    unsigned int flag2 : 1;
    unsigned int value : 6;
} BitFieldStruct;

// Confronto bit field
int compare_bit_fields(const BitFieldStruct *bf1, const BitFieldStruct *bf2) {
    // Confronta flag1
    if (bf1->flag1 != bf2->flag1) {
        return (bf1->flag1 < bf2->flag1) ? -1 : 1;
    }
    
    // Confronta flag2
    if (bf1->flag2 != bf2->flag2) {
        return (bf1->flag2 < bf2->flag2) ? -1 : 1;
    }
    
    // Confronta value
    if (bf1->value != bf2->value) {
        return (bf1->value < bf2->value) ? -1 : 1;
    }
    
    return 0;
}

// Confronto binario di bit field (attenzione al padding!)
int compare_bit_fields_binary(const BitFieldStruct *bf1, const BitFieldStruct *bf2) {
    return memcmp(bf1, bf2, sizeof(BitFieldStruct));
}
```

### Variabili Volatile e Atomic

```c
#include <stdatomic.h>

// Confronto variabili volatile
int compare_volatile_ints(const volatile int *v1, const volatile int *v2) {
    // Lettura atomica dei valori
    int val1 = *v1;
    int val2 = *v2;
    
    return compare_int(val1, val2);
}

// Confronto variabili atomic
int compare_atomic_ints(const atomic_int *a1, const atomic_int *a2) {
    int val1 = atomic_load(a1);
    int val2 = atomic_load(a2);
    
    return compare_int(val1, val2);
}
```

---

## Confronti Generici

### Sistema di Confronto Universale

```c
// Tipo enumerazione per tutti i tipi supportati
typedef enum {
    COMPARE_INT,
    COMPARE_FLOAT,
    COMPARE_DOUBLE, 
    COMPARE_STRING,
    COMPARE_POINTER,
    COMPARE_STRUCT,
    COMPARE_ARRAY
} CompareType;

// Struttura per dati di confronto
typedef struct {
    const void *data1;
    const void *data2;
    CompareType type;
    size_t size;            // Per array/struct
    size_t element_count;   // Per array
    compare_func_t custom_compare; // Per struct custom
} CompareContext;

// Funzione di confronto universale
int universal_compare(const CompareContext *ctx) {
    if (!ctx || !ctx->data1 || !ctx->data2) {
        return (ctx->data1 == ctx->data2) ? 0 : 
               (ctx->data1 == NULL) ? -1 : 1;
    }
    
    switch (ctx->type) {
        case COMPARE_INT:
            return compare_int(*(const int*)ctx->data1, *(const int*)ctx->data2);
            
        case COMPARE_FLOAT:
            return compare_float(*(const float*)ctx->data1, *(const float*)ctx->data2);
            
        case COMPARE_DOUBLE:
            return compare_double(*(const double*)ctx->data1, *(const double*)ctx->data2);
            
        case COMPARE_STRING:
            return compare_strings((const char*)ctx->data1, (const char*)ctx->data2);
            
        case COMPARE_POINTER:
            return compare_pointer_addresses(ctx->data1, ctx->data2);
            
        case COMPARE_STRUCT:
            if (ctx->custom_compare) {
                return ctx->custom_compare(ctx->data1, ctx->data2);
            } else {
                return memcmp(ctx->data1, ctx->data2, ctx->size);
            }
            
        case COMPARE_ARRAY:
            if (ctx->custom_compare) {
                return compare_arrays_generic(ctx->data1, ctx->data2, 
                                            ctx->element_count, ctx->size, 
                                            ctx->custom_compare);
            } else {
                return memcmp(ctx->data1, ctx->data2, 
                            ctx->size * ctx->element_count);
            }
    }
    
    return 0;
}

// Macro per semplificare l'uso
#define COMPARE_VALUES(a, b, type) \
    ({ CompareContext ctx = {&(a), &(b), type, 0, 0, NULL}; \
       universal_compare(&ctx); })

#define COMPARE_ARRAYS(arr1, arr2, count, elem_size, type) \
    ({ CompareContext ctx = {arr1, arr2, type, elem_size, count, NULL}; \
       universal_compare(&ctx); })

#define COMPARE_STRUCTS(s1, s2, size, compare_func) \
    ({ CompareContext ctx = {s1, s2, COMPARE_STRUCT, size, 0, compare_func}; \
       universal_compare(&ctx); })
```

### Template C con Macro

```c
// Template generico per funzioni di confronto
#define DEFINE_COMPARE_FUNCTION(TYPE, NAME) \
    int compare_##NAME(const TYPE *a, const TYPE *b) { \
        if (*a == *b) return 0; \
        return (*a < *b) ? -1 : 1; \
    }

#define DEFINE_ARRAY_COMPARE_FUNCTION(TYPE, NAME) \
    int compare_##NAME##_array(const TYPE *arr1, const TYPE *arr2, size_t size) { \
        for (size_t i = 0; i < size; i++) { \
            if (arr1[i] != arr2[i]) { \
                return (arr1[i] < arr2[i]) ? -1 : 1; \
            } \
        } \
        return 0; \
    }

// Genera funzioni per tipi specifici
DEFINE_COMPARE_FUNCTION(int, int)
DEFINE_COMPARE_FUNCTION(long, long)
DEFINE_COMPARE_FUNCTION(short, short)
DEFINE_COMPARE_FUNCTION(char, char)

DEFINE_ARRAY_COMPARE_FUNCTION(int, int)
DEFINE_ARRAY_COMPARE_FUNCTION(long, long)
DEFINE_ARRAY_COMPARE_FUNCTION(char, char)

// Uso delle macro
void test_macro_generated_functions() {
    int a = 10, b = 20;
    printf("compare_int: %d\n", compare_int(&a, &b));
    
    int arr1[] = {1, 2, 3};
    int arr2[] = {1, 2, 4};
    printf("compare_int_array: %d\n", compare_int_array(arr1, arr2, 3));
}
```

---

## Funzioni di Utilità

### Utility Complete per Confronti

```c
// Struttura per risultati di confronto dettagliati
typedef struct {
    int result;             // -1, 0, 1
    int equal;             // 0 o 1
    const char *description; // Descrizione testuale
} CompareResult;

// Crea risultato di confronto
CompareResult make_compare_result(int result, const char *desc) {
    CompareResult cr;
    cr.result = result;
    cr.equal = (result == 0);
    cr.description = desc;
    return cr;
}

// Confronto con risultato dettagliato
CompareResult detailed_compare_int(int a, int b) {
    if (a == b) return make_compare_result(0, "Valori uguali");
    if (a < b) return make_compare_result(-1, "Primo valore minore");
    return make_compare_result(1, "Primo valore maggiore");
}

// Array di confrontatori per diversi tipi
typedef struct {
    const char *type_name;
    compare_func_t compare_func;
    size_t type_size;
} ComparatorInfo;

ComparatorInfo comparators[] = {
    {"int", compare_ints, sizeof(int)},
    {"double", compare_doubles, sizeof(double)},
    // Aggiungi altri tipi...
};

// Trova comparatore per tipo
compare_func_t find_comparator(const char *type_name) {
    int num_comparators = sizeof(comparators) / sizeof(comparators[0]);
    
    for (int i = 0; i < num_comparators; i++) {
        if (strcmp(comparators[i].type_name, type_name) == 0) {
            return comparators[i].compare_func;
        }
    }
    
    return NULL;
}

// Sistema di cache per confronti costosi
#define MAX_CACHE_SIZE 100

typedef struct {
    void *key1, *key2;
    int result;
    int valid;
} CompareCache;

static CompareCache compare_cache[MAX_CACHE_SIZE];
static int cache_index = 0;

int cached_compare(const void *a, const void *b, compare_func_t compare_func) {
    // Cerca in cache
    for (int i = 0; i < MAX_CACHE_SIZE; i++) {
        if (compare_cache[i].valid && 
            compare_cache[i].key1 == a && 
            compare_cache[i].key2 == b) {
            return compare_cache[i].result;
        }
    }
    
    // Non in cache - calcola
    int result = compare_func(a, b);
    
    // Salva in cache
    compare_cache[cache_index].key1 = (void*)a;
    compare_cache[cache_index].key2 = (void*)b;
    compare_cache[cache_index].result = result;
    compare_cache[cache_index].valid = 1;
    
    cache_index = (cache_index + 1) % MAX_CACHE_SIZE;
    
    return result;
}

// Cleanup cache
void clear_compare_cache() {
    for (int i = 0; i < MAX_CACHE_SIZE; i++) {
        compare_cache[i].valid = 0;
    }
    cache_index = 0;
}
```

### Debug e Testing

```c
// Macro per debug confronti
#define DEBUG_COMPARE(a, b, func, expected) \
    do { \
        int result = func(a, b); \
        printf("DEBUG: %s(%s, %s) = %d (expected %d) %s\n", \
               #func, #a, #b, result, expected, \
               (result == expected) ? "PASS" : "FAIL"); \
    } while(0)

// Test automatico per funzioni di confronto
typedef struct {
    const char *test_name;
    void *value1;
    void *value2;
    int expected_result;
    compare_func_t compare_func;
} CompareTest;

int run_compare_tests(CompareTest *tests, int num_tests) {
    int passed = 0;
    
    printf("Esecuzione %d test di confronto...\n", num_tests);
    
    for (int i = 0; i < num_tests; i++) {
        int result = tests[i].compare_func(tests[i].value1, tests[i].value2);
        
        if (result == tests[i].expected_result) {
            printf("✓ %s: PASS\n", tests[i].test_name);
            passed++;
        } else {
            printf("✗ %s: FAIL (got %d, expected %d)\n", 
                   tests[i].test_name, result, tests[i].expected_result);
        }
    }
    
    printf("Risultato: %d/%d test passati\n", passed, num_tests);
    return passed;
}

// Esempio test suite
void run_comparison_test_suite() {
    int a = 10, b = 20, c = 10;
    
    CompareTest tests[] = {
        {"int_equal", &a, &c, 0, compare_ints},
        {"int_less", &a, &b, -1, compare_ints},
        {"int_greater", &b, &a, 1, compare_ints},
    };
    
    run_compare_tests(tests, sizeof(tests) / sizeof(tests[0]));
}
```

## Checklist Confronti

### Punti Chiave da Ricordare

- **Interi**: Confronto diretto con `==`, `<`, `>`
- **Float/Double**: Mai usare `==`, sempre tolleranza (epsilon)  
- **Stringhe**: `strcmp()`, `strncmp()`, `strcasecmp()`
- **Array**: Confronto elemento per elemento
- **Strutture**: Campo per campo o `memcmp()` (attenzione padding)
- **Puntatori**: Confronto indirizzi vs valori puntati
- **Generici**: Funzioni con `void*` e callback di confronto

### Pattern Comuni

1. **Controllo NULL**: Sempre verificare puntatori NULL
2. **Valore di ritorno**: -1 (minore), 0 (uguale), 1 (maggiore)
3. **Tolleranza**: Per numeri floating point
4. **Callback**: Per confronti personalizzati
5. **Caching**: Per confronti costosi

---

## Compilazione e Test

```bash
# Compila esempi
gcc -o comparison_test comparison_examples.c -lm

# Test con valgrind per memory leaks
valgrind --leak-check=full ./comparison_test

# Test con diversi flag di ottimizzazione
gcc -O0 -g -o comparison_debug comparison_examples.c -lm
gcc -O2 -o comparison_optimized comparison_examples.c -lm

# Test comparisons performance
time ./comparison_test
```

Questa guida copre tutti i casi di confronto che potresti incontrare in C, dai tipi base ai più avanzati!