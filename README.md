# SOL_Farm2
Progetto SOL 2023/24, appello di Settembre.

Istruzioni rapide per l'utilizzo di Farm2

Su Linux, aprire un terminale sulla directory principale del progetto. Dopodiché, digitare i seguenti comandi:

make

make test

make clean

Il primo comando compila il codice sorgente e generafile.c. Il secondo lancia lo script test.sh. Il terzo ripulisce le directory da tutti i file .o, .dat e dagli eseguibili creati durante la fase di compilazione e test.

Digitando "make debug" al posto di "make", il programma viene lanciato in versione debug

NOTA 15/9/2024: l'errore dei primi due test di "test.sh" nella versione originale del programma erano dovuti ad un comando "pthred_kill()", inserito durante la fase finale di test e situato tra la ricerca di file da linea di comando e nelle directory, che inviava un SIGTERM al master. Ciò portava ad una terminazione prematura del programma. Da notare che i file non inviati al collector erano proprio quelli all'interno di directory passate con -d.  
