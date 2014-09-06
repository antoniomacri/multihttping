Scopo del progetto
----------------------
Supporto multihost a un'applicazione di http-ping con calcolo di valori per la valutazione delle tempistiche di ping


Funzionamento di base
---------------------

Se viene specificato un solo host, il programma si comporta come l'httping classico (caso base). Se vengono specificati più host, il programma crea tanti processi figli ciascuno deputato a gestire un singolo host. Il processo padre riceve i dati da tutti i figli, ne calcola le statistiche e le mostra a video.

Lo sviluppo del progetto è stato guidato sin dall'inizio dalla intenzione di modificare il meno possibile il codice esistente. La funzione `main()` originaria, rinominata in `main_single_host()`, è stata modificata solo in alcuni parti, con l'obiettivo di riformattare l'output e uniformarlo al caso multihost. Il nuovo `main()` è stato invece riscritto da zero, in modo da permettere al processo padre di intercettare e manipolare le opzioni passate dalla linea di comando, oltre che di individuare gli host. Per il corretto funzionamento dell'applicazione, infatti, nel caso multihost si rende necessario rilevare possibili conflitti tra opzioni o modalità non supportate, avvisando di conseguenza l'utente. Nello specifico:

    * viene intercettata l'opzione `-K` (interfaccia *ncurses*), in quanto non è attualmente supportata in modalità multihost;
    * vengono intercettate le opzioni `-g` (`--url`) e `-h` (`--hostname`), che servono a determinare la modalità di funzionamento (single-host o multi-host);
    * viene intercettata l'opzione `--aggregate` (visualizzazione dati aggregati per gruppi di pacchetti), in quanto non supportata in modalità multihost.

Gli argomenti da linea di comando vengono esaminati dal processo principale, per determinare innanzitutto se la modalità di esecuzione sarà single o multi-host, in accordo alla quale dovranno essere accettate le opzioni specificate. Una volte rilevata la modalità di funzionamento, il processo principale provvederà a eseguire le operazioni relative:

    * in modalità single-host, vengono svolte le operazioni di ping come previsto dal main originale;
    * in modalità multi-host, vengono generati i processi figli specificandogli delle direttive di esecuzione (che vengono aggiunte alle opzioni a riga di comando passate ai figli).


Modifiche apportate
-----------------------
* generazione di processi figli per la gestione del multihost uno per ogni indirizzò specificato in input
* raccolta dati da ciascun processo per il calcolo dei valori di minimo, massimo e medio del RTT
* disabilitazione di alcune opzioni presenti nell'applicazione
* inserimento dell'opzione -S per la visualizzazione degli intervalli temporali delle varie fasi di comunicazione


Gestione multihost
------------------

La generazione dei processi figli, che si occupano ciascuno della gestione di uno degli host specificati, si realizza mediante l'uso della funzione di libreria `fork()`. I processi figli operano individualmente come nel caso single-host. Il padre, invece, ha il compito di raccogliere e analizzare i dati prodotti dai figli, i quali stampano i propri dati sullo standard output secondo un formato standard (JSON). Di conseguenza, il padre ricava i dati intercettando lo standard output di ciascun figlio.

La comunicazione avviene tramite la funzione `pipe()` che crea un canale di comunicazione tra ciascun figlio e il padre. Su tale canale ciascun figlio, eseguendo una chiamata alla `dup2()`, immetterà il proprio flusso di output.

Da lato del padre, la funzione `parse_children_output()` si pone in attesa dell'output dei processi figli, facendo uso della `select()`. Nel momento in cui un qualsiasi figlio restituisce dei dati, il padre ne inizia l'elaborazione. L'output di ciascun figlio viene trattato come un flusso di oggetti JSON, dai quali vengono estratte le informazioni di interesse e aggiornate le relative statistiche.

Alla conclusione dei processi figli, vengono visualizzate a video le statistiche finali per ciascuno di essi.


Problemi riscontrati
--------------------

Come conseguenza della particolare metodologia usata (minimizzare gli interventi sul codice originale), il programma risente di alcune limitazioni. La comunicazione con i processi figli è limitata alla lettura del loro output e di conseguenza è essenzialmente unidirezionale. Questo fa sì che non sia possibile negoziare le informazioni ottenibili al di fuori di quelle che i processi figli forniscono esplicitamente.

* l'applicazione non mostra gli indirizzi ip su FreeBSD OS, é un problema dell'applicazione originale che abbiamo deciso di non modificare per non alterare troppo il codice. Su altri sistemi Linux (es. Ubuntu) il problema non si presenta
