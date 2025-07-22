================================================================================
                    SISTEMA DISTRIBUITO MASTER-SLAVE CON MULTICAST
================================================================================

DESCRIZIONE:
-----------
Sistema distribuito che implementa un'architettura master-slave per il calcolo
parallelo di radici quadrate. Il master coordina 10 macchine slave attraverso 
comunicazione multicast UDP, mentre gli slave eseguono calcoli su range numerici
specifici e comunicano il completamento al master.

ARCHITETTURA:
------------
- Master (master.c): Coordina i round di calcolo inviando messaggi multicast
- Slave (slave.c): Ricevono istruzioni multicast e calcolano radici quadrate
- Ogni slave ha un ID unico K (0-9) che determina il proprio range di calcolo
- Range di calcolo per slave K nel round N: M*(N*10 + K) ≤ H < M*(N*10 + K + 1)

FILE INCLUSI:
------------
- master.c         : Programma master
- slave.c          : Programma slave
- Makefile         : Script di compilazione
- README.txt       : Questa documentazione

DIPENDENZE:
----------
- Libreria APUE (apue.h)
- Librerie standard: pthread, math, socket
- Compilatore: gcc con supporto C90/ANSI

CONFIGURAZIONE:
--------------
Prima di compilare, modificare nei sorgenti:

1. In master.c e slave.c, linea ~15:
   #define MULTICAST_ADDR "224.0.0.X"
   Sostituire X con il quarto ottetto del vostro IP

2. In slave.c, linea ~20:
   #define SLAVE_K 0
   Modificare per ogni istanza slave (0-9)

3. In slave.c, linea ~17:
   #define MASTER_ADDR "127.0.0.1"
   Modificare con l'IP del master se su macchine diverse

COMPILAZIONE:
------------
1. Posizionarsi nella directory contenente i file
2. Eseguire: make
3. Verranno creati gli eseguibili 'master' e 'slave'

In caso di errori:
- Verificare che la libreria APUE sia correttamente installata
- Controllare i percorsi in ./lib/ e ./include/

ESECUZIONE:
----------
1. AVVIARE IL MASTER:
   ./master
   
   Il master mostrerà:
   - Parametri di rete utilizzati
   - Invio di ogni round multicast
   - Ricezione conferme dagli slave
   
2. AVVIARE GLI SLAVE:
   Per testare in locale, aprire terminali separati e:
   
   Terminale 1: ./slave    (userà SLAVE_K = 0)
   
   Per più slave, modificare SLAVE_K in slave.c e ricompilare:
   - Modificare #define SLAVE_K 1
   - make clean && make
   - ./slave
   
   Ogni slave mostrerà:
   - Ricezione messaggi multicast
   - Range di calcolo assegnato
   - Progresso dei calcoli
   - Invio conferma al master

PARAMETRI DI DEFAULT:
--------------------
- Numero di round: 10 (N da 0 a 9)
- Valore M: 15000
- Porta multicast: 9999
- Porta unicast: 9998
- Numero slave: 10 (K da 0 a 9)

OUTPUT:
------
- Il master mostra il coordinamento dei round sulla console
- Ogni slave salva i risultati in: results_slave_K.txt
- I file risultato contengono tutte le radici quadrate calcolate

ESEMPIO DI RANGE:
----------------
Con M=15000, per il round N=0:
- Slave 0: calcola sqrt(H) per 0 ≤ H < 15000
- Slave 1: calcola sqrt(H) per 15000 ≤ H < 30000  
- Slave 2: calcola sqrt(H) per 30000 ≤ H < 45000
- ...
- Slave 9: calcola sqrt(H) per 135000 ≤ H < 150000

Per il round N=1:
- Slave 0: calcola sqrt(H) per 150000 ≤ H < 165000
- Slave 1: calcola sqrt(H) per 165000 ≤ H < 180000
- ...

VERIFICA RANGE:
--------------
Per verificare i range numerici, eseguire il comando:
M=15000; for N in $(seq 0 9); do for K in $(seq 0 9) ; do echo $((M*(N*10+K))) N=$N K=$K; echo $((M*(N*10+K+1))) N=$N K=$K; echo; done; done

FUNZIONAMENTO DETTAGLIATO:
-------------------------
1. Il master invia 3 datagrammi identici (distanziati 1 sec) con N e M
2. Ogni slave riceve il messaggio e avvia il calcolo del proprio range
3. I calcoli vengono eseguiti in thread separati dal thread di comunicazione
4. Al completamento, ogni slave invia conferma al master
5. Il master attende tutte le conferme prima del round successivo
6. Il processo si ripete per tutti i round (0-9)

THREADING:
---------
Master:
- Thread principale: gestisce invio multicast e controllo round
- Thread receiver: riceve conferme dagli slave

Slave:
- Thread comunicazione: riceve messaggi multicast dal master
- Thread calcolo: esegue calcoli matematici e salva risultati

PULIZIA:
-------
Per rimuovere file compilati e risultati:
make clean

TROUBLESHOOTING:
---------------
1. "bind: Address already in use"
   -> Attendere qualche secondo e riprovare, o cambiare porte

2. "Permission denied" per multicast
   -> Verificare che l'indirizzo multicast sia valido (224.0.0.1-239.255.255.255)

3. Slave non ricevono messaggi
   -> Verificare indirizzo multicast e firewall

4. Errori di compilazione APUE
   -> Controllare installazione libreria e percorsi include/lib

5. Calcoli non salvati
   -> Verificare permessi di scrittura nella directory corrente

NOTA IMPORTANTE:
---------------
Per deployment su macchine separate:
- Modificare MASTER_ADDR in slave.c con IP reale del master
- Modificare MULTICAST_ADDR con indirizzo appropriato
- Assicurarsi che non ci siano firewall che bloccano le porte UDP

================================================================================