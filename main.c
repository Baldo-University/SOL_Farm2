/*
PROGETTO FARM2 2023/24
Autore: Baldini Enrico

-----------

Questa sezione di codice contiene il main lanciato come processo iniziale
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "masterthread.h"	//header della funzione del masterworker

#define SOCKNAME "farm2.sck"

int main(int argc, char *argv[]) {
	
	//controllo del numero di argomenti passati. 
	//Se non ne sono stati passati, il programma si interrompe immediatamente.
	if(argc<=1) {
		fprintf(stderr,"Usare il seguente formato: \n%s [");
		EXIT(exit_failure);
	}
	
	//Si crea un processo figlio con fork(), il quale lancera' collector.
	//Il thread padre lancera' il MasterWorker.
	//TODO rifare con signal masking
	switch(fork()) {
	case -1:	//errore fork
		perror("Main, fork");
		exit(EXIT_FAILURE);
	case 0:		//figlio
		execl("./collector","collector",SOCKNAME,(char*)NULL);
		//se si arriva a questa sezione di codice qualcosa e' andato storto
		perror("Main, exec");
		exit(EXIT_FAILURE);
	default:	//padre
		master_worker(argc,argv);
	}
	
	return 0;
}
