/*
PROGETTO FARM2 2023/24
Autore: Baldini Enrico, matrciola 597780

Questa sezione di codice contiene il main lanciato come processo iniziale.
Il main invoca fork per lanciare il processo figlio collector.
Il processo padre invoca la funzione masterworker.
*/

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "headers/masterworker.h"	//header della funzione che indica il funzionamento di masterworker
#include "headers/utils.h"	//utilities

#define SOCKNAME "farm2.sck"	//nome socket, da passare a collector e masterworker

int main(int argc, char *argv[]) {
	
	//controllo del numero di argomenti passati. 
	//Se non ne sono stati passati, il programma si interrompe immediatamente.
	if(argc<=1) {
		fprintf(stderr,"Usare il seguente formato:\n%s [-n <nthread>] [-q <qlen>] [-d <pathname>] [-t <delay>] <filename> [, <filename>, ...]\n",argv[0]);
		exit(EXIT_FAILURE);
	}
	
	/*impostazione maschera*/
	pid_t collector;
	sigset_t mask, oldmask;	//mask e' la nuova maschera, oldmask e' quella corrente
	//prepara maschera che blocca tutti i segnali
	ec_is(sigfillset(&mask),-1,"main, sigfillsset");
	//setta la suddetta maschera sul thread corrente e salva in oldmask la maschera precedente
	ec_is(pthread_sigmask(SIG_SETMASK,&mask,&oldmask),-1,"main, pthread_sigmask pre-fork")
	
	//Si crea un processo figlio con fork(), il quale lancera' collector.
	//Il thread padre lancera' il MasterWorker.
	collector=fork();
	switch(collector) {
	case -1:	//errore fork
		perror("Main, fork");
		exit(EXIT_FAILURE);
	case 0:		//figlio
		execl("./collector","collector",SOCKNAME,(char*)NULL);
		//se si arriva a questa sezione di codice qualcosa e' andato storto
		perror("Main, execl collector");
		exit(EXIT_FAILURE);
	default:	//padre
		//rimozione maschera dei segnali
		ec_is(pthread_sigmask(SIG_SETMASK,&oldmask,NULL),-1,"main pthread_sigmask padre");
		masterworker(argc,argv,SOCKNAME);
	}
	
	//attesa fine del figlio collector
	int collector_status;
	collector=waitpid(collector,&collector_status,0);
	if(WIFEXITED(collector_status))
		DEBUG("Main: stato collector: %d\n",WEXITSTATUS(collector_status));
	
	DEBUG("Fine programma\n");
	return 0;
}
