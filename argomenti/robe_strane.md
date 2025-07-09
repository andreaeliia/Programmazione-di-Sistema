SETTARE IL TERMINALE IN MODALITA RAW (non c'e' bisogno di invio)

/* Funzione per impostare il terminale in modalità raw */

void set_raw_mode(struct termios *orig_termios) {
    struct termios raw;
    
    tcgetattr(STDIN_FILENO, orig_termios);
    raw = *orig_termios;
    
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}




/* Funzione per ripristinare il terminale */
void restore_terminal(struct termios *orig_termios) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, orig_termios);
}