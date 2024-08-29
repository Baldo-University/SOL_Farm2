/*
Questa sezione di codice contiene la parte del processo Collector

TODO timer ogni secondo per la stampa dei risultati parziali
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "message.h"
#include "utils.h"

/*Main del collector*/
int main(int argc, char *argv[]) {
	/*controllo argomenti*/
	if(argc!=2) {	//al collector deve essere passato solo il nome della socket
		fprintf(stderr,"Collector: passato un numero sbagliato di argomenti\n");
		exit(EXIT_FAILURE);
	}
	
	fprintf(stderr,"---Collector Parte---\n");
	
	/*gestione segnali*/
	sigset_t mask;	//maschera del collector
	ec_is(sigemptyset(&mask),-1,"collector, sigemtpyset");
	ec_isnot(pthread_sigmask(SIG_SETMASK,&mask,NULL),0,"collector, pthread_sigmask");
	//blocca i segnali specificati
	struct sigaction collector_sa;
	memset(&collecotr_sa,0,sizeof(collector_sa));
	collector_sa.sa_handler=SIG_IGN;	//i segnali vengono ignorati dal collector
	ec_is(sigaction(SIGHUP,&collector_sa,NULL),-1,"collector, sigaddset SIGHUP");
	ec_is(sigaction(SIGINT,&collector_sa,NULL),-1,"collector, sigaddset SIGINT");
	ec_is(sigaction(SIGQUIT,&collector_sa,NULL),-1,"collector, sigaddset SIGQUIT");
	ec_is(sigaction(SIGTERM,&collector_sa,NULL),-1,"collector, sigaddset SIGTERM");
	ec_is(sigaction(SIGUSR1,&collector_sa,NULL),-1,"collector, sigaddset SIGUSR1");
	ec_is(sigaction(SIGUSR2,&collector_sa,NULL),-1,"collector, sigaddset SIGUSR2");
	ec_is(sigaction(SIGPIPE,&collector_sa,NULL),-1,"collector, sigaddset SIGPIPE");
	
	return 0;
}
