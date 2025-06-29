# TCP client-server

## Analisi del traffico di rete
***

Il protocollo TCP richiede una comunicazione che implementi il modello client-server.


Il server deve effettuare una **passive open**, mentre il client, _noto l'indirizzo del server TCP_, effettua la **active open**.

La **connect()** avvia il procedimento di 3 way handshake, al termine del quale entrambe le applicazioni remoto hanno un socket TCP connesso sul quale poter avviare la 
comunicazione. 

Al fine di illustrare cosa succede durante il procedimento di **3WHS**, costruiamo un _fake server TCP_:

- si blocca prima di aprire il socket;
- si blocca prima di effettuare il bind dell'indirizzo al socket;
- si blocca prima di effettuare la passive open;
- non esegue mai accept;

Il codice di riferimento è **05- FakeTCP/FakeServer.c**. 

---

### Server running hplinux2.unile.it

---

#### Opzione 1 

- il socket non viene creato;
- _netstat_ non lo elenca e _Wireshark_ mostra che la richiesta di connessione da parte di un client viene rifiutata (->SYN : <-RST) ed la _connect()_ nel client ritorna con l'errore **ECONNREFUSED**

---

#### Opzione 2:

- il socket è stato creato ma _netstat_ non lo elenca
- _Wireshark_ mostra che la richiesta di connessione da parte di un client viene rifiutata: (->SYN : <-RST)

---

#### Opzione 3:
- il socket è stato creato e gli è stato assegnato un local address, ma rimane la stessa situazione dei casi precedenti

---

#### Opzione 4:
A questo punto, la **Passive Open** è stata completata:

_netstat_ ci riporta il socket nello stato LISTEN:

`tcp        0      0 *:49152                 *:*                     LISTEN`

Se ora il client (running su Mac OS X 10.10.5) si connette al server, finalmente la connessione viene **established**.

Su Linux _netstat_ ci riporta i 2 socket del server: 

`hplinux2:~$ netstat -at | grep 49152`
`tcp        0      0 *:49152                 *:*                     LISTEN`     
`tcp       12      0 hplinux2.unile.it:49152 5.170.131.121:24211     ESTABLISHED`
	
in cui i **12 byte** corrispondono al messaggio di _12 bytes_ inviati dal client al server.
	
Quindi su Linux il TCP layer accetta connessioni su un socket solo se è stata completata la **passive open**. 

In tutti gli altri casi, un **RST** è generato in risposta (analogamente allo scenario di processo server TCP _not running_). 
	
Su Mac OS X, per il processo client vediamo (notare che il client è NATed):

`macosx:~$ netstat -a -p tcp | grep 49152` 
`tcp4       0      0  192.168.0.102.55412    hplinux2.unile.i.49152 ESTABLISHED`
	
_Wireshark_ ci mostra il traffico scambiato: 
- dopo il 3WHS, il client ha inviato un segmento di dati che è stato correttamente riscontrato dal TCP ricevente.
- A questo punto il client rimane bloccato indefinitamente in attesa della risposta da parte del server.
	
> terminiamo il server: i suoi socket vengono chiusi (_netstat_ non li elenca più). 
> Questo genera l'invio di un **RST** al client, che quindi termina (*connection reset by remote peer*). 
	
	*Attenzione*: Il RST potrebbe andare perso, e in questo caso il client rimarrebbe appeso indefinitamente.
	Sul client, il socket risulta ancora connesso.
	Non ci resta che terminare anche il processo client: questo fa partire il processo di 4-Way-Teardown.
	Il client invia un FIN che non sarà mai riscontrato dal server. 
	Il TCP client ritrasmette il FIN fino allo scadere del timeout (su Mac OS ~100s): 
	se nessuna risposta è ricevuta, il TCP resetta la connessione, inviando il RST al server.
	Il socket del client entra nello stato FIN_WAIT_1, e poi viene chiuso dal RST.
	
> Se invece terminiamo il processo client, mentre il server non ha ancora rilevato la connessione né i dati.
> La chiusura di connessione è inviata al server, su cui netstat ci mostra:
	
`hplinux2:~$ netstat -at | grep 49152` 
`tcp        0      0 *:49152                 *:*                     LISTEN` 
`tcp       13      0 hplinux2.unile.it:49152 5.170.131.121:24825     CLOSE_WAIT` 

> Il socket connesso entra nello stato **CLOSE_WAIT**, e ai 12 byte se ne è aggiunto 1 relativo alla chiusura della connessione. 

> Il processo server è ancora running.
	
> Su Mac OS X invece il processo Client è stato terminato e _netstat_ ci riporta il socket nello stato `FIN_WAIT_2`: 

`macosx:~$ netstat -a -p tcp | grep 49152` 

`tcp4       0      0  192.168.0.102.55445    hplinux2.unile.i.49152 FIN_WAIT_2`
	
_Wireshark_ ci conferma quanto segue: 

- il client ha inviato un **FIN**, che è stato correttamente riscontrato dal TCP server (FIN,ACK), ma a cui non è seguito il FIN da parte del server. 
	
A questo punto abbiamo: 

- su Linux: server running, 1 socket in LISTEN state e 1 socket in CLOSE_WAIT
- su MAC OS X: client terminated, 1 socket in FIN_WAIT_2 (fino ad un timeout, dopo il quale viene chiuso)
	
Quando il processo server verrà terminato, i suoi socket verranno chiusi. 

***

### Server running on MACH3

#### Opzione 1
Il socket non viene creato e _netstat_ non lo elenca.
	
Un Client (running su un altro Mac OS X) invia un **SYN**: 
- il socket entra nello stato SYN_SENT
- il TCP client però non riceve nessuna risposta. 
- parte il meccanismo delle ritrasmissioni (~75s), al termine delle quali il client termina la connect con l'errore **Operation Timed-out** ed il processo termina (il socket viene chiuso).

> _Wireshark_ mostra le ritrasmissioni del SYN, e a differenza del server Linux, nessun RST viene inviato dal TCP layer dell'host su cui è stato lanciato il fake server TCP. 

---

#### Opzione 2
Socket creato ma no *bind*ed: come prima.

---

#### Opzione 3
Il socket è stato creato e gli è stato assegnato un local address.

_netstat_ lo riporta nello stato iniziale:

`tcp4       0      0  *.49152                *.*                    CLOSED`
	
Il client avvia la connessione, che prosegue come prima: ritrasmissioni di SYN.

> Anche in questo caso, il TCP layer dell'host su cui è attivo il fake server scarta i SYN *silently*. 

---

#### Opzione 4
A questo punto la Passive Open è stata completata.
_netstat_ ci riporta il socket nello stato LISTEN:

`tcp        0      0 *:49152                 *:*                     LISTEN`

Il client (running su Mac OS X 10.10.5) si connette al server e la connessione viene **established**.

I dati vengono inviati e riscontrati dal server.
	
Su MACH3 ora abbiamo 2 socket: 


`mach3:~ kat$ netstat -a -p tcp | grep 49152` 

`tcp4      12      0  mach3.unile.it.49152   5.170.131.121.24282    ESTABLISHED` 

`tcp4       0      0  *.49152                *.*                    LISTEN` 
	
Sul Client Mac OS X: 

`$ netstat -a -p tcp | grep 49152` 
`tcp4       0      0  192.168.0.102.55550    mach3.unile.it.49152   ESTABLISHED`
	
	*Attenzione*: termino il server running su MACH3. I suoi socket vengono chiusi, un **RST** inviato al client (ma perso). 

Stesso scenario come in Linux: il client ritrasmette i **FIN** e allo scadere del timeout resetta la connessione.


> Termino prima il client: come per Linux, il client chiude la sua metà della connessioe e attende la chiusura del server. 
> (il socket del client rimane nello stato FIN_WAIT_2 fino allo scadere del timeout) 


