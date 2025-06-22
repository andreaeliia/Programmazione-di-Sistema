# Error Handling e Logging - Riferimento Completo

## 🎯 Concetti Fondamentali

### **Error Handling**
- **Defensive programming**: anticipare e gestire errori
- **Error codes**: valori di ritorno per indicare stato operazione
- **errno**: variabile globale per codici errore sistema
- **Graceful degradation**: continuare funzionamento con funzionalità ridotte
- **Fail-fast vs Fail-safe**: strategie di gestione errori

### **Logging**
- **Audit trail**: traccia delle operazioni per debug/forensics
- **Log levels**: criticità messaggi (DEBUG, INFO, WARN, ERROR, FATAL)
- **Structured logging**: formato consistente per parsing automatico
- **Log rotation**: gestione dimensione e retention log files
- **Syslog**: sistema standard Unix/Linux per logging

### **Componenti Chiave**
- **Error detection**: identificazione condizioni errore
- **Error reporting**: comunicazione errori a utente/sistema
- **Error recovery**: tentativo ripristino stato valido
- **Logging framework**: sistema organizzato per registrazione eventi

---

## 🚨 Error Handling Base

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <stdarg.h>
#include <syslog.h>

// Codici errore personalizzati
typedef enum {
    ERR_SUCCESS = 0,
    ERR_INVALID_PARAM = -1,
    ERR_FILE_NOT_FOUND = -2,
    ERR_PERMISSION_DENIED = -3,
    ERR_OUT_OF_MEMORY = -4,
    ERR_NETWORK_FAILED = -5,
    ERR_TIMEOUT = -6,
    ERR_INVALID_FORMAT = -7,
    ERR_SYSTEM_ERROR = -8
} ErrorCode;

// Struttura per informazioni dettagliate errore
typedef struct {
    ErrorCode code;
    int system_errno;
    char message[256];
    char function[64];
    char file[64];
    int line;
    time_t timestamp;
} ErrorInfo;

// Variabile globale per ultimo errore
static ErrorInfo last_error = {0};

// Macro per catturare informazioni errore
#define SET_ERROR(code, msg, ...) \
    set_error_info(code, __FUNCTION__, __FILE__, __LINE__, msg, ##__VA_ARGS__)

// Imposta informazioni errore dettagliate
void set_error_info(ErrorCode code, const char* function, const char* file, 
                   int line, const char* format, ...) {
    last_error.code = code;
    last_error.system_errno = errno;
    last_error.timestamp = time(NULL);
    
    strncpy(last_error.function, function, sizeof(last_error.function) - 1);
    strncpy(last_error.file, file, sizeof(last_error.file) - 1);
    last_error.line = line;
    
    va_list args;
    va_start(args, format);
    vsnprintf(last_error.message, sizeof(last_error.message), format, args);
    va_end(args);
}

// Ottieni descrizione errore
const char* get_error_string(ErrorCode code) {
    switch (code) {
        case ERR_SUCCESS: return "Success";
        case ERR_INVALID_PARAM: return "Invalid parameter";
        case ERR_FILE_NOT_FOUND: return "File not found";
        case ERR_PERMISSION_DENIED: return "Permission denied";
        case ERR_OUT_OF_MEMORY: return "Out of memory";
        case ERR_NETWORK_FAILED: return "Network operation failed";
        case ERR_TIMEOUT: return "Operation timed out";
        case ERR_INVALID_FORMAT: return "Invalid format";
        case ERR_SYSTEM_ERROR: return "System error";
        default: return "Unknown error";
    }
}

// Stampa informazioni errore complete
void print_error_info() {
    printf("❌ ERROR DETAILS:\n");
    printf("   Code: %d (%s)\n", last_error.code, get_error_string(last_error.code));
    printf("   Message: %s\n", last_error.message);
    printf("   Function: %s\n", last_error.function);
    printf("   File: %s:%d\n", last_error.file, last_error.line);
    printf("   System errno: %d (%s)\n", last_error.system_errno, 
           strerror(last_error.system_errno));
    printf("   Timestamp: %s", ctime(&last_error.timestamp));
}

// Esempio gestione errori base
ErrorCode safe_file_read(const char* filename, char** content, size_t* size) {
    if (!filename || !content || !size) {
        SET_ERROR(ERR_INVALID_PARAM, "NULL parameter provided");
        return ERR_INVALID_PARAM;
    }
    
    // Verifica esistenza file
    struct stat file_stats;
    if (stat(filename, &file_stats) != 0) {
        SET_ERROR(ERR_FILE_NOT_FOUND, "Cannot access file '%s'", filename);
        return ERR_FILE_NOT_FOUND;
    }
    
    // Verifica permessi lettura
    if (access(filename, R_OK) != 0) {
        SET_ERROR(ERR_PERMISSION_DENIED, "No read permission for file '%s'", filename);
        return ERR_PERMISSION_DENIED;
    }
    
    // Alloca memoria per contenuto
    *size = file_stats.st_size;
    *content = malloc(*size + 1);
    if (!*content) {
        SET_ERROR(ERR_OUT_OF_MEMORY, "Cannot allocate %zu bytes for file content", *size);
        return ERR_OUT_OF_MEMORY;
    }
    
    // Apri file
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        SET_ERROR(ERR_SYSTEM_ERROR, "Cannot open file '%s'", filename);
        free(*content);
        *content = NULL;
        return ERR_SYSTEM_ERROR;
    }
    
    // Leggi contenuto
    ssize_t bytes_read = read(fd, *content, *size);
    close(fd);
    
    if (bytes_read != *size) {
        SET_ERROR(ERR_SYSTEM_ERROR, "Read %zd bytes, expected %zu", bytes_read, *size);
        free(*content);
        *content = NULL;
        return ERR_SYSTEM_ERROR;
    }
    
    (*content)[*size] = '\0';  // Null-terminate
    return ERR_SUCCESS;
}

// Retry logic per operazioni che possono fallire temporaneamente
typedef struct {
    int max_retries;
    int base_delay_ms;
    int max_delay_ms;
    double backoff_multiplier;
} RetryConfig;

ErrorCode retry_operation(ErrorCode (*operation)(void* param), void* param, 
                         const RetryConfig* config) {
    ErrorCode result;
    int current_delay = config->base_delay_ms;
    
    for (int retry = 0; retry <= config->max_retries; retry++) {
        result = operation(param);
        
        if (result == ERR_SUCCESS) {
            if (retry > 0) {
                printf("✅ Operation succeeded after %d retries\n", retry);
            }
            return ERR_SUCCESS;
        }
        
        if (retry < config->max_retries) {
            printf("⚠️  Attempt %d failed, retrying in %d ms...\n", retry + 1, current_delay);
            usleep(current_delay * 1000);
            
            current_delay = (int)(current_delay * config->backoff_multiplier);
            if (current_delay > config->max_delay_ms) {
                current_delay = config->max_delay_ms;
            }
        }
    }
    
    SET_ERROR(ERR_TIMEOUT, "Operation failed after %d retries", config->max_retries);
    return ERR_TIMEOUT;
}

// Esempio operazione che può fallire
ErrorCode unreliable_network_operation(void* param) {
    // Simula operazione di rete inaffidabile
    static int attempt_count = 0;
    attempt_count++;
    
    if (attempt_count < 3) {
        SET_ERROR(ERR_NETWORK_FAILED, "Network timeout (attempt %d)", attempt_count);
        return ERR_NETWORK_FAILED;
    }
    
    printf("🌐 Network operation completed successfully\n");
    return ERR_SUCCESS;
}

// Esempio error handling con retry
void error_handling_example() {
    printf("=== ERROR HANDLING EXAMPLE ===\n");
    
    // Test 1: File esistente
    printf("\n📁 Test 1: Lettura file esistente\n");
    
    // Crea file di test
    const char* test_file = "test_content.txt";
    int fd = open(test_file, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd != -1) {
        write(fd, "Contenuto file di test\nSeconda riga\n", 34);
        close(fd);
    }
    
    char* content;
    size_t size;
    ErrorCode result = safe_file_read(test_file, &content, &size);
    
    if (result == ERR_SUCCESS) {
        printf("✅ File letto con successo (%zu bytes):\n%s\n", size, content);
        free(content);
    } else {
        printf("❌ Errore lettura file:\n");
        print_error_info();
    }
    
    unlink(test_file);
    
    // Test 2: File inesistente
    printf("\n📁 Test 2: File inesistente\n");
    result = safe_file_read("file_inesistente.txt", &content, &size);
    if (result != ERR_SUCCESS) {
        printf("❌ Errore atteso per file inesistente:\n");
        print_error_info();
    }
    
    // Test 3: Retry operation
    printf("\n🔄 Test 3: Retry operation\n");
    RetryConfig retry_config = {
        .max_retries = 5,
        .base_delay_ms = 100,
        .max_delay_ms = 2000,
        .backoff_multiplier = 1.5
    };
    
    result = retry_operation(unreliable_network_operation, NULL, &retry_config);
    if (result == ERR_SUCCESS) {
        printf("✅ Retry operation completed successfully\n");
    } else {
        printf("❌ Retry operation failed:\n");
        print_error_info();
    }
}
```

---

## 📝 Sistema di Logging Completo

```c
// Livelli di log
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_ERROR = 3,
    LOG_LEVEL_FATAL = 4
} LogLevel;

// Destinazioni log
typedef enum {
    LOG_DEST_CONSOLE = 1,
    LOG_DEST_FILE = 2,
    LOG_DEST_SYSLOG = 4,
    LOG_DEST_ALL = 7
} LogDestination;

// Configurazione logger
typedef struct {
    LogLevel min_level;
    LogDestination destinations;
    char log_file[256];
    int max_file_size;
    int max_backup_files;
    int enable_timestamps;
    int enable_thread_info;
    int enable_colors;
    pthread_mutex_t mutex;
} LoggerConfig;

// Istanza globale logger
static LoggerConfig g_logger = {
    .min_level = LOG_LEVEL_INFO,
    .destinations = LOG_DEST_CONSOLE,
    .log_file = "application.log",
    .max_file_size = 10 * 1024 * 1024,  // 10MB
    .max_backup_files = 5,
    .enable_timestamps = 1,
    .enable_thread_info = 1,
    .enable_colors = 1,
    .mutex = PTHREAD_MUTEX_INITIALIZER
};

// Inizializza logger
void logger_init(LogLevel min_level, LogDestination destinations, const char* log_file) {
    pthread_mutex_lock(&g_logger.mutex);
    
    g_logger.min_level = min_level;
    g_logger.destinations = destinations;
    
    if (log_file) {
        strncpy(g_logger.log_file, log_file, sizeof(g_logger.log_file) - 1);
    }
    
    // Apri syslog se necessario
    if (destinations & LOG_DEST_SYSLOG) {
        openlog("myapp", LOG_PID | LOG_CONS, LOG_USER);
    }
    
    pthread_mutex_unlock(&g_logger.mutex);
    
    LOG_INFO("Logger initialized - level=%s, destinations=%d, file=%s",
             get_log_level_string(min_level), destinations, log_file ? log_file : "none");
}

// Ottieni stringa livello log
const char* get_log_level_string(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO:  return "INFO";
        case LOG_LEVEL_WARN:  return "WARN";
        case LOG_LEVEL_ERROR: return "ERROR";
        case LOG_LEVEL_FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

// Ottieni colore ANSI per livello
const char* get_log_color(LogLevel level) {
    if (!g_logger.enable_colors) return "";
    
    switch (level) {
        case LOG_LEVEL_DEBUG: return "\033[36m";  // Cyan
        case LOG_LEVEL_INFO:  return "\033[32m";  // Green
        case LOG_LEVEL_WARN:  return "\033[33m";  // Yellow
        case LOG_LEVEL_ERROR: return "\033[31m";  // Red
        case LOG_LEVEL_FATAL: return "\033[35m";  // Magenta
        default: return "\033[0m";
    }
}

// Reset colore ANSI
const char* get_reset_color() {
    return g_logger.enable_colors ? "\033[0m" : "";
}

// Verifica necessità rotazione log file
void check_log_rotation() {
    struct stat file_stat;
    
    if (stat(g_logger.log_file, &file_stat) == 0) {
        if (file_stat.st_size > g_logger.max_file_size) {
            // Rotazione necessaria
            char old_file[300], new_file[300];
            
            // Sposta file esistenti
            for (int i = g_logger.max_backup_files - 1; i > 0; i--) {
                snprintf(old_file, sizeof(old_file), "%s.%d", g_logger.log_file, i - 1);
                snprintf(new_file, sizeof(new_file), "%s.%d", g_logger.log_file, i);
                rename(old_file, new_file);
            }
            
            // Sposta file corrente a .0
            snprintf(new_file, sizeof(new_file), "%s.0", g_logger.log_file);
            rename(g_logger.log_file, new_file);
            
            printf("🔄 Log file rotated\n");
        }
    }
}

// Scrivi nel log file
void write_to_log_file(const char* formatted_message) {
    check_log_rotation();
    
    FILE* file = fopen(g_logger.log_file, "a");
    if (file) {
        fprintf(file, "%s\n", formatted_message);
        fclose(file);
    }
}

// Converte livello custom a syslog
int log_level_to_syslog(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_DEBUG: return LOG_DEBUG;
        case LOG_LEVEL_INFO:  return LOG_INFO;
        case LOG_LEVEL_WARN:  return LOG_WARNING;
        case LOG_LEVEL_ERROR: return LOG_ERR;
        case LOG_LEVEL_FATAL: return LOG_CRIT;
        default: return LOG_INFO;
    }
}

// Funzione principale di logging
void log_message(LogLevel level, const char* file, const char* function, 
                int line, const char* format, ...) {
    
    if (level < g_logger.min_level) {
        return;  // Livello troppo basso
    }
    
    pthread_mutex_lock(&g_logger.mutex);
    
    char message[1024];
    char formatted_message[1200];
    
    // Formatta messaggio utente
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    // Prepara timestamp
    char timestamp[32] = "";
    if (g_logger.enable_timestamps) {
        time_t now = time(NULL);
        struct tm* tm_info = localtime(&now);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    }
    
    // Prepara info thread
    char thread_info[32] = "";
    if (g_logger.enable_thread_info) {
        snprintf(thread_info, sizeof(thread_info), "[PID:%d]", getpid());
    }
    
    // Formatta messaggio completo
    snprintf(formatted_message, sizeof(formatted_message),
             "%s %s [%s] %s %s:%s():%d - %s",
             timestamp,
             thread_info,
             get_log_level_string(level),
             get_log_color(level),
             file,
             function,
             line,
             message);
    
    // Output su console
    if (g_logger.destinations & LOG_DEST_CONSOLE) {
        printf("%s%s\n", formatted_message, get_reset_color());
    }
    
    // Output su file
    if (g_logger.destinations & LOG_DEST_FILE) {
        write_to_log_file(formatted_message);
    }
    
    // Output su syslog
    if (g_logger.destinations & LOG_DEST_SYSLOG) {
        syslog(log_level_to_syslog(level), "%s", message);
    }
    
    pthread_mutex_unlock(&g_logger.mutex);
}

// Macro per logging con informazioni file/linea
#define LOG_DEBUG(fmt, ...) log_message(LOG_LEVEL_DEBUG, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  log_message(LOG_LEVEL_INFO,  __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  log_message(LOG_LEVEL_WARN,  __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) log_message(LOG_LEVEL_ERROR, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...) log_message(LOG_LEVEL_FATAL, __FILE__, __FUNCTION__, __LINE__, fmt, ##__VA_ARGS__)

// Cleanup logger
void logger_cleanup() {
    pthread_mutex_lock(&g_logger.mutex);
    
    if (g_logger.destinations & LOG_DEST_SYSLOG) {
        closelog();
    }
    
    LOG_INFO("Logger shutdown");
    pthread_mutex_unlock(&g_logger.mutex);
}

// Esempio utilizzo sistema di logging
void logging_system_example() {
    printf("=== SISTEMA DI LOGGING EXAMPLE ===\n");
    
    // Inizializza logger
    logger_init(LOG_LEVEL_DEBUG, LOG_DEST_CONSOLE | LOG_DEST_FILE, "test_app.log");
    
    // Test diversi livelli di log
    LOG_DEBUG("Questo è un messaggio di debug con valore: %d", 42);
    LOG_INFO("Applicazione avviata con successo");
    LOG_WARN("Attenzione: memoria al 85%% di utilizzo");
    LOG_ERROR("Errore di connessione al database: %s", "Connection timeout");
    LOG_FATAL("Errore critico: impossibile continuare l'esecuzione");
    
    // Test con diversi parametri
    LOG_INFO("Elaborazione file '%s' completata in %.2f secondi", "data.txt", 3.14);
    LOG_WARN("Tentativo di accesso negato per utente '%s' da IP %s", "admin", "192.168.1.100");
    
    // Simula operazione con molti log
    for (int i = 1; i <= 5; i++) {
        LOG_DEBUG("Elaborazione elemento %d/5", i);
        usleep(100000);  // 100ms
    }
    
    LOG_INFO("Elaborazione batch completata");
    
    // Test cambio livello runtime
    printf("\n🔧 Cambio livello log a ERROR (i DEBUG/INFO/WARN non verranno mostrati)\n");
    g_logger.min_level = LOG_LEVEL_ERROR;
    
    LOG_DEBUG("Questo debug NON verrà mostrato");
    LOG_INFO("Questo info NON verrà mostrato");
    LOG_WARN("Questo warning NON verrà mostrato");
    LOG_ERROR("Questo errore VERRÀ mostrato");
    
    // Ripristina livello
    g_logger.min_level = LOG_LEVEL_DEBUG;
    LOG_INFO("Livello log ripristinato");
    
    logger_cleanup();
}
```

---

## 🛡️ Error Recovery e Resilienza

```c
// Struttura per gestione stato applicazione
typedef struct {
    int is_running;
    int error_count;
    time_t last_error_time;
    char last_error_message[256];
    pthread_mutex_t state_mutex;
} ApplicationState;

static ApplicationState app_state = {
    .is_running = 1,
    .error_count = 0,
    .state_mutex = PTHREAD_MUTEX_INITIALIZER
};

// Circuit breaker pattern
typedef struct {
    int failure_threshold;
    int success_threshold;
    int timeout_ms;
    int failure_count;
    int success_count;
    time_t last_failure_time;
    enum {
        CIRCUIT_CLOSED,    // Normale operazione
        CIRCUIT_OPEN,      // Blocca chiamate per timeout
        CIRCUIT_HALF_OPEN  // Test per riapertura
    } state;
    pthread_mutex_t mutex;
} CircuitBreaker;

CircuitBreaker* create_circuit_breaker(int failure_threshold, int success_threshold, int timeout_ms) {
    CircuitBreaker* cb = malloc(sizeof(CircuitBreaker));
    
    cb->failure_threshold = failure_threshold;
    cb->success_threshold = success_threshold;
    cb->timeout_ms = timeout_ms;
    cb->failure_count = 0;
    cb->success_count = 0;
    cb->last_failure_time = 0;
    cb->state = CIRCUIT_CLOSED;
    
    pthread_mutex_init(&cb->mutex, NULL);
    
    return cb;
}

ErrorCode circuit_breaker_call(CircuitBreaker* cb, ErrorCode (*operation)(void*), void* param) {
    pthread_mutex_lock(&cb->mutex);
    
    time_t now = time(NULL);
    
    // Verifica stato circuit breaker
    if (cb->state == CIRCUIT_OPEN) {
        if ((now - cb->last_failure_time) * 1000 > cb->timeout_ms) {
            cb->state = CIRCUIT_HALF_OPEN;
            cb->success_count = 0;
            LOG_INFO("Circuit breaker transitioning to HALF_OPEN");
        } else {
            pthread_mutex_unlock(&cb->mutex);
            LOG_WARN("Circuit breaker is OPEN - operation blocked");
            SET_ERROR(ERR_SYSTEM_ERROR, "Circuit breaker is open");
            return ERR_SYSTEM_ERROR;
        }
    }
    
    pthread_mutex_unlock(&cb->mutex);
    
    // Esegui operazione
    ErrorCode result = operation(param);
    
    pthread_mutex_lock(&cb->mutex);
    
    if (result == ERR_SUCCESS) {
        cb->success_count++;
        cb->failure_count = 0;
        
        if (cb->state == CIRCUIT_HALF_OPEN && cb->success_count >= cb->success_threshold) {
            cb->state = CIRCUIT_CLOSED;
            LOG_INFO("Circuit breaker transitioning to CLOSED");
        }
        
    } else {
        cb->failure_count++;
        cb->success_count = 0;
        cb->last_failure_time = now;
        
        if (cb->state == CIRCUIT_CLOSED && cb->failure_count >= cb->failure_threshold) {
            cb->state = CIRCUIT_OPEN;
            LOG_ERROR("Circuit breaker transitioning to OPEN after %d failures", cb->failure_count);
        } else if (cb->state == CIRCUIT_HALF_OPEN) {
            cb->state = CIRCUIT_OPEN;
            LOG_ERROR("Circuit breaker returning to OPEN from HALF_OPEN");
        }
    }
    
    pthread_mutex_unlock(&cb->mutex);
    
    return result;
}

// Graceful shutdown handler
void graceful_shutdown_handler(int signal) {
    LOG_WARN("Received signal %d - initiating graceful shutdown", signal);
    
    pthread_mutex_lock(&app_state.state_mutex);
    app_state.is_running = 0;
    pthread_mutex_unlock(&app_state.state_mutex);
}

// Health check system
typedef struct {
    char name[64];
    int (*check_function)(void);
    int is_critical;
    time_t last_check;
    int last_result;
} HealthCheck;

typedef struct {
    HealthCheck* checks;
    int num_checks;
    int overall_health;
    pthread_mutex_t mutex;
} HealthMonitor;

int memory_health_check() {
    // Simula controllo memoria
    FILE* meminfo = fopen("/proc/meminfo", "r");
    if (!meminfo) return 0;
    
    char line[256];
    long total_mem = 0, available_mem = 0;
    
    while (fgets(line, sizeof(line), meminfo)) {
        if (sscanf(line, "MemTotal: %ld kB", &total_mem) == 1) continue;
        if (sscanf(line, "MemAvailable: %ld kB", &available_mem) == 1) break;
    }
    
    fclose(meminfo);
    
    if (total_mem > 0 && available_mem > 0) {
        float usage_percent = (float)(total_mem - available_mem) / total_mem * 100;
        return usage_percent < 90.0 ? 1 : 0;  // Healthy se < 90%
    }
    
    return 1;  // Default healthy se non possiamo determinare
}

int disk_health_check() {
    // Simula controllo spazio disco
    struct statvfs disk_stat;
    if (statvfs("/", &disk_stat) == 0) {
        double usage = (double)(disk_stat.f_blocks - disk_stat.f_available) / disk_stat.f_blocks * 100;
        return usage < 95.0 ? 1 : 0;  // Healthy se < 95%
    }
    return 1;
}

int network_health_check() {
    // Simula controllo rete (ping localhost)
    int result = system("ping -c 1 -W 1 localhost >/dev/null 2>&1");
    return WEXITSTATUS(result) == 0 ? 1 : 0;
}

HealthMonitor* create_health_monitor() {
    HealthMonitor* monitor = malloc(sizeof(HealthMonitor));
    
    monitor->num_checks = 3;
    monitor->checks = malloc(sizeof(HealthCheck) * monitor->num_checks);
    monitor->overall_health = 1;
    
    // Configura health check
    strcpy(monitor->checks[0].name, "Memory");
    monitor->checks[0].check_function = memory_health_check;
    monitor->checks[0].is_critical = 1;
    
    strcpy(monitor->checks[1].name, "Disk");
    monitor->checks[1].check_function = disk_health_check;
    monitor->checks[1].is_critical = 1;
    
    strcpy(monitor->checks[2].name, "Network");
    monitor->checks[2].check_function = network_health_check;
    monitor->checks[2].is_critical = 0;
    
    pthread_mutex_init(&monitor->mutex, NULL);
    
    return monitor;
}

void run_health_checks(HealthMonitor* monitor) {
    pthread_mutex_lock(&monitor->mutex);
    
    int overall_health = 1;
    time_t now = time(NULL);
    
    LOG_INFO("Running health checks...");
    
    for (int i = 0; i < monitor->num_checks; i++) {
        HealthCheck* check = &monitor->checks[i];
        
        check->last_result = check->check_function();
        check->last_check = now;
        
        if (check->last_result) {
            LOG_DEBUG("Health check '%s': PASS", check->name);
        } else {
            LOG_WARN("Health check '%s': FAIL", check->name);
            if (check->is_critical) {
                overall_health = 0;
            }
        }
    }
    
    monitor->overall_health = overall_health;
    
    if (overall_health) {
        LOG_INFO("Overall health: HEALTHY");
    } else {
        LOG_ERROR("Overall health: UNHEALTHY");
    }
    
    pthread_mutex_unlock(&monitor->mutex);
}

// Watchdog timer
typedef struct {
    int timeout_seconds;
    time_t last_ping;
    int enabled;
    pthread_t watchdog_thread;
    pthread_mutex_t mutex;
} Watchdog;

void* watchdog_thread_func(void* arg) {
    Watchdog* wd = (Watchdog*)arg;
    
    while (wd->enabled) {
        sleep(1);
        
        pthread_mutex_lock(&wd->mutex);
        
        if (wd->enabled) {
            time_t now = time(NULL);
            if (now - wd->last_ping > wd->timeout_seconds) {
                LOG_FATAL("Watchdog timeout exceeded (%d seconds) - application may be hung", 
                         wd->timeout_seconds);
                
                // In un sistema reale, potresti:
                // - Inviare SIGTERM al processo
                // - Riavviare componenti
                // - Alertare amministratori
                pthread_mutex_unlock(&wd->mutex);
                break;
            }
        }
        
        pthread_mutex_unlock(&wd->mutex);
    }
    
    return NULL;
}

Watchdog* create_watchdog(int timeout_seconds) {
    Watchdog* wd = malloc(sizeof(Watchdog));
    
    wd->timeout_seconds = timeout_seconds;
    wd->last_ping = time(NULL);
    wd->enabled = 1;
    
    pthread_mutex_init(&wd->mutex, NULL);
    pthread_create(&wd->watchdog_thread, NULL, watchdog_thread_func, wd);
    
    LOG_INFO("Watchdog started with %d second timeout", timeout_seconds);
    
    return wd;
}

void watchdog_ping(Watchdog* wd) {
    pthread_mutex_lock(&wd->mutex);
    wd->last_ping = time(NULL);
    pthread_mutex_unlock(&wd->mutex);
}

// Esempio completo error recovery
void error_recovery_example() {
    printf("=== ERROR RECOVERY E RESILIENZA EXAMPLE ===\n");
    
    // Setup signal handler per graceful shutdown
    signal(SIGINT, graceful_shutdown_handler);
    signal(SIGTERM, graceful_shutdown_handler);
    
    // Crea circuit breaker
    CircuitBreaker* cb = create_circuit_breaker(3, 2, 5000);  // 3 fail, 2 success, 5s timeout
    
    // Crea health monitor
    HealthMonitor* health = create_health_monitor();
    
    // Crea watchdog
    Watchdog* watchdog = create_watchdog(10);  // 10 secondi timeout
    
    LOG_INFO("Application started with error recovery features");
    
    // Simula loop principale applicazione
    int iteration = 0;
    while (app_state.is_running && iteration < 20) {
        iteration++;
        
        // Ping watchdog
        watchdog_ping(watchdog);
        
        // Health check ogni 5 iterazioni
        if (iteration % 5 == 0) {
            run_health_checks(health);
        }
        
        // Simula operazione che può fallire
        ErrorCode result = circuit_breaker_call(cb, unreliable_network_operation, NULL);
        
        if (result != ERR_SUCCESS) {
            LOG_ERROR("Operation failed in iteration %d", iteration);
            
            pthread_mutex_lock(&app_state.state_mutex);
            app_state.error_count++;
            app_state.last_error_time = time(NULL);
            snprintf(app_state.last_error_message, sizeof(app_state.last_error_message),
                    "Network operation failed in iteration %d", iteration);
            pthread_mutex_unlock(&app_state.state_mutex);
        } else {
            LOG_INFO("Operation succeeded in iteration %d", iteration);
        }
        
        sleep(1);
    }
    
    LOG_INFO("Application shutting down gracefully");
    
    // Cleanup
    pthread_mutex_lock(&watchdog->mutex);
    watchdog->enabled = 0;
    pthread_mutex_unlock(&watchdog->mutex);
    
    pthread_join(watchdog->watchdog_thread, NULL);
    
    free(cb);
    free(health->checks);
    free(health);
    free(watchdog);
    
    LOG_INFO("Cleanup completed");
}
```

---

## 🛠️ Utility Error Handling

```c
// Stack trace semplice (richiede -rdynamic e -ldl)
#include <execinfo.h>
#include <dlfcn.h>

void print_stack_trace() {
    void *array[20];
    size_t size;
    char **strings;
    
    size = backtrace(array, 20);
    strings = backtrace_symbols(array, size);
    
    printf("Stack trace (%zd frames):\n", size);
    for (size_t i = 0; i < size; i++) {
        printf("  [%zu] %s\n", i, strings[i]);
    }
    
    free(strings);
}

// Memory leak detector semplice
typedef struct {
    void* ptr;
    size_t size;
    const char* file;
    int line;
    time_t alloc_time;
} MemoryAllocation;

static MemoryAllocation allocations[1000];
static int allocation_count = 0;
static pthread_mutex_t alloc_mutex = PTHREAD_MUTEX_INITIALIZER;

void* debug_malloc(size_t size, const char* file, int line) {
    void* ptr = malloc(size);
    
    if (ptr) {
        pthread_mutex_lock(&alloc_mutex);
        if (allocation_count < 1000) {
            allocations[allocation_count].ptr = ptr;
            allocations[allocation_count].size = size;
            allocations[allocation_count].file = file;
            allocations[allocation_count].line = line;
            allocations[allocation_count].alloc_time = time(NULL);
            allocation_count++;
        }
        pthread_mutex_unlock(&alloc_mutex);
    }
    
    return ptr;
}

void debug_free(void* ptr, const char* file, int line) {
    if (!ptr) return;
    
    pthread_mutex_lock(&alloc_mutex);
    for (int i = 0; i < allocation_count; i++) {
        if (allocations[i].ptr == ptr) {
            // Sposta ultimo elemento
            allocations[i] = allocations[allocation_count - 1];
            allocation_count--;
            break;
        }
    }
    pthread_mutex_unlock(&alloc_mutex);
    
    free(ptr);
}

void check_memory_leaks() {
    pthread_mutex_lock(&alloc_mutex);
    
    if (allocation_count > 0) {
        printf("🚨 MEMORY LEAKS DETECTED:\n");
        for (int i = 0; i < allocation_count; i++) {
            printf("  Leak: %zu bytes at %p (allocated in %s:%d)\n",
                   allocations[i].size, allocations[i].ptr,
                   allocations[i].file, allocations[i].line);
        }
    } else {
        printf("✅ No memory leaks detected\n");
    }
    
    pthread_mutex_unlock(&alloc_mutex);
}

#define MALLOC(size) debug_malloc(size, __FILE__, __LINE__)
#define FREE(ptr) debug_free(ptr, __FILE__, __LINE__)

// Assert personalizzato con logging
#define ASSERT(condition, message, ...) \
    do { \
        if (!(condition)) { \
            LOG_FATAL("ASSERTION FAILED: " message, ##__VA_ARGS__); \
            print_stack_trace(); \
            abort(); \
        } \
    } while(0)

// Error context stack
typedef struct {
    char context[64];
    char operation[64];
    void* data;
} ErrorContext;

static ErrorContext error_stack[10];
static int error_stack_depth = 0;
static pthread_mutex_t error_stack_mutex = PTHREAD_MUTEX_INITIALIZER;

void push_error_context(const char* context, const char* operation, void* data) {
    pthread_mutex_lock(&error_stack_mutex);
    
    if (error_stack_depth < 10) {
        strncpy(error_stack[error_stack_depth].context, context, 63);
        strncpy(error_stack[error_stack_depth].operation, operation, 63);
        error_stack[error_stack_depth].data = data;
        error_stack_depth++;
    }
    
    pthread_mutex_unlock(&error_stack_mutex);
}

void pop_error_context() {
    pthread_mutex_lock(&error_stack_mutex);
    
    if (error_stack_depth > 0) {
        error_stack_depth--;
    }
    
    pthread_mutex_unlock(&error_stack_mutex);
}

void print_error_context() {
    pthread_mutex_lock(&error_stack_mutex);
    
    if (error_stack_depth > 0) {
        printf("Error context stack:\n");
        for (int i = error_stack_depth - 1; i >= 0; i--) {
            printf("  %d: %s -> %s\n", i, 
                   error_stack[i].context, 
                   error_stack[i].operation);
        }
    }
    
    pthread_mutex_unlock(&error_stack_mutex);
}

#define ERROR_CONTEXT_PUSH(ctx, op, data) push_error_context(ctx, op, data)
#define ERROR_CONTEXT_POP() pop_error_context()

// Performance monitoring integrato
typedef struct {
    clock_t start_time;
    char operation[64];
} PerformanceTimer;

PerformanceTimer* start_performance_timer(const char* operation) {
    PerformanceTimer* timer = malloc(sizeof(PerformanceTimer));
    timer->start_time = clock();
    strncpy(timer->operation, operation, sizeof(timer->operation) - 1);
    return timer;
}

void end_performance_timer(PerformanceTimer* timer) {
    if (timer) {
        clock_t end_time = clock();
        double elapsed = ((double)(end_time - timer->start_time)) / CLOCKS_PER_SEC;
        
        if (elapsed > 1.0) {
            LOG_WARN("Performance: %s took %.3f seconds (slow)", timer->operation, elapsed);
        } else {
            LOG_DEBUG("Performance: %s took %.3f seconds", timer->operation, elapsed);
        }
        
        free(timer);
    }
}

#define PERF_TIMER_START(op) PerformanceTimer* _timer = start_performance_timer(op)
#define PERF_TIMER_END() end_performance_timer(_timer)
```

---

## 📋 Checklist Error Handling

### ✅ **Error Detection**
- [ ] Controlla valori di ritorno di tutte le system call
- [ ] Valida parametri input funzioni
- [ ] Usa assert per condizioni che non dovrebbero mai verificarsi
- [ ] Controlla errno dopo operazioni che possono fallire
- [ ] Implementa timeout per operazioni bloccanti

### ✅ **Error Reporting**
- [ ] Usa logging strutturato con livelli appropriati
- [ ] Include context informativo nei messaggi errore
- [ ] Propaga errori attraverso call stack
- [ ] Documenta condizioni errore nelle API
- [ ] Fornisci error code machine-readable

### ✅ **Error Recovery**
- [ ] Implementa retry logic per errori temporanei
- [ ] Usa circuit breaker per dipendenze instabili
- [ ] Graceful degradation quando possibile
- [ ] Cleanup risorse in caso errore
- [ ] Health check per monitoraggio stato

### ✅ **Logging Best Practices**
- [ ] Livelli appropriati (DEBUG/INFO/WARN/ERROR/FATAL)
- [ ] Timestamp e context info
- [ ] Log rotation per gestione dimensioni
- [ ] Structured logging per parsing automatico
- [ ] Performance monitoring integrato

---

## 🎯 Compilazione e Test

```bash
# Compila con debug symbols
gcc -g -rdynamic -o error_test error_handling.c -lpthread -ldl

# Test memory leaks con valgrind
valgrind --leak-check=full --show-leak-kinds=all ./error_test

# Test con sanitizers
gcc -fsanitize=address,undefined -g -o error_test error_handling.c -lpthread

# Monitor log files
tail -f application.log

# Test signal handling
./error_test &
kill -TERM $!

# Stress test error conditions
for i in {1..100}; do ./error_test; done
```

## 🚀 Error Handling Patterns

| **Pattern** | **Uso** | **Vantaggi** |
|-------------|---------|--------------|
| **Return codes** | System call, API functions | Semplice, performance |
| **Exceptions** | C++ (non C), high-level errors | Pulito, automatic propagation |
| **Error structs** | Complex error info | Dettagli completi |
| **Callbacks** | Async operations | Non-blocking |
| **Global errno** | POSIX compatibility | Standard, thread-local |