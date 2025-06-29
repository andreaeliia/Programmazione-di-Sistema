#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

const char *lock_file = "/tmp/LCK.test2";

int main() {
    int file_desc;
    int tries = 10;

    while (tries--) {
	/*se il file viene creato da me va tutto bene, tuttavia se un altro tenta
	di creare lo stesso file che è già esistente, allora incontrerà dei problemi.
	Quindi l'altro utente se vuole creare lo stesso file lo deve prima rimuovere
	e lo farà dopo con unlink*/
        file_desc = open(lock_file, O_RDWR | O_CREAT | O_EXCL, 0444);
        if (file_desc == -1) {
            printf("%d - Lock already present\n", getpid());
            sleep(3);
        }
        else {
                /* critical region */
            printf("%d - I have exclusive access\n", getpid());
            sleep(1);
            (void)close(file_desc);
            (void)unlink(lock_file);
                /* non-critical region */
            sleep(2);
        }
    } /* while */
    exit(EXIT_SUCCESS);
}
