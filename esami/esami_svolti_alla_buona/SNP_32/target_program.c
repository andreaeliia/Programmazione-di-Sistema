/*
 * Target program per l'esercizio 2
 * Questo programma stampa l'indirizzo della funzione printf() e termina
 * 
 * Autore: Generato automaticamente
 * Compilazione: make target_program
 */

#include "apue.h"

/*
 * Funzione principale del programma target
 * Stampa l'indirizzo della funzione printf() in formato esadecimale
 */
int main(void)
{
    /* Ottiene e stampa l'indirizzo della funzione printf */
    printf("%lx\n", (unsigned long)&printf);
    
    return 0;
}