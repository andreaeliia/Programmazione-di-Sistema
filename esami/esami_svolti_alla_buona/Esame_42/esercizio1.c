#include "apue.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

/* Struttura per memorizzare i risultati del test */
struct stat_test_result {
    char field_name[32];
    int supported;
    char value_str[64];
};

/* Prototipi delle funzioni */
static int create_test_socket(char *socket_path);
static void test_stat_fields(const char *socket_path, struct stat_test_result *results, int *num_results);
static void print_test_results(struct stat_test_result *results, int num_results);
static void cleanup_socket(const char *socket_path);

/*
 * Crea un socket UNIX per il test
 * Restituisce 0 in caso di successo, -1 in caso di errore
 */
static int
create_test_socket(char *socket_path)
{
    int sockfd;
    struct sockaddr_un addr;
    
    /* Crea il socket */
    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        err_ret("socket error");
        return -1;
    }
    
    /* Configura l'indirizzo */
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    
    /* Rimuove il socket se esiste già */
    unlink(socket_path);
    
    /* Bind del socket */
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        err_ret("bind error");
        close(sockfd);
        return -1;
    }
    
    close(sockfd);
    return 0;
}

/*
 * Testa i vari campi della struttura stat per il socket
 */
static void
test_stat_fields(const char *socket_path, struct stat_test_result *results, int *num_results)
{
    struct stat st;
    int idx = 0;
    
    if (stat(socket_path, &st) < 0) {
        err_ret("stat error for %s", socket_path);
        *num_results = 0;
        return;
    }
    
    /* Test st_mode */
    strcpy(results[idx].field_name, "st_mode");
    results[idx].supported = 1;
    sprintf(results[idx].value_str, "0%o (S_ISSOCK=%s)", 
            (unsigned int)st.st_mode, 
            S_ISSOCK(st.st_mode) ? "YES" : "NO");
    idx++;
    
    /* Test st_ino */
    strcpy(results[idx].field_name, "st_ino");
    results[idx].supported = (st.st_ino != 0);
    sprintf(results[idx].value_str, "%lu", (unsigned long)st.st_ino);
    idx++;
    
    /* Test st_dev */
    strcpy(results[idx].field_name, "st_dev");
    results[idx].supported = 1;
    sprintf(results[idx].value_str, "%lu", (unsigned long)st.st_dev);
    idx++;
    
    /* Test st_rdev */
    strcpy(results[idx].field_name, "st_rdev");
    results[idx].supported = (st.st_rdev != 0);
    sprintf(results[idx].value_str, "%lu", (unsigned long)st.st_rdev);
    idx++;
    
    /* Test st_nlink */
    strcpy(results[idx].field_name, "st_nlink");
    results[idx].supported = 1;
    sprintf(results[idx].value_str, "%lu", (unsigned long)st.st_nlink);
    idx++;
    
    /* Test st_uid */
    strcpy(results[idx].field_name, "st_uid");
    results[idx].supported = 1;
    sprintf(results[idx].value_str, "%u", (unsigned int)st.st_uid);
    idx++;
    
    /* Test st_gid */
    strcpy(results[idx].field_name, "st_gid");
    results[idx].supported = 1;
    sprintf(results[idx].value_str, "%u", (unsigned int)st.st_gid);
    idx++;
    
    /* Test st_size */
    strcpy(results[idx].field_name, "st_size");
    results[idx].supported = 1;
    sprintf(results[idx].value_str, "%ld", (long)st.st_size);
    idx++;
    
    /* Test st_atime */
    strcpy(results[idx].field_name, "st_atime");
    results[idx].supported = (st.st_atime != 0);
    sprintf(results[idx].value_str, "%ld", (long)st.st_atime);
    idx++;
    
    /* Test st_mtime */
    strcpy(results[idx].field_name, "st_mtime");
    results[idx].supported = (st.st_mtime != 0);
    sprintf(results[idx].value_str, "%ld", (long)st.st_mtime);
    idx++;
    
    /* Test st_ctime */
    strcpy(results[idx].field_name, "st_ctime");
    results[idx].supported = (st.st_ctime != 0);
    sprintf(results[idx].value_str, "%ld", (long)st.st_ctime);
    idx++;
    
    /* Test st_blksize */
#ifdef LINUX
    strcpy(results[idx].field_name, "st_blksize");
    results[idx].supported = (st.st_blksize != 0);
    sprintf(results[idx].value_str, "%ld", (long)st.st_blksize);
    idx++;
#endif
    
    /* Test st_blocks */
#ifdef LINUX
    strcpy(results[idx].field_name, "st_blocks");
    results[idx].supported = 1;
    sprintf(results[idx].value_str, "%ld", (long)st.st_blocks);
    idx++;
#endif
    
    *num_results = idx;
}

/*
 * Stampa i risultati del test
 */
static void
print_test_results(struct stat_test_result *results, int num_results)
{
    int i;
    
    printf("\n=== SOCKET STAT FIELD ANALYSIS ===\n");
    
#ifdef LINUX
    printf("Platform: GNU/Linux\n");
#elif defined(MACOS)
    printf("Platform: macOS\n");
#else
    printf("Platform: Unknown\n");
#endif
    
    printf("%-15s %-10s %s\n", "Field", "Supported", "Value");
    printf("%-15s %-10s %s\n", "-----", "---------", "-----");
    
    for (i = 0; i < num_results; i++) {
        printf("%-15s %-10s %s\n", 
               results[i].field_name,
               results[i].supported ? "YES" : "NO",
               results[i].value_str);
    }
    
    printf("\n");
}

/*
 * Rimuove il socket di test
 */
static void
cleanup_socket(const char *socket_path)
{
    if (unlink(socket_path) < 0 && errno != ENOENT) {
        err_ret("unlink error for %s", socket_path);
    }
}

int
main(void)
{
    char socket_path[] = "/tmp/test_socket";
    struct stat_test_result results[20];
    int num_results;
    
    printf("Socket stat() field support test\n");
    printf("Creating test socket: %s\n", socket_path);
    
    /* Crea il socket di test */
    if (create_test_socket(socket_path) < 0) {
        err_sys("Failed to create test socket");
    }
    
    /* Testa i campi della struttura stat */
    test_stat_fields(socket_path, results, &num_results);
    
    /* Stampa i risultati */
    print_test_results(results, num_results);
    
    /* Pulizia */
    cleanup_socket(socket_path);
    
    printf("Platform-specific notes:\n");
    printf("- Linux: Provides st_blksize and st_blocks fields\n");
    printf("- macOS: May have different behavior for st_rdev\n");
    printf("- Both: Support basic fields (mode, ino, dev, uid, gid, times)\n");
    
    exit(0);
}