Scopo del progetto
----------------------
Supporto multihost a un'applicazione di http-ping con calcolo di valori per la valutazione delle tempistiche di ping

Modifiche apportate
-----------------------
* generazione di processi figli per la gestione del multi body! uno per ogni indirizzò specificato in input
* raccolta dati da ciascun processo per il calcolo dei valori di minimo, massimo e medio del RTT
* disabilitazione di alcune opzioni presenti nell'applicazione
* inserimento dell'opzione -S per la visualizzazione degli intervalli temporali delle varie fasi di comunicazione

Problemi riscontrati
----------------------
* l'applicazione non mostra gli indirizzi ip su FreeBSD OS, é un problema dell'applicazione originale che abbiamo deciso di non modificare per non alterare troppo il codice. Su altri sistemi Linux (es. Ubuntu) il problema non si presenta
* estrazione dei valori temporali dei processi figli risolto con una struttura dati appositi per il salvataggio di questi ultimi
