HARD LINK FINDER - README
DESCRIZIONE
Programma C che utilizza due thread per trovare file con hard link multipli
in una gerarchia di directory specificata.

COMPILAZIONE
Per compilare il programma utilizzare il Makefile fornito:

Aggiungere "hardlink_finder" alla variabile PROGS nel Makefile: PROGS = hardlink_finder
Compilare con: make hardlink_finder
Il programma e' compatibile sia con Linux che con macOS grazie alle flag
del compilatore definite nel Makefile.

UTILIZZO
./hardlink_finder <percorso_directory>

Esempio:
./hardlink_finder /home/utente/documenti

FUNZIONAMENTO
Il programma crea due thread che lavorano in modo sincrono:

THREAD 1:

Attraversa ricorsivamente la directory specificata
Identifica tutti i file regolari con link count > 1
Memorizza percorso e inode di ogni file trovato
Segnala al Thread 2 quando trova un nuovo file
THREAD 2:

Attende i file identificati dal Thread 1
Per ogni file ricevuto, cerca tutti gli altri file nella stessa gerarchia che condividono lo stesso inode (hard link)
Stampa tutti gli hard link trovati
SINCRONIZZAZIONE
La sincronizzazione tra i thread e' gestita tramite:

Mutex per l'accesso esclusivo alle strutture dati condivise
Condition variable per la comunicazione tra thread
Flag per indicare quando Thread 1 ha completato la ricerca
STRUTTURE DATI
Il programma utilizza una struttura condivisa che contiene:

Array di percorsi dei file con link count > 1
Array degli inode corrispondenti
Contatore dei file trovati
Flag di completamento Thread 1
Primitives di sincronizzazione (mutex e condition variable)
Directory di ricerca
OUTPUT
Il programma stampa:

File trovati dal Thread 1 con il loro link count e inode
Hard link trovati dal Thread 2 per ogni inode
Messaggi di stato e progresso
LIMITAZIONI
Massimo 1000 file con hard link multipli (MAX_FILES)
Percorsi massimi di 4096 caratteri (MAX_PATH)
Solo file regolari vengono considerati (non link simbolici)
GESTIONE ERRORI
Il programma utilizza le funzioni di gestione errori della libreria APUE:

err_sys() per errori di sistema
err_msg() per messaggi di errore non fatali
err_quit() per errori fatali
REQUISITI
Libreria APUE (Advanced Programming in the UNIX Environment)
Supporto pthread
Sistema UNIX-like (Linux/macOS)
Compilatore C compatibile con standard C90 (ANSI C)
NOTE TECNICHE
Utilizza lstat() per evitare di seguire i link simbolici
Attraversamento ricorsivo delle directory
Thread-safe tramite mutex e condition variable
Gestione corretta della memoria e cleanup delle risorse
