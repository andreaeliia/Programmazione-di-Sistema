# File I/O e Directory Management - Riferimento Completo

## Concetti Fondamentali

### **File Descriptor**
- **Numero intero**: identifica file aperti nel processo
- **Standard FD**: 0 (stdin), 1 (stdout), 2 (stderr)
- **Tabella FD**: ogni processo ha propria tabella
- **Eredità**: processi figli ereditano FD aperti del padre

### **Tipi di File**
- **Regular files**: file normali con dati
- **Directories**: cartelle contenenti altri file
- **Special files**: device files, pipes, sockets
- **Links**: symbolic links e hard links

### **Modalità Accesso**
- **Read only** (O_RDONLY): solo lettura
- **Write only** (O_WRONLY): solo scrittura  
- **Read/Write** (O_RDWR): lettura e scrittura
- **Append** (O_APPEND): scrittura in coda
- **Create** (O_CREAT): crea se non esiste

---

## File I/O Base - System Calls

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <time.h>

// Esempio file I/O base con system calls
void basic_file_io_example() {
    printf("=== FILE I/O BASE CON SYSTEM CALLS ===\n");
    
    const char* filename = "test_file.txt";
    const char* data = "Questo è un test di scrittura file\nSeconda riga\nTerza riga\n";
    char read_buffer[256];
    int fd;
    ssize_t bytes_written, bytes_read;
    
    // 1. CREAZIONE E SCRITTURA FILE
    printf("Creazione e scrittura file '%s'\n", filename);
    
    fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open per scrittura");
        return;
    }
    
    bytes_written = write(fd, data, strlen(data));
    if (bytes_written == -1) {
        perror("write");
        close(fd);
        return;
    }
    
    printf("Scritti %zd bytes nel file\n", bytes_written);
    close(fd);
    
    // 2. LETTURA FILE
    printf("\nLettura file '%s'\n", filename);
    
    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open per lettura");
        return;
    }
    
    bytes_read = read(fd, read_buffer, sizeof(read_buffer) - 1);
    if (bytes_read == -1) {
        perror("read");
        close(fd);
        return;
    }
    
    read_buffer[bytes_read] = '\0';
    printf("Letti %zd bytes:\n%s\n", bytes_read, read_buffer);
    close(fd);
    
    // 3. APPEND AL FILE
    printf("Append al file\n");
    
    fd = open(filename, O_WRONLY | O_APPEND);
    if (fd == -1) {
        perror("open per append");
        return;
    }
    
    const char* append_data = "Riga aggiunta con append\n";
    bytes_written = write(fd, append_data, strlen(append_data));
    
    printf("Aggiunti %zd bytes al file\n", bytes_written);
    close(fd);
    
    // 4. LETTURA FINALE
    printf("\nContenuto finale:\n");
    
    fd = open(filename, O_RDONLY);
    if (fd != -1) {
        while ((bytes_read = read(fd, read_buffer, sizeof(read_buffer) - 1)) > 0) {
            read_buffer[bytes_read] = '\0';
            printf("%s", read_buffer);
        }
        close(fd);
    }
    
    // Cleanup
    unlink(filename);
    printf("\nFile rimosso\n");
}

// Copia file efficiente
ssize_t copy_file(const char* source, const char* destination) {
    int src_fd, dst_fd;
    char buffer[4096];
    ssize_t bytes_read, bytes_written, total_copied = 0;
    struct stat src_stats;
    
    // Apri file sorgente
    src_fd = open(source, O_RDONLY);
    if (src_fd == -1) {
        perror("open source");
        return -1;
    }
    
    // Ottieni permessi file sorgente
    if (fstat(src_fd, &src_stats) == -1) {
        perror("fstat");
        close(src_fd);
        return -1;
    }
    
    // Crea file destinazione con stessi permessi
    dst_fd = open(destination, O_CREAT | O_WRONLY | O_TRUNC, src_stats.st_mode);
    if (dst_fd == -1) {
        perror("open destination");
        close(src_fd);
        return -1;
    }
    
    // Copia dati
    while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
        bytes_written = write(dst_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            perror("write");
            close(src_fd);
            close(dst_fd);
            unlink(destination);  // Rimuovi file parzialmente copiato
            return -1;
        }
        total_copied += bytes_written;
    }
    
    if (bytes_read == -1) {
        perror("read");
        close(src_fd);
        close(dst_fd);
        unlink(destination);
        return -1;
    }
    
    close(src_fd);
    close(dst_fd);
    
    return total_copied;
}
```

---

## Directory Management e Traversal Completo

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

// Struttura per raccogliere informazioni sui file
typedef struct {
    char **file_paths;
    int file_count;
    int capacity;
    long total_size;
} FileCollection;

// Inizializza collezione file
FileCollection* init_file_collection() {
    FileCollection* fc = malloc(sizeof(FileCollection));
    fc->file_paths = malloc(100 * sizeof(char*));
    fc->file_count = 0;
    fc->capacity = 100;
    fc->total_size = 0;
    return fc;
}

// Aggiungi file alla collezione
void add_file_to_collection(FileCollection* fc, const char* filepath, long size) {
    if (fc->file_count >= fc->capacity) {
        fc->capacity *= 2;
        fc->file_paths = realloc(fc->file_paths, fc->capacity * sizeof(char*));
    }
    
    fc->file_paths[fc->file_count] = malloc(strlen(filepath) + 1);
    strcpy(fc->file_paths[fc->file_count], filepath);
    fc->file_count++;
    fc->total_size += size;
}

// Libera collezione file
void free_file_collection(FileCollection* fc) {
    for (int i = 0; i < fc->file_count; i++) {
        free(fc->file_paths[i]);
    }
    free(fc->file_paths);
    free(fc);
}

// FUNZIONE PRINCIPALE: Raccoglie TUTTI i file da una directory ricorsivamente
int collect_all_files_recursive(const char* directory_path, FileCollection* fc) {
    DIR* dir;
    struct dirent* entry;
    struct stat entry_stats;
    char full_path[1024];
    
    printf("Esplorando directory: %s\n", directory_path);
    
    dir = opendir(directory_path);
    if (dir == NULL) {
        printf("Errore apertura directory %s: %s\n", directory_path, strerror(errno));
        return -1;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        // Salta . e ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Costruisci path completo
        snprintf(full_path, sizeof(full_path), "%s/%s", directory_path, entry->d_name);
        
        // Ottieni informazioni sul file/directory
        if (stat(full_path, &entry_stats) != 0) {
            printf("Errore stat su %s: %s\n", full_path, strerror(errno));
            continue;
        }
        
        if (S_ISREG(entry_stats.st_mode)) {
            // È un file regolare - aggiungilo alla collezione
            add_file_to_collection(fc, full_path, entry_stats.st_size);
            printf("File trovato: %s (%ld bytes)\n", full_path, entry_stats.st_size);
            
        } else if (S_ISDIR(entry_stats.st_mode)) {
            // È una directory - ricorsione
            printf("Directory trovata: %s (esplorando...)\n", full_path);
            collect_all_files_recursive(full_path, fc);
            
        } else if (S_ISLNK(entry_stats.st_mode)) {
            // È un link simbolico
            printf("Link simbolico trovato: %s\n", full_path);
            // Opzionale: puoi decidere se seguire i link o meno
            
        } else {
            // Altri tipi di file (device, pipe, etc.)
            printf("File speciale trovato: %s\n", full_path);
        }
    }
    
    closedir(dir);
    return 0;
}

// Esempio di utilizzo principale
void collect_all_files_example() {
    printf("=== RACCOLTA COMPLETA DI TUTTI I FILE ===\n");
    
    // Directory da esplorare (può essere cambiata)
    const char* target_directory = "/tmp";  // Cambia con la directory desiderata
    
    // Inizializza collezione
    FileCollection* fc = init_file_collection();
    
    printf("Inizio raccolta di tutti i file da: %s\n\n", target_directory);
    
    // Raccoglie tutti i file ricorsivamente
    if (collect_all_files_recursive(target_directory, fc) == 0) {
        printf("\n=== RISULTATI RACCOLTA ===\n");
        printf("Totale file trovati: %d\n", fc->file_count);
        printf("Spazio totale: %ld bytes (%.2f MB)\n", 
               fc->total_size, fc->total_size / (1024.0 * 1024.0));
        
        printf("\nElenco completo file:\n");
        for (int i = 0; i < fc->file_count; i++) {
            printf("%4d. %s\n", i + 1, fc->file_paths[i]);
        }
        
    } else {
        printf("Errore durante la raccolta file\n");
    }
    
    // Cleanup
    free_file_collection(fc);
}

// Versione semplificata che stampa solo i percorsi
void print_all_files_recursive(const char* directory_path, int depth) {
    DIR* dir;
    struct dirent* entry;
    struct stat entry_stats;
    char full_path[1024];
    
    // Indentazione per mostrare la gerarchia
    for (int i = 0; i < depth; i++) {
        printf("  ");
    }
    printf("+ %s/\n", directory_path);
    
    dir = opendir(directory_path);
    if (dir == NULL) {
        return;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        snprintf(full_path, sizeof(full_path), "%s/%s", directory_path, entry->d_name);
        
        if (stat(full_path, &entry_stats) == 0) {
            // Indentazione
            for (int i = 0; i <= depth; i++) {
                printf("  ");
            }
            
            if (S_ISREG(entry_stats.st_mode)) {
                printf("- %s (%ld bytes)\n", entry->d_name, entry_stats.st_size);
            } else if (S_ISDIR(entry_stats.st_mode)) {
                // Ricorsione per subdirectory
                print_all_files_recursive(full_path, depth + 1);
            } else {
                printf("? %s (special)\n", entry->d_name);
            }
        }
    }
    
    closedir(dir);
}

// Raccolta file con filtri
typedef struct {
    char **extensions;
    int ext_count;
    long max_size;
    long min_size;
} FileFilter;

int matches_filter(const char* filename, const struct stat* stats, FileFilter* filter) {
    // Controllo dimensione
    if (filter->max_size > 0 && stats->st_size > filter->max_size) {
        return 0;
    }
    if (filter->min_size > 0 && stats->st_size < filter->min_size) {
        return 0;
    }
    
    // Controllo estensioni
    if (filter->ext_count > 0) {
        char* ext = strrchr(filename, '.');
        if (!ext) return 0;  // Nessuna estensione
        
        for (int i = 0; i < filter->ext_count; i++) {
            if (strcmp(ext, filter->extensions[i]) == 0) {
                return 1;
            }
        }
        return 0;  // Estensione non nella lista
    }
    
    return 1;  // Nessun filtro o tutti i filtri passati
}

int collect_filtered_files_recursive(const char* directory_path, FileCollection* fc, FileFilter* filter) {
    DIR* dir;
    struct dirent* entry;
    struct stat entry_stats;
    char full_path[1024];
    
    dir = opendir(directory_path);
    if (dir == NULL) {
        return -1;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        snprintf(full_path, sizeof(full_path), "%s/%s", directory_path, entry->d_name);
        
        if (stat(full_path, &entry_stats) == 0) {
            if (S_ISREG(entry_stats.st_mode)) {
                // Applica filtri
                if (matches_filter(entry->d_name, &entry_stats, filter)) {
                    add_file_to_collection(fc, full_path, entry_stats.st_size);
                    printf("File filtrato: %s (%ld bytes)\n", full_path, entry_stats.st_size);
                }
            } else if (S_ISDIR(entry_stats.st_mode)) {
                // Ricorsione per subdirectory
                collect_filtered_files_recursive(full_path, fc, filter);
            }
        }
    }
    
    closedir(dir);
    return 0;
}

// Esempio con filtri
void filtered_file_collection_example() {
    printf("\n=== RACCOLTA FILE CON FILTRI ===\n");
    
    const char* target_directory = "/usr/share/doc";  // Directory di esempio
    
    // Setup filtri
    FileFilter filter;
    char* extensions[] = {".txt", ".md", ".html", ".xml"};
    filter.extensions = extensions;
    filter.ext_count = 4;
    filter.max_size = 1024 * 1024;  // Max 1MB
    filter.min_size = 100;          // Min 100 bytes
    
    FileCollection* fc = init_file_collection();
    
    printf("Cercando file con estensioni: .txt, .md, .html, .xml\n");
    printf("Dimensione tra 100 bytes e 1MB\n");
    printf("Directory: %s\n\n", target_directory);
    
    if (collect_filtered_files_recursive(target_directory, fc, &filter) == 0) {
        printf("\n=== RISULTATI FILTRATI ===\n");
        printf("File trovati: %d\n", fc->file_count);
        printf("Spazio totale: %ld bytes\n", fc->total_size);
        
        // Mostra primi 20 risultati
        int max_show = fc->file_count < 20 ? fc->file_count : 20;
        printf("\nPrimi %d risultati:\n", max_show);
        for (int i = 0; i < max_show; i++) {
            printf("%3d. %s\n", i + 1, fc->file_paths[i]);
        }
        
        if (fc->file_count > 20) {
            printf("... e altri %d file\n", fc->file_count - 20);
        }
    }
    
    free_file_collection(fc);
}

// Crea struttura di test per dimostrare la raccolta
int create_test_directory_structure(const char* base_path) {
    printf("=== CREAZIONE STRUTTURA DI TEST ===\n");
    
    char path[256];
    
    // Crea directory base
    if (mkdir(base_path, 0755) != 0 && errno != EEXIST) {
        perror("mkdir base");
        return -1;
    }
    printf("Directory base creata: %s\n", base_path);
    
    // Crea sottodirectory
    const char* subdirs[] = {
        "docs", "src", "bin", "tests", "logs",
        "docs/manual", "docs/api", "src/core", "src/utils"
    };
    int num_subdirs = sizeof(subdirs) / sizeof(subdirs[0]);
    
    for (int i = 0; i < num_subdirs; i++) {
        snprintf(path, sizeof(path), "%s/%s", base_path, subdirs[i]);
        
        if (mkdir(path, 0755) != 0 && errno != EEXIST) {
            perror("mkdir subdir");
            continue;
        }
        printf("Subdirectory creata: %s\n", path);
    }
    
    // Crea file di test
    const char* test_files[] = {
        "README.md",
        "docs/manual/user_guide.txt",
        "docs/manual/install.md", 
        "docs/api/reference.html",
        "src/main.c",
        "src/utils.c",
        "src/core/engine.c",
        "src/core/config.h",
        "tests/test_main.c",
        "tests/test_utils.c",
        "bin/app",
        "logs/error.log",
        "logs/access.log"
    };
    int num_files = sizeof(test_files) / sizeof(test_files[0]);
    
    for (int i = 0; i < num_files; i++) {
        snprintf(path, sizeof(path), "%s/%s", base_path, test_files[i]);
        
        int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd != -1) {
            char content[256];
            snprintf(content, sizeof(content), 
                    "File di test: %s\nCreato il: %ld\nNumero riga: %d\n", 
                    test_files[i], time(NULL), i);
            write(fd, content, strlen(content));
            close(fd);
            printf("File creato: %s\n", path);
        }
    }
    
    return 0;
}

// Esempio completo con test
void complete_directory_traversal_example() {
    printf("=== ESEMPIO COMPLETO TRAVERSAL DIRECTORY ===\n");
    
    const char* test_dir = "test_complete_traversal";
    
    // 1. Crea struttura di test
    create_test_directory_structure(test_dir);
    
    // 2. Raccoglie tutti i file
    printf("\n=== RACCOLTA TUTTI I FILE ===\n");
    collect_all_files_example_local(test_dir);
    
    // 3. Stampa struttura ad albero
    printf("\n=== STRUTTURA AD ALBERO ===\n");
    print_all_files_recursive(test_dir, 0);
    
    // 4. Raccolta con filtri
    printf("\n=== RACCOLTA CON FILTRI (.c e .h) ===\n");
    FileFilter filter;
    char* extensions[] = {".c", ".h"};
    filter.extensions = extensions;
    filter.ext_count = 2;
    filter.max_size = 0;  // Nessun limite
    filter.min_size = 0;
    
    FileCollection* fc = init_file_collection();
    collect_filtered_files_recursive(test_dir, fc, &filter);
    
    printf("File C/H trovati: %d\n", fc->file_count);
    for (int i = 0; i < fc->file_count; i++) {
        printf("  %s\n", fc->file_paths[i]);
    }
    
    free_file_collection(fc);
    
    // 5. Cleanup
    printf("\n=== CLEANUP ===\n");
    remove_directory_recursive(test_dir);
    
    printf("Esempio completo terminato\n");
}

// Versione locale della funzione per il test
void collect_all_files_example_local(const char* directory) {
    FileCollection* fc = init_file_collection();
    
    printf("Raccogliendo tutti i file da: %s\n", directory);
    
    if (collect_all_files_recursive(directory, fc) == 0) {
        printf("Totale file: %d\n", fc->file_count);
        printf("Spazio totale: %ld bytes\n", fc->total_size);
        
        for (int i = 0; i < fc->file_count; i++) {
            printf("  %s\n", fc->file_paths[i]);
        }
    }
    
    free_file_collection(fc);
}

// Rimozione ricorsiva directory (per cleanup)
int remove_directory_recursive(const char* directory_path) {
    DIR* dir;
    struct dirent* entry;
    struct stat entry_stats;
    char full_path[512];
    
    dir = opendir(directory_path);
    if (dir == NULL) {
        return -1;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        snprintf(full_path, sizeof(full_path), "%s/%s", directory_path, entry->d_name);
        
        if (stat(full_path, &entry_stats) == 0) {
            if (S_ISDIR(entry_stats.st_mode)) {
                remove_directory_recursive(full_path);
                rmdir(full_path);
            } else {
                unlink(full_path);
            }
        }
    }
    
    closedir(dir);
    rmdir(directory_path);
    
    return 0;
}

// Funzione main di esempio
int main() {
    // Esempio base file I/O
    basic_file_io_example();
    
    // Esempio completo directory traversal
    complete_directory_traversal_example();
    
    // Esempio raccolta file esistenti
    printf("\n=== PROVA SU DIRECTORY REALE ===\n");
    printf("Raccogliendo file da /etc (primi 50):\n");
    
    FileCollection* fc = init_file_collection();
    collect_all_files_recursive("/etc", fc);
    
    int max_show = fc->file_count < 50 ? fc->file_count : 50;
    printf("Mostrando primi %d di %d file trovati:\n", max_show, fc->file_count);
    
    for (int i = 0; i < max_show; i++) {
        printf("%3d. %s\n", i + 1, fc->file_paths[i]);
    }
    
    free_file_collection(fc);
    
    return 0;
}
```

---

## Funzione Ready-to-Use per Raccolta File

```c
// Funzione semplice e pronta all'uso per raccogliere tutti i file
int get_all_files_from_directory(const char* directory_path, 
                                 char*** file_list, 
                                 int* file_count) {
    FileCollection* fc = init_file_collection();
    
    if (collect_all_files_recursive(directory_path, fc) != 0) {
        free_file_collection(fc);
        return -1;
    }
    
    // Trasferisci risultati
    *file_list = fc->file_paths;
    *file_count = fc->file_count;
    
    // Non liberare i path perché li stiamo restituendo
    free(fc);
    
    return 0;
}

// Esempio di utilizzo della funzione ready-to-use
void example_usage() {
    char** all_files;
    int total_files;
    
    // Ottieni tutti i file dalla directory
    if (get_all_files_from_directory("/home/user/documents", &all_files, &total_files) == 0) {
        printf("Trovati %d file:\n", total_files);
        
        for (int i = 0; i < total_files; i++) {
            printf("%s\n", all_files[i]);
            free(all_files[i]);  // Libera ogni path
        }
        
        free(all_files);  // Libera l'array di puntatori
    } else {
        printf("Errore nella raccolta file\n");
    }
}
```

---

## Checklist Directory Traversal

### **Traversal Completo**
- [ ] Usa `opendir()`, `readdir()`, `closedir()` per attraversare directory
- [ ] Controlla sempre il tipo di file con `stat()` o `entry->d_type`
- [ ] Implementa ricorsione per subdirectory
- [ ] Gestisci casi di errore (permessi, directory inesistenti)
- [ ] Evita loop infiniti (skip "." e "..")

### **Raccolta File**
- [ ] Alloca dinamicamente memoria per lista file
- [ ] Ridimensiona array quando necessario (`realloc()`)
- [ ] Salva path completi, non solo nomi file
- [ ] Traccia statistiche (count, dimensioni totali)
- [ ] Libera sempre la memoria allocata

### **Sicurezza e Performance**
- [ ] Limita profondità ricorsione per evitare stack overflow
- [ ] Gestisci permessi negati senza terminare il programma
- [ ] Evita seguire link simbolici se non necessario
- [ ] Usa buffer appropriati per path lunghi
- [ ] Controlla ritorno di tutte le system call

---

## Compilazione e Test

```bash
# Compila il programma
gcc -o directory_traversal directory_traversal.c

# Test su directory piccola
./directory_traversal

# Test su directory sistema (con attenzione)
sudo ./directory_traversal

# Conta quanti file trova
./directory_traversal | grep "File trovato" | wc -l

# Performance test
time ./directory_traversal

# Memory check
valgrind --leak-check=full ./directory_traversal
```