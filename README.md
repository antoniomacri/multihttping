Scopo del progetto
----------------------

Lo scopo del progetto è estendere il noto programma *httping* implementando la gestione di host multipli. L'applicazione deve essere distribuita mediante il meccanismo dei port usato sul sistema operativo FreeBSD.


Funzionamento di base
---------------------

Nel caso base in cui viene specificato un solo host, il programma si comporta come l'httping classico. Se vengono specificati più host, invece, il programma crea tanti processi figli ciascuno deputato a gestire un singolo host, dopodiché si pone in attesa di ricevere i dati da essi, ne calcola le statistiche per tutto il corso dell'esecuzione e infine le mostra a video.

Lo sviluppo del progetto è stato guidato sin da subito dall'intenzione di modificare il meno possibile il codice esistente. La funzione `main()` originaria, rinominata in `main_single_host()`, è stata modificata solo in alcuni parti, con l'obiettivo di riformattare l'output e uniformarlo al caso multihost. Il nuovo `main()` è stato invece scritto da zero, in modo da permettere al processo padre di intercettare e manipolare le opzioni passate dalla linea di comando, oltre che di individuare gli host. Per il corretto funzionamento dell'applicazione, infatti, nel caso multihost si rende necessario rilevare possibili conflitti tra opzioni o modalità non supportate, avvisando di conseguenza l'utente. Nello specifico:

  * viene intercettata l'opzione `-K` (interfaccia *ncurses*), in quanto non è attualmente supportata in modalità multihost;
  * vengono intercettate le opzioni `-g` (`--url`) e `-h` (`--hostname`), che sono superflue in modalità multi-host;
  * viene intercettata l'opzione `--aggregate` (visualizzazione dati aggregati per gruppi di pacchetti), in quanto non supportata in modalità multihost.

Gli argomenti da linea di comando vengono esaminati dal processo principale, per determinare innanzitutto se la modalità di esecuzione sarà single o multi-host, in accordo alla quale dovranno essere accettate le opzioni specificate. Una volta rilevata la modalità di funzionamento, il processo principale provvederà a eseguire le operazioni relative:

  * in modalità single-host, vengono svolte le operazioni di ping come previsto dal `main()` originale;
  * in modalità multi-host, vengono generati i processi figli specificandogli delle direttive di esecuzione (che vengono aggiunte alle opzioni a riga di comando passate ai figli).


Modifiche apportate
-------------------

  * generazione di processi figli per la gestione del multihost uno per ogni indirizzo specificato in input
  * raccolta dati da ciascun processo per il calcolo dei valori di minimo, massimo e medio del RTT
  * disabilitazione di alcune opzioni presenti nell'applicazione
  * Adattamento dell'opzione -S per la visualizzazione degli intervalli temporali delle varie fasi di comunicazione al caso multihost.
  * Modifica al makefile originario ai fini del port.


Generazione del port FreeBSD
----------------------------

La generazione del port è stata automatizzata mediante la scrittura di un apposito Makefile che si occupi di eseguire tutte le operazione necessarie. In particolare:

  * scaricamento ed estrazione del codice del port originale *httping*;
  * preparazione dei file e generazione delle patch tramite il target `makepatch` fornito dagli strumenti di supporto al sistema dei port FreeBSD;
  * aggiunta dei nuovi file relativi alla gestione multihost;
  * copia del port nelle cartelle di sistema.

In fase di installazione del port, viene scaricato e applicato in automatico il modulo di supporto jansson, richiesto come dipendenza dinamica per effettuare il parsing di dati in formato JSON.


Gestione multihost
------------------

La generazione dei processi figli, che si occupano ciascuno della gestione di uno degli host specificati, si realizza mediante l'uso della funzione di libreria `fork()`. I processi figli operano individualmente come nel caso single-host. Il padre, invece, ha il compito di raccogliere e analizzare i dati prodotti dai figli, i quali stampano i propri dati sullo standard output secondo un formato standard (JSON). Di conseguenza, il padre ricava i dati intercettando lo standard output di ciascun figlio.

La comunicazione avviene tramite la funzione `pipe()` che crea un canale di comunicazione tra ciascun figlio e il padre. Su tale canale ciascun figlio, eseguendo una chiamata alla `dup2()`, immetterà il proprio flusso di output.

Da lato del padre, la funzione `parse_children_output()` si pone in attesa dell'output dei processi figli, facendo uso della `select()`. Nel momento in cui un qualsiasi figlio restituisce dei dati, il padre ne inizia l'elaborazione. L'output di ciascun figlio viene trattato come un flusso di oggetti JSON, dai quali vengono estratte le informazioni di interesse e aggiornate le relative statistiche.

Alla conclusione dei processi figli, vengono visualizzate a video le statistiche finali per ciascuno di essi.


Problemi riscontrati
--------------------

Come conseguenza della particolare metodologia usata, il programma risente di alcune limitazioni. La comunicazione con i processi figli è limitata alla lettura del loro output e di conseguenza è essenzialmente unidirezionale. Questo fa sì che non sia possibile negoziare le informazioni ottenibili al di fuori di quelle che i processi figli forniscono esplicitamente.

L'applicazione originale presentava un problema nell'ottenimento dell'indirizzo IP degli host contattati durante ciascuna connessione. Tale problema era dovuto a un conflitto tra formato IPv4 e IPv6 dell'indirizzo ed è stato risolto semplicemente specificando la corretta dimensione della struttura che viene passata come argomento alla funzione di gestione, in modo che essa riconosca l'indirizzo passato come IPv4.