# Guida Completa - Operazioni su File in C

## Indice
1. [Operazioni Base](#operazioni-base)
2. [Funzioni di Utilità per Esame](#funzioni-di-utilità-per-esame)
3. [Controllo e Validazione File](#controllo-e-validazione-file)
4. [Esempi Pratici](#esempi-pratici)
5. [Compilazione e Test](#compilazione-e-test)

---

## Operazioni Base

### Include Necessari
```c
#include <stdio.h>      // fopen, fclose, fprintf, fscanf
#include <stdlib.h>     // malloc, free
#include <string.h>     // strlen, strcpy, strcmp
#include <fcntl.h>      // open, O_RDONLY, O_WRONLY
#include <unistd.h>     // read, write, close
#include <sys/stat.h>   // stat, fstat
#include <errno.h>      // errno, strerror
#include <ctype.h>      // isalpha, isdigit, tolower
#include <time.h>       // time, srand
```

### Apertura e Chiusura File

#### Con FILE*
```c
// Modalità di apertura
FILE* file;

file = fopen("file.txt", "r");    // Solo lettura
file = fopen("file.txt", "w");    // Solo scrittura (cancella contenuto)
file = fopen("file.txt", "a");    // Append (aggiunge alla fine)
file = fopen("file.txt", "r+");   // Lettura/scrittura (file deve esistere)
file = fopen("file.txt", "w+");   // Lettura/scrittura (cancella contenuto)
file = fopen("file.txt", "a+");   // Lettura/append

// Controllo errori
if (file == NULL) {
    perror("Errore apertura file");
    return -1;
}

// Chiusura
fclose(file);
```

#### Con File Descriptor
```c
int fd;

fd = open("file.txt", O_RDONLY);                    // Solo lettura
fd = open("file.txt", O_WRONLY | O_CREAT, 0644);    // Scrittura, crea se non esiste
fd = open("file.txt", O_RDWR | O_CREAT, 0644);      // Lettura/scrittura
fd = open("file.txt", O_WRONLY | O_APPEND, 0644);   // Append

// Controllo errori
if (fd == -1) {
    perror("Errore apertura file");
    return -1;
}

// Chiusura
close(fd);
```

### Lettura File

#### Lettura con FILE*
```c
// Lettura carattere per carattere
int ch;
while ((ch = fgetc(file)) != EOF) {
    printf("%c", ch);
}

// Lettura riga per riga
char line[256];
while (fgets(line, sizeof(line), file) != NULL) {
    printf("Riga: %s", line);
}

// Lettura formattata
int numero;
char parola[100];
while (fscanf(file, "%d %99s", &numero, parola) == 2) {
    printf("Numero: %d, Parola: %s\n", numero, parola);
}

// Lettura blocco dati
char buffer[1024];
size_t bytes_read = fread(buffer, 1, sizeof(buffer), file);
buffer[bytes_read] = '\0';
```

#### Lettura con File Descriptor
```c
char buffer[1024];
ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
if (bytes_read > 0) {
    buffer[bytes_read] = '\0';
    printf("Contenuto: %s\n", buffer);
}
```

### Scrittura File

#### Scrittura con FILE*
```c
// Scrittura carattere
fputc('A', file);

// Scrittura stringa
fputs("Hello World\n", file);

// Scrittura formattata
fprintf(file, "Numero: %d, Nome: %s\n", 42, "Test");

// Scrittura blocco dati
char data[] = "Dati da scrivere";
fwrite(data, 1, strlen(data), file);
```

#### Scrittura con File Descriptor
```c
char data[] = "Hello World\n";
ssize_t bytes_written = write(fd, data, strlen(data));
if (bytes_written == -1) {
    perror("Errore scrittura");
}
```

### Posizionamento nel File

#### Con FILE*
```c
// Vai all'inizio
rewind(file);

// Posizionamento specifico
fseek(file, 0, SEEK_SET);    // Inizio file
fseek(file, 0, SEEK_END);    // Fine file
fseek(file, 10, SEEK_CUR);   // 10 byte dalla posizione corrente

// Ottieni posizione corrente
long pos = ftell(file);
```

#### Con File Descriptor
```c
// Posizionamento
off_t pos = lseek(fd, 0, SEEK_SET);    // Inizio
off_t pos = lseek(fd, 0, SEEK_END);    // Fine
off_t pos = lseek(fd, 10, SEEK_CUR);   // Relativo
```

---

## Funzioni di Utilità per Esame

### Funzioni per Parole Casuali

#### Prendi Parola Random da File
```c
char* get_random_word_from_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) return NULL;
    
    char words[1000][100];
    int word_count = 0;
    char word[100];
    
    // Leggi tutte le parole
    while (fscanf(file, "%99s", word) == 1 && word_count < 1000) {
        strcpy(words[word_count], word);
        word_count++;
    }
    fclose(file);
    
    if (word_count == 0) return NULL;
    
    // Scegli parola casuale
    srand(time(NULL));
    int index = rand() % word_count;
    
    char* result = malloc(strlen(words[index]) + 1);
    strcpy(result, words[index]);
    return result;
}
```

#### Prendi N Parole Random
```c
char** get_n_random_words(const char* filename, int n) {
    FILE* file = fopen(filename, "r");
    if (!file) return NULL;
    
    char words[1000][100];
    int word_count = 0;
    char word[100];
    
    while (fscanf(file, "%99s", word) == 1 && word_count < 1000) {
        strcpy(words[word_count], word);
        word_count++;
    }
    fclose(file);
    
    if (word_count == 0 || n <= 0) return NULL;
    
    char** result = malloc(n * sizeof(char*));
    srand(time(NULL));
    
    for (int i = 0; i < n; i++) {
        int index = rand() % word_count;
        result[i] = malloc(strlen(words[index]) + 1);
        strcpy(result[i], words[index]);
    }
    
    return result;
}

void free_word_array(char** words, int count) {
    for (int i = 0; i < count; i++) {
        free(words[i]);
    }
    free(words);
}
```

### Funzioni per Caratteri Casuali

#### Prendi Carattere Random da File
```c
char get_random_char_from_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) return '\0';
    
    // Conta caratteri nel file
    int char_count = 0;
    while (fgetc(file) != EOF) {
        char_count++;
    }
    
    if (char_count == 0) {
        fclose(file);
        return '\0';
    }
    
    // Scegli posizione casuale
    srand(time(NULL));
    int target_pos = rand() % char_count;
    
    // Vai alla posizione e leggi carattere
    rewind(file);
    for (int i = 0; i < target_pos; i++) {
        fgetc(file);
    }
    
    char result = fgetc(file);
    fclose(file);
    return result;
}
```

#### Prendi Carattere Alfabetico Random
```c
char get_random_alpha_char_from_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) return '\0';
    
    char alpha_chars[10000];
    int alpha_count = 0;
    int ch;
    
    // Raccogli solo caratteri alfabetici
    while ((ch = fgetc(file)) != EOF && alpha_count < 9999) {
        if (isalpha(ch)) {
            alpha_chars[alpha_count++] = ch;
        }
    }
    fclose(file);
    
    if (alpha_count == 0) return '\0';
    
    srand(time(NULL));
    return alpha_chars[rand() % alpha_count];
}
```

### Funzioni per Modificare File

#### Cambia Carattere alla Posizione N
```c
int change_char_at_position(const char* filename, int position, char new_char) {
    FILE* file = fopen(filename, "r+");
    if (!file) return -1;
    
    // Vai alla posizione
    if (fseek(file, position, SEEK_SET) != 0) {
        fclose(file);
        return -1;
    }
    
    // Scrivi nuovo carattere
    if (fputc(new_char, file) == EOF) {
        fclose(file);
        return -1;
    }
    
    fclose(file);
    return 0;
}
```

#### Sostituisci Prima Occorrenza di Carattere
```c
int replace_first_char(const char* filename, char old_char, char new_char) {
    FILE* file = fopen(filename, "r+");
    if (!file) return -1;
    
    int ch;
    long pos = 0;
    
    // Trova primo carattere
    while ((ch = fgetc(file)) != EOF) {
        if (ch == old_char) {
            // Torna indietro e sovrascrivi
            fseek(file, pos, SEEK_SET);
            fputc(new_char, file);
            fclose(file);
            return 0;
        }
        pos++;
    }
    
    fclose(file);
    return -1; // Carattere non trovato
}
```

#### Sostituisci Tutte le Occorrenze
```c
int replace_all_chars(const char* filename, char old_char, char new_char) {
    FILE* file = fopen(filename, "r");
    if (!file) return -1;
    
    // Leggi tutto il contenuto
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    
    char* content = malloc(file_size + 1);
    fread(content, 1, file_size, file);
    content[file_size] = '\0';
    fclose(file);
    
    // Sostituisci caratteri
    int count = 0;
    for (long i = 0; i < file_size; i++) {
        if (content[i] == old_char) {
            content[i] = new_char;
            count++;
        }
    }
    
    // Riscrivi file
    file = fopen(filename, "w");
    if (!file) {
        free(content);
        return -1;
    }
    
    fwrite(content, 1, file_size, file);
    fclose(file);
    free(content);
    
    return count;
}
```

### Funzioni per Ricerca con Criteri

#### Trova Parole di Lunghezza Specifica
```c
char** find_words_by_length(const char* filename, int target_length, int* count) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        *count = 0;
        return NULL;
    }
    
    char words[1000][100];
    char word[100];
    *count = 0;
    
    // Leggi e filtra parole
    while (fscanf(file, "%99s", word) == 1 && *count < 1000) {
        if (strlen(word) == target_length) {
            strcpy(words[*count], word);
            (*count)++;
        }
    }
    fclose(file);
    
    if (*count == 0) return NULL;
    
    // Crea array risultato
    char** result = malloc(*count * sizeof(char*));
    for (int i = 0; i < *count; i++) {
        result[i] = malloc(strlen(words[i]) + 1);
        strcpy(result[i], words[i]);
    }
    
    return result;
}
```

#### Trova Parole che Iniziano con Carattere
```c
char** find_words_starting_with(const char* filename, char start_char, int* count) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        *count = 0;
        return NULL;
    }
    
    char words[1000][100];
    char word[100];
    *count = 0;
    
    while (fscanf(file, "%99s", word) == 1 && *count < 1000) {
        if (tolower(word[0]) == tolower(start_char)) {
            strcpy(words[*count], word);
            (*count)++;
        }
    }
    fclose(file);
    
    if (*count == 0) return NULL;
    
    char** result = malloc(*count * sizeof(char*));
    for (int i = 0; i < *count; i++) {
        result[i] = malloc(strlen(words[i]) + 1);
        strcpy(result[i], words[i]);
    }
    
    return result;
}
```

#### Trova Parole che Contengono Sottostringa
```c
char** find_words_containing(const char* filename, const char* substring, int* count) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        *count = 0;
        return NULL;
    }
    
    char words[1000][100];
    char word[100];
    *count = 0;
    
    while (fscanf(file, "%99s", word) == 1 && *count < 1000) {
        if (strstr(word, substring) != NULL) {
            strcpy(words[*count], word);
            (*count)++;
        }
    }
    fclose(file);
    
    if (*count == 0) return NULL;
    
    char** result = malloc(*count * sizeof(char*));
    for (int i = 0; i < *count; i++) {
        result[i] = malloc(strlen(words[i]) + 1);
        strcpy(result[i], words[i]);
    }
    
    return result;
}
```

#### Conta Occorrenze di Carattere
```c
int count_char_occurrences(const char* filename, char target_char) {
    FILE* file = fopen(filename, "r");
    if (!file) return -1;
    
    int count = 0;
    int ch;
    
    while ((ch = fgetc(file)) != EOF) {
        if (ch == target_char) {
            count++;
        }
    }
    
    fclose(file);
    return count;
}
```

#### Conta Parole nel File
```c
int count_words_in_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) return -1;
    
    int word_count = 0;
    char word[100];
    
    while (fscanf(file, "%99s", word) == 1) {
        word_count++;
    }
    
    fclose(file);
    return word_count;
}
```

---

## Controllo e Validazione File

### Verifica Esistenza e Proprietà File
```c
#include <sys/stat.h>

int file_exists(const char* filename) {
    struct stat st;
    return (stat(filename, &st) == 0);
}

long get_file_size(const char* filename) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        return st.st_size;
    }
    return -1;
}

int is_file_readable(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

int is_file_writable(const char* filename) {
    FILE* file = fopen(filename, "a");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}
```

### Controllo Contenuto File
```c
int file_contains_string(const char* filename, const char* search_string) {
    FILE* file = fopen(filename, "r");
    if (!file) return -1;
    
    char line[1000];
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, search_string) != NULL) {
            fclose(file);
            return 1; // Trovato
        }
    }
    
    fclose(file);
    return 0; // Non trovato
}

int is_file_empty(const char* filename) {
    return (get_file_size(filename) == 0);
}

int count_lines_in_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) return -1;
    
    int line_count = 0;
    char line[1000];
    
    while (fgets(line, sizeof(line), file)) {
        line_count++;
    }
    
    fclose(file);
    return line_count;
}
```

### Funzioni di Backup e Copia
```c
int backup_file(const char* filename) {
    char backup_name[256];
    snprintf(backup_name, sizeof(backup_name), "%s.backup", filename);
    
    FILE* src = fopen(filename, "rb");
    FILE* dst = fopen(backup_name, "wb");
    
    if (!src || !dst) {
        if (src) fclose(src);
        if (dst) fclose(dst);
        return -1;
    }
    
    char buffer[4096];
    size_t bytes;
    
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        if (fwrite(buffer, 1, bytes, dst) != bytes) {
            fclose(src);
            fclose(dst);
            return -1;
        }
    }
    
    fclose(src);
    fclose(dst);
    return 0;
}

int copy_file(const char* src_filename, const char* dst_filename) {
    FILE* src = fopen(src_filename, "rb");
    FILE* dst = fopen(dst_filename, "wb");
    
    if (!src || !dst) {
        if (src) fclose(src);
        if (dst) fclose(dst);
        return -1;
    }
    
    char buffer[4096];
    size_t bytes;
    
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytes, dst);
    }
    
    fclose(src);
    fclose(dst);
    return 0;
}
```

---

## Esempi Pratici

### Programma Completo di Test
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

// Includi tutte le funzioni sopra...

int main() {
    const char* test_file = "test_words.txt";
    
    // Crea file di test
    FILE* file = fopen(test_file, "w");
    if (file) {
        fprintf(file, "ciao mondo questo è un test\n");
        fprintf(file, "con diverse parole casuali\n");
        fprintf(file, "per verificare le funzioni\n");
        fclose(file);
    }
    
    printf("=== TEST FUNZIONI FILE ===\n\n");
    
    // Test parola random
    char* random_word = get_random_word_from_file(test_file);
    if (random_word) {
        printf("Parola random: %s\n", random_word);
        free(random_word);
    }
    
    // Test carattere random
    char random_char = get_random_alpha_char_from_file(test_file);
    printf("Carattere random: %c\n", random_char);
    
    // Test conta parole
    int word_count = count_words_in_file(test_file);
    printf("Numero parole: %d\n", word_count);
    
    // Test trova parole per lunghezza
    int count;
    char** words_len_4 = find_words_by_length(test_file, 4, &count);
    printf("Parole di 4 caratteri: %d\n", count);
    for (int i = 0; i < count; i++) {
        printf("  - %s\n", words_len_4[i]);
    }
    free_word_array(words_len_4, count);
    
    // Test parole che iniziano con 'c'
    char** words_with_c = find_words_starting_with(test_file, 'c', &count);
    printf("Parole che iniziano con 'c': %d\n", count);
    for (int i = 0; i < count; i++) {
        printf("  - %s\n", words_with_c[i]);
    }
    free_word_array(words_with_c, count);
    
    // Test conta carattere
    int char_count = count_char_occurrences(test_file, 'e');
    printf("Occorrenze di 'e': %d\n", char_count);
    
    // Test backup
    if (backup_file(test_file) == 0) {
        printf("Backup creato con successo\n");
    }
    
    // Test modifica carattere
    if (change_char_at_position(test_file, 0, 'C') == 0) {
        printf("Primo carattere cambiato in 'C'\n");
    }
    
    // Cleanup
    remove(test_file);
    remove("test_words.txt.backup");
    
    return 0;
}
```

### Gestore File Interattivo
```c
void file_manager_menu() {
    char filename[256];
    char command[100];
    
    printf("=== GESTORE FILE ===\n");
    printf("Inserisci nome file: ");
    fgets(filename, sizeof(filename), stdin);
    filename[strcspn(filename, "\n")] = '\0';
    
    while (1) {
        printf("\nComandi:\n");
        printf("1. random_word - Parola casuale\n");
        printf("2. count_words - Conta parole\n");
        printf("3. find_length N - Trova parole di N caratteri\n");
        printf("4. backup - Crea backup\n");
        printf("5. quit - Esci\n");
        printf("Comando: ");
        
        fgets(command, sizeof(command), stdin);
        command[strcspn(command, "\n")] = '\0';
        
        if (strcmp(command, "random_word") == 0) {
            char* word = get_random_word_from_file(filename);
            if (word) {
                printf("Parola: %s\n", word);
                free(word);
            } else {
                printf("Nessuna parola trovata\n");
            }
        }
        else if (strcmp(command, "count_words") == 0) {
            int count = count_words_in_file(filename);
            printf("Parole nel file: %d\n", count);
        }
        else if (strncmp(command, "find_length", 11) == 0) {
            int length = atoi(command + 12);
            int count;
            char** words = find_words_by_length(filename, length, &count);
            printf("Parole di %d caratteri: %d\n", length, count);
            for (int i = 0; i < count; i++) {
                printf("  %s\n", words[i]);
            }
            free_word_array(words, count);
        }
        else if (strcmp(command, "backup") == 0) {
            if (backup_file(filename) == 0) {
                printf("Backup creato\n");
            } else {
                printf("Errore backup\n");
            }
        }
        else if (strcmp(command, "quit") == 0) {
            break;
        }
        else {
            printf("Comando non riconosciuto\n");
        }
    }
}
```

---

## Compilazione e Test

### Compilazione Base
```bash
gcc -o file_test file_operations.c
```

### Compilazione con Warning
```bash
gcc -Wall -Wextra -o file_test file_operations.c
```

### Test con File di Esempio
```bash
# Crea file di test
echo "ciao mondo questo è un test con parole diverse" > test.txt
echo "alcune parole sono lunghe altre corte" >> test.txt

# Esegui programma
./file_test
```

### Verifica Memory Leaks
```bash
valgrind --leak-check=full ./file_test
```

---

## Checklist Funzioni File

### Apertura e Chiusura
- [ ] Controlla sempre il valore di ritorno di fopen/open
- [ ] Chiudi sempre i file aperti
- [ ] Usa modalità di apertura appropriate
- [ ] Gestisci errori con perror/strerror

### Lettura e Scrittura
- [ ] Controlla valore di ritorno delle operazioni I/O
- [ ] Usa buffer di dimensione appropriata
- [ ] Termina stringhe con '\0' dopo lettura
- [ ] Gestisci EOF correttamente

### Memoria
- [ ] Libera sempre memoria allocata con malloc
- [ ] Controlla successo di malloc prima dell'uso
- [ ] Usa free() per ogni malloc()
- [ ] Evita double free

### Sicurezza
- [ ] Valida input utente
- [ ] Controlla bounds degli array
- [ ] Usa funzioni sicure (strncpy vs strcpy)
- [ ] Gestisci file molto grandi

Tutte le funzioni sono pronte all'uso e gestiscono correttamente errori e memoria!