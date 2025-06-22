//Creare un programma che diventa un daemon e dimostra di funzionare scrivendo in un file ogni secondo.//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <string.h>

volatile sig_atomic_t daemon_running = 1;

void signal_handler(int sig) {
    daemon_running = 0;
}

void daemon_log(const char *message) {
    FILE *log = fopen("/tmp/daemon.log", "a");
    if (log) {
        time_t now = time(NULL);
        char *timestr = ctime(&now);
        timestr[strlen(timestr)-1] = '\0';
        fprintf(log, "[%s] %s\n", timestr, message);
        fflush(log);
        fclose(log);
    }
}

void become_daemon() {
    pid_t pid = fork();
    if (pid > 0) {
        printf("Daemon PID: %d\n", pid);
        exit(0);
    }
    if (pid < 0) {
        perror("fork");
        exit(1);
    }
    
    if (setsid() < 0) {
        perror("setsid");
        exit(1);
    }
    
    chdir("/");
    
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_WRONLY);
}

int main() {
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    
    become_daemon();
    
    daemon_log("Daemon avviato");
    
    while (daemon_running) {
        // Lavoro del daemon qui

        while(1){
        daemon_log("Messaggio test");
        sleep(1);
        }
    }
    
    daemon_log("Daemon terminato");
    return 0;
}