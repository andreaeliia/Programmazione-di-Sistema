Esercizio

Scrivere un programma che funzioni da server TCP e un programma che funzioni da client per tale server.

Il servizio può consistere nella semplice visualizzazione a terminale di un carattere casuale inviato dal server.

Il client deve accedere al server con due thread e deve fare in modo che gli intervalli di tempo in cui le due thread accedono al server non siano mai,
neanche parzialmente, sovrapposti. 



Per sincronizzare l'accesso al server delle due thread si usi una condition variable.


Il server deve gestire gli accessi multipli tramite una select.