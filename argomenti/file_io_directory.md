# File I/O e Directory Management - Riferimento Completo

## 🎯 Concetti Fondamentali

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

## 📁 File I/O Base - System Calls

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
    printf("📝 Creazione e scrittura file '%s'\n", filename);
    
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
    
    printf("✅ Scritti %zd bytes nel file\n", bytes_written);
    close(fd);
    
    // 2. LETTURA FILE
    printf("\n📖 Lettura file '%s'\n", filename);
    
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
    printf("✅ Letti %zd bytes:\n%s\n", bytes_read, read_buffer);
    close(fd);
    
    // 3. APPEND AL FILE
    printf("📝 Append al file\n");
    
    fd = open(filename, O_WRONLY | O_APPEND);
    if (fd == -1) {
        perror("open per append");
        return;
    }
    
    const char* append_data = "Riga aggiunta con append\n";
    bytes_written = write(fd, append_data, strlen(append_data));
    
    printf("✅ Aggiunti %zd bytes al file\n", bytes_written);
    close(fd);
    
    // 4. LETTURA FINALE
    printf("\n📖 Contenuto finale:\n");
    
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
    printf("\n🗑️  File rimosso\n");
}

// Gestione avanzata file descriptor
void advanced_file_descriptor_example() {
    printf("\n=== GESTIONE AVANZATA FILE DESCRIPTOR ===\n");
    
    const char* filename = "advanced_test.txt";
    int fd1, fd2, fd3;
    char buffer[64];
    off_t position;
    
    // Crea file di test
    fd1 = open(filename, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fd1 == -1) {
        perror("open create");
        return;
    }
    
    // Scrivi dati di test
    write(fd1, "0123456789ABCDEFGHIJ", 20);
    
    printf("📄 File creato con 20 bytes: '0123456789ABCDEFGHIJ'\n");
    
    // 1. DUPLICAZIONE FILE DESCRIPTOR
    printf("\n🔄 Duplicazione file descriptor\n");
    
    fd2 = dup(fd1);        // Duplica FD
    fd3 = dup2(fd1, 10);   // Duplica su FD specifico (10)
    
    printf("FD originale: %d\n", fd1);
    printf("FD duplicato (dup): %d\n", fd2);
    printf("FD duplicato (dup2): %d\n", fd3);
    
    // Tutti i FD puntano allo stesso file!
    
    // 2. SEEK - POSIZIONAMENTO NEL FILE
    printf("\n🎯 Posizionamento nel file (lseek)\n");
    
    // Posiziona all'inizio
    position = lseek(fd1, 0, SEEK_SET);
    printf("Posizione dopo SEEK_SET(0): %ld\n", position);
    
    read(fd1, buffer, 5);
    buffer[5] = '\0';
    printf("Letti 5 bytes: '%s'\n", buffer);
    
    // Posizione corrente
    position = lseek(fd1, 0, SEEK_CUR);
    printf("Posizione corrente: %ld\n", position);
    
    // Vai alla fine
    position = lseek(fd1, 0, SEEK_END);
    printf("Posizione fine file: %ld\n", position);
    
    // Posiziona 5 bytes dall'inizio
    lseek(fd1, 5, SEEK_SET);
    read(fd1, buffer, 5);
    buffer[5] = '\0';
    printf("5 bytes da posizione 5: '%s'\n", buffer);
    
    // 3. FCNTL - CONTROLLO FILE DESCRIPTOR
    printf("\n⚙️  Controllo file descriptor (fcntl)\n");
    
    // Ottieni flag attuali
    int flags = fcntl(fd1, F_GETFL);
    if (flags != -1) {
        printf("Flag file: ");
        if (flags & O_RDONLY) printf("O_RDONLY ");
        if (flags & O_WRONLY) printf("O_WRONLY ");
        if (flags & O_RDWR) printf("O_RDWR ");
        if (flags & O_APPEND) printf("O_APPEND ");
        printf("\n");
    }
    
    // Imposta flag non-bloccante
    fcntl(fd1, F_SETFL, flags | O_NONBLOCK);
    printf("Flag O_NONBLOCK impostato\n");
    
    // 4. FILE LOCKING
    printf("\n🔒 File locking\n");
    
    struct flock lock;
    lock.l_type = F_WRLCK;      // Write lock
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;             // Lock intero file
    
    if (fcntl(fd1, F_SETLK, &lock) == 0) {
        printf("✅ File locked per scrittura\n");
        
        // Prova a fare lock con altro FD (dovrebbe fallire)
        if (fcntl(fd2, F_SETLK, &lock) == -1) {
            printf("❌ Lock fallito su FD duplicato (corretto!)\n");
        }
        
        // Unlock
        lock.l_type = F_UNLCK;
        fcntl(fd1, F_SETLK, &lock);
        printf("🔓 File unlocked\n");
        
    } else {
        printf("❌ Errore file lock: %s\n", strerror(errno));
    }
    
    // Cleanup
    close(fd1);
    close(fd2);
    close(fd3);
    unlink(filename);
    
    printf("🗑️  File e FD puliti\n");
}

// Statistiche file
void file_statistics_example() {
    printf("\n=== STATISTICHE FILE ===\n");
    
    const char* filename = "stats_test.txt";
    struct stat file_stats;
    int fd;
    
    // Crea file di test
    fd = open(filename, O_CREAT | O_WRONLY, 0755);
    if (fd == -1) {
        perror("open");
        return;
    }
    
    write(fd, "File di test per statistiche\n", 29);
    close(fd);
    
    // Ottieni statistiche con stat()
    if (stat(filename, &file_stats) == 0) {
        printf("📊 Statistiche file '%s':\n", filename);
        printf("   📏 Dimensione: %ld bytes\n", file_stats.st_size);
        printf("   🔗 Hard links: %ld\n", file_stats.st_nlink);
        printf("   🆔 Inode: %ld\n", file_stats.st_ino);
        printf("   👤 UID: %d\n", file_stats.st_uid);
        printf("   👥 GID: %d\n", file_stats.st_gid);
        
        // Permessi
        printf("   🔐 Permessi: ");
        printf((S_ISDIR(file_stats.st_mode)) ? "d" : "-");
        printf((file_stats.st_mode & S_IRUSR) ? "r" : "-");
        printf((file_stats.st_mode & S_IWUSR) ? "w" : "-");
        printf((file_stats.st_mode & S_IXUSR) ? "x" : "-");
        printf((file_stats.st_mode & S_IRGRP) ? "r" : "-");
        printf((file_stats.st_mode & S_IWGRP) ? "w" : "-");
        printf((file_stats.st_mode & S_IXGRP) ? "x" : "-");
        printf((file_stats.st_mode & S_IROTH) ? "r" : "-");
        printf((file_stats.st_mode & S_IWOTH) ? "w" : "-");
        printf((file_stats.st_mode & S_IXOTH) ? "x" : "-");
        printf(" (%o)\n", file_stats.st_mode & 0777);
        
        // Timestamp
        printf("   📅 Creazione: %s", ctime(&file_stats.st_ctime));
        printf("   📝 Modifica: %s", ctime(&file_stats.st_mtime));
        printf("   👁️  Accesso: %s", ctime(&file_stats.st_atime));
        
        // Tipo file
        printf("   📄 Tipo: ");
        if (S_ISREG(file_stats.st_mode)) printf("File regolare\n");
        else if (S_ISDIR(file_stats.st_mode)) printf("Directory\n");
        else if (S_ISLNK(file_stats.st_mode)) printf("Symbolic link\n");
        else if (S_ISBLK(file_stats.st_mode)) printf("Block device\n");
        else if (S_ISCHR(file_stats.st_mode)) printf("Character device\n");
        else if (S_ISFIFO(file_stats.st_mode)) printf("FIFO/pipe\n");
        else if (S_ISSOCK(file_stats.st_mode)) printf("Socket\n");
        else printf("Sconosciuto\n");
        
    } else {
        perror("stat");
    }
    
    // Test accesso file
    printf("\n🔍 Test accesso file:\n");
    printf("   Leggibile: %s\n", (access(filename, R_OK) == 0) ? "✅ Sì" : "❌ No");
    printf("   Scrivibile: %s\n", (access(filename, W_OK) == 0) ? "✅ Sì" : "❌ No");
    printf("   Eseguibile: %s\n", (access(filename, X_OK) == 0) ? "✅ Sì" : "❌ No");
    printf("   Esiste: %s\n", (access(filename, F_OK) == 0) ? "✅ Sì" : "❌ No");
    
    unlink(filename);
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

void file_copy_example() {
    printf("\n=== COPIA FILE ===\n");
    
    const char* source = "source.txt";
    const char* destination = "destination.txt";
    
    // Crea file sorgente
    int fd = open(source, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd != -1) {
        const char* content = "Questo è il contenuto del file sorgente.\n"
                             "Seconda riga del file.\n"
                             "Terza riga con più testo per test.\n";
        write(fd, content, strlen(content));
        close(fd);
    }
    
    printf("📁 File sorgente '%s' creato\n", source);
    
    // Copia file
    ssize_t copied = copy_file(source, destination);
    
    if (copied > 0) {
        printf("✅ File copiato: %zd bytes da '%s' a '%s'\n", 
               copied, source, destination);
        
        // Verifica copia
        struct stat src_stats, dst_stats;
        stat(source, &src_stats);
        stat(destination, &dst_stats);
        
        printf("📊 Dimensioni: source=%ld, destination=%ld\n", 
               src_stats.st_size, dst_stats.st_size);
        
        if (src_stats.st_size == dst_stats.st_size) {
            printf("✅ Copia verificata correttamente\n");
        } else {
            printf("❌ Errore nella copia\n");
        }
        
    } else {
        printf("❌ Errore copia file\n");
    }
    
    // Cleanup
    unlink(source);
    unlink(destination);
}
```

---

## 📂 Directory Management

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

// Lista contenuto directory
void list_directory_contents(const char* directory_path) {
    printf("=== CONTENUTO DIRECTORY '%s' ===\n", directory_path);
    
    DIR* dir = opendir(directory_path);
    if (dir == NULL) {
        perror("opendir");
        return;
    }
    
    struct dirent* entry;
    struct stat entry_stats;
    char full_path[512];
    int file_count = 0, dir_count = 0;
    
    printf("📋 Lista contenuti:\n");
    
    while ((entry = readdir(dir)) != NULL) {
        // Salta . e ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Costruisci path completo
        snprintf(full_path, sizeof(full_path), "%s/%s", directory_path, entry->d_name);
        
        // Ottieni statistiche
        if (stat(full_path, &entry_stats) == 0) {
            // Icona basata sul tipo
            char type_icon = '?';
            if (S_ISDIR(entry_stats.st_mode)) {
                type_icon = '📁';
                dir_count++;
            } else if (S_ISREG(entry_stats.st_mode)) {
                type_icon = '📄';
                file_count++;
            } else if (S_ISLNK(entry_stats.st_mode)) {
                type_icon = '🔗';
            }
            
            // Formato output
            printf("   %c %-30s %8ld bytes  %s", 
                   type_icon, 
                   entry->d_name,
                   entry_stats.st_size,
                   ctime(&entry_stats.st_mtime));
            
        } else {
            printf("   ❓ %-30s (stat error)\n", entry->d_name);
        }
    }
    
    closedir(dir);
    
    printf("\n📊 Totale: %d file, %d directory\n", file_count, dir_count);
}

// Ricerca file ricorsiva
void find_files_recursive(const char* directory_path, const char* pattern, int depth) {
    DIR* dir;
    struct dirent* entry;
    struct stat entry_stats;
    char full_path[512];
    
    // Limita profondità ricorsione
    if (depth > 10) {
        printf("⚠️  Profondità massima raggiunta\n");
        return;
    }
    
    dir = opendir(directory_path);
    if (dir == NULL) {
        // Non stampare errore per directory senza permessi
        return;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        // Salta . e ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        snprintf(full_path, sizeof(full_path), "%s/%s", directory_path, entry->d_name);
        
        if (stat(full_path, &entry_stats) != 0) {
            continue;
        }
        
        // Se è un file e matcha il pattern
        if (S_ISREG(entry_stats.st_mode)) {
            if (strstr(entry->d_name, pattern) != NULL) {
                printf("🔍 Trovato: %s (%ld bytes)\n", full_path, entry_stats.st_size);
            }
        }
        
        // Se è una directory, ricorsione
        if (S_ISDIR(entry_stats.st_mode)) {
            find_files_recursive(full_path, pattern, depth + 1);
        }
    }
    
    closedir(dir);
}

// Creazione struttura directory
int create_directory_structure(const char* base_path) {
    printf("=== CREAZIONE STRUTTURA DIRECTORY ===\n");
    
    char path[256];
    
    // Crea directory base
    if (mkdir(base_path, 0755) != 0 && errno != EEXIST) {
        perror("mkdir base");
        return -1;
    }
    printf("📁 Creata directory base: %s\n", base_path);
    
    // Crea sottodirectory
    const char* subdirs[] = {"docs", "src", "bin", "tests", "logs"};
    int num_subdirs = sizeof(subdirs) / sizeof(subdirs[0]);
    
    for (int i = 0; i < num_subdirs; i++) {
        snprintf(path, sizeof(path), "%s/%s", base_path, subdirs[i]);
        
        if (mkdir(path, 0755) != 0 && errno != EEXIST) {
            perror("mkdir subdir");
            continue;
        }
        printf("📁 Creata subdirectory: %s\n", path);
    }
    
    // Crea alcuni file di test
    const char* test_files[] = {
        "docs/readme.txt",
        "src/main.c", 
        "src/utils.c",
        "tests/test1.c",
        "logs/app.log"
    };
    int num_files = sizeof(test_files) / sizeof(test_files[0]);
    
    for (int i = 0; i < num_files; i++) {
        snprintf(path, sizeof(path), "%s/%s", base_path, test_files[i]);
        
        int fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd != -1) {
            char content[128];
            snprintf(content, sizeof(content), "File di test: %s\nCreato il: %ld\n", 
                    test_files[i], time(NULL));
            write(fd, content, strlen(content));
            close(fd);
            printf("📄 Creato file: %s\n", path);
        }
    }
    
    return 0;
}

// Rimozione ricorsiva directory
int remove_directory_recursive(const char* directory_path) {
    DIR* dir;
    struct dirent* entry;
    struct stat entry_stats;
    char full_path[512];
    int removed_files = 0, removed_dirs = 0;
    
    dir = opendir(directory_path);
    if (dir == NULL) {
        perror("opendir for removal");
        return -1;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        snprintf(full_path, sizeof(full_path), "%s/%s", directory_path, entry->d_name);
        
        if (stat(full_path, &entry_stats) != 0) {
            continue;
        }
        
        if (S_ISDIR(entry_stats.st_mode)) {
            // Ricorsione per subdirectory
            remove_directory_recursive(full_path);
            
            if (rmdir(full_path) == 0) {
                removed_dirs++;
                printf("🗑️  Rimossa directory: %s\n", full_path);
            }
        } else {
            // Rimuovi file
            if (unlink(full_path) == 0) {
                removed_files++;
                printf("🗑️  Rimosso file: %s\n", full_path);
            }
        }
    }
    
    closedir(dir);
    
    // Rimuovi directory corrente
    if (rmdir(directory_path) == 0) {
        removed_dirs++;
        printf("🗑️  Rimossa directory: %s\n", directory_path);
    }
    
    printf("📊 Rimozione completata: %d file, %d directory\n", removed_files, removed_dirs);
    return 0;
}

// Copia directory ricorsiva
int copy_directory_recursive(const char* source_dir, const char* dest_dir) {
    DIR* dir;
    struct dirent* entry;
    struct stat entry_stats;
    char src_path[512], dst_path[512];
    int copied_files = 0, created_dirs = 0;
    
    // Crea directory destinazione
    if (mkdir(dest_dir, 0755) != 0 && errno != EEXIST) {
        perror("mkdir destination");
        return -1;
    }
    created_dirs++;
    
    dir = opendir(source_dir);
    if (dir == NULL) {
        perror("opendir source");
        return -1;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        snprintf(src_path, sizeof(src_path), "%s/%s", source_dir, entry->d_name);
        snprintf(dst_path, sizeof(dst_path), "%s/%s", dest_dir, entry->d_name);
        
        if (stat(src_path, &entry_stats) != 0) {
            continue;
        }
        
        if (S_ISDIR(entry_stats.st_mode)) {
            // Ricorsione per subdirectory
            int result = copy_directory_recursive(src_path, dst_path);
            if (result > 0) {
                created_dirs += result;
            }
        } else if (S_ISREG(entry_stats.st_mode)) {
            // Copia file
            if (copy_file(src_path, dst_path) > 0) {
                copied_files++;
                printf("📋 Copiato: %s -> %s\n", src_path, dst_path);
            }
        }
    }
    
    closedir(dir);
    
    printf("📊 Copia directory: %d file, %d directory\n", copied_files, created_dirs);
    return created_dirs;
}

// Directory walker con callback
typedef void (*file_callback_t)(const char* filepath, const struct stat* stats, void* userdata);

void walk_directory(const char* directory_path, file_callback_t callback, void* userdata) {
    DIR* dir;
    struct dirent* entry;
    struct stat entry_stats;
    char full_path[512];
    
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
            // Chiama callback per questo file/directory
            callback(full_path, &entry_stats, userdata);
            
            // Ricorsione per directory
            if (S_ISDIR(entry_stats.st_mode)) {
                walk_directory(full_path, callback, userdata);
            }
        }
    }
    
    closedir(dir);
}

// Callback esempio: calcola spazio totale
void calculate_size_callback(const char* filepath, const struct stat* stats, void* userdata) {
    long* total_size = (long*)userdata;
    
    if (S_ISREG(stats->st_mode)) {
        *total_size += stats->st_size;
    }
}

// Callback esempio: conta file per tipo
typedef struct {
    int regular_files;
    int directories;
    int symlinks;
    int other;
} FileTypeCount;

void count_types_callback(const char* filepath, const struct stat* stats, void* userdata) {
    FileTypeCount* counts = (FileTypeCount*)userdata;
    
    if (S_ISREG(stats->st_mode)) {
        counts->regular_files++;
    } else if (S_ISDIR(stats->st_mode)) {
        counts->directories++;
    } else if (S_ISLNK(stats->st_mode)) {
        counts->symlinks++;
    } else {
        counts->other++;
    }
}

// Esempio completo directory management
void directory_management_example() {
    printf("=== ESEMPIO COMPLETO DIRECTORY MANAGEMENT ===\n");
    
    const char* test_dir = "test_project";
    const char* copy_dir = "test_project_copy";
    
    // 1. Crea struttura directory
    create_directory_structure(test_dir);
    
    // 2. Lista contenuto
    printf("\n📋 Lista contenuto directory principale:\n");
    list_directory_contents(test_dir);
    
    // 3. Ricerca file
    printf("\n🔍 Ricerca file '*.c':\n");
    find_files_recursive(test_dir, ".c", 0);
    
    // 4. Calcola spazio totale
    printf("\n📊 Calcolo spazio totale:\n");
    long total_size = 0;
    walk_directory(test_dir, calculate_size_callback, &total_size);
    printf("Spazio totale utilizzato: %ld bytes\n", total_size);
    
    // 5. Conta tipi file
    printf("\n📈 Conteggio tipi file:\n");
    FileTypeCount counts = {0, 0, 0, 0};
    walk_directory(test_dir, count_types_callback, &counts);
    printf("File regolari: %d\n", counts.regular_files);
    printf("Directory: %d\n", counts.directories);
    printf("Symbolic link: %d\n", counts.symlinks);
    printf("Altri: %d\n", counts.other);
    
    // 6. Copia directory
    printf("\n📋 Copia directory:\n");
    copy_directory_recursive(test_dir, copy_dir);
    
    // 7. Verifica copia
    printf("\n✅ Verifica copia:\n");
    list_directory_contents(copy_dir);
    
    // 8. Cleanup
    printf("\n🗑️  Cleanup:\n");
    remove_directory_recursive(test_dir);
    remove_directory_recursive(copy_dir);
    
    printf("✅ Directory management example completato\n");
}
```

---

## 🛠️ Utility File I/O Avanzate

```c
// File mapping in memoria
#include <sys/mman.h>

typedef struct {
    void* mapped_data;
    size_t file_size;
    int fd;
} MappedFile;

MappedFile* map_file_readonly(const char* filename) {
    MappedFile* mf = malloc(sizeof(MappedFile));
    struct stat stats;
    
    mf->fd = open(filename, O_RDONLY);
    if (mf->fd == -1) {
        free(mf);
        return NULL;
    }
    
    if (fstat(mf->fd, &stats) == -1) {
        close(mf->fd);
        free(mf);
        return NULL;
    }
    
    mf->file_size = stats.st_size;
    mf->mapped_data = mmap(NULL, mf->file_size, PROT_READ, MAP_PRIVATE, mf->fd, 0);
    
    if (mf->mapped_data == MAP_FAILED) {
        close(mf->fd);
        free(mf);
        return NULL;
    }
    
    return mf;
}

void unmap_file(MappedFile* mf) {
    if (mf) {
        munmap(mf->mapped_data, mf->file_size);
        close(mf->fd);
        free(mf);
    }
}

// File watcher semplice
typedef struct {
    char filename[256];
    time_t last_modified;
    off_t last_size;
} FileWatcher;

FileWatcher* create_file_watcher(const char* filename) {
    FileWatcher* fw = malloc(sizeof(FileWatcher));
    struct stat stats;
    
    strncpy(fw->filename, filename, sizeof(fw->filename) - 1);
    
    if (stat(filename, &stats) == 0) {
        fw->last_modified = stats.st_mtime;
        fw->last_size = stats.st_size;
    } else {
        fw->last_modified = 0;
        fw->last_size = 0;
    }
    
    return fw;
}

typedef enum {
    FILE_UNCHANGED,
    FILE_MODIFIED,
    FILE_CREATED,
    FILE_DELETED
} FileChangeType;

FileChangeType check_file_changes(FileWatcher* fw) {
    struct stat stats;
    
    if (stat(fw->filename, &stats) == 0) {
        if (fw->last_modified == 0) {
            // File creato
            fw->last_modified = stats.st_mtime;
            fw->last_size = stats.st_size;
            return FILE_CREATED;
        } else if (stats.st_mtime != fw->last_modified || stats.st_size != fw->last_size) {
            // File modificato
            fw->last_modified = stats.st_mtime;
            fw->last_size = stats.st_size;
            return FILE_MODIFIED;
        } else {
            return FILE_UNCHANGED;
        }
    } else {
        if (fw->last_modified != 0) {
            // File cancellato
            fw->last_modified = 0;
            fw->last_size = 0;
            return FILE_DELETED;
        } else {
            return FILE_UNCHANGED;
        }
    }
}

// Lettura file per linee
typedef struct {
    FILE* file;
    char* line_buffer;
    size_t buffer_size;
    int line_number;
} LineReader;

LineReader* create_line_reader(const char* filename) {
    LineReader* lr = malloc(sizeof(LineReader));
    
    lr->file = fopen(filename, "r");
    if (!lr->file) {
        free(lr);
        return NULL;
    }
    
    lr->line_buffer = malloc(1024);
    lr->buffer_size = 1024;
    lr->line_number = 0;
    
    return lr;
}

char* read_next_line(LineReader* lr) {
    if (getline(&lr->line_buffer, &lr->buffer_size, lr->file) != -1) {
        lr->line_number++;
        // Rimuovi newline finale
        lr->line_buffer[strcspn(lr->line_buffer, "\n")] = 0;
        return lr->line_buffer;
    }
    return NULL;
}

void destroy_line_reader(LineReader* lr) {
    if (lr) {
        if (lr->file) fclose(lr->file);
        free(lr->line_buffer);
        free(lr);
    }
}

// File backup con timestamp
int backup_file(const char* filename) {
    char backup_name[512];
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    
    snprintf(backup_name, sizeof(backup_name), "%s.backup_%04d%02d%02d_%02d%02d%02d",
             filename,
             tm_info->tm_year + 1900,
             tm_info->tm_mon + 1,
             tm_info->tm_mday,
             tm_info->tm_hour,
             tm_info->tm_min,
             tm_info->tm_sec);
    
    return copy_file(filename, backup_name);
}

// Atomic file write
int atomic_write_file(const char* filename, const void* data, size_t data_size) {
    char temp_filename[512];
    snprintf(temp_filename, sizeof(temp_filename), "%s.tmp.%d", filename, getpid());
    
    // Scrivi su file temporaneo
    int fd = open(temp_filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd == -1) {
        return -1;
    }
    
    ssize_t written = write(fd, data, data_size);
    if (written != data_size) {
        close(fd);
        unlink(temp_filename);
        return -1;
    }
    
    // Forza scrittura su disco
    if (fsync(fd) == -1) {
        close(fd);
        unlink(temp_filename);
        return -1;
    }
    
    close(fd);
    
    // Rename atomico
    if (rename(temp_filename, filename) == -1) {
        unlink(temp_filename);
        return -1;
    }
    
    return written;
}
```

---

## 📋 Checklist File I/O

### ✅ **System Calls Base**
- [ ] `open()` con flag appropriati (O_RDONLY, O_WRONLY, O_RDWR)
- [ ] Controlla errori su tutte le operazioni
- [ ] `close()` sempre i file descriptor
- [ ] Usa `read()`/`write()` per accesso a basso livello
- [ ] `lseek()` per posizionamento nel file

### ✅ **Directory Operations**
- [ ] `opendir()`/`readdir()`/`closedir()` per listing
- [ ] `mkdir()`/`rmdir()` per gestione directory
- [ ] `stat()`/`fstat()`/`lstat()` per informazioni file
- [ ] Ricorsione per operazioni su alberi directory
- [ ] Gestione permessi con `chmod()`/`chown()`

### ✅ **Best Practices**
- [ ] Controllo errori su ogni system call
- [ ] Cleanup risorse (close FD, free memoria)
- [ ] Gestione permessi e sicurezza
- [ ] Operazioni atomiche per consistency
- [ ] Backup prima di modifiche critiche

---

## 🎯 Compilazione e Test

```bash
# Compila esempi
gcc -o file_io file_io_examples.c

# Test con file grandi
dd if=/dev/zero of=large_file.txt bs=1M count=100

# Test permessi
chmod 755 test_program
chmod 644 data_file.txt

# Monitor I/O
iostat -x 1

# Debug file operations
strace -e trace=file ./file_io

# Test filesystem
df -h
du -sh directory/
```

## 🚀 Performance File I/O

| **Operazione** | **System Call** | **Buffered I/O** |
|----------------|-----------------|-------------------|
| **Overhead** | Basso | Medio |
| **Controllo** | Completo | Limitato |
| **Portabilità** | Unix/Linux | Standard C |
| **Buffer** | Manuale | Automatico |
| **Performance** | Ottimizzabile | Buona di default |