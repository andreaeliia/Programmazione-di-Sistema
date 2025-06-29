#include <stdio.h>
#include <signal.h>
#include <unistd.h>

volatile long int conteggio = 0;

void stop_timer(int signum) {
    printf("Tempo scaduto! Hai effettuato %ld moltiplicazioni.\n", conteggio);
    _exit(0);
}

int main() {
    int a, b, risultato;

    printf("Inserisci due numeri da moltiplicare (es: 5 6): ");
    scanf("%d %d", &a, &b);

    signal(SIGALRM, stop_timer);

    alarm(10);

     while (1) {
        risultato = a * b;
        conteggio++;
    }

    return 0;
}



