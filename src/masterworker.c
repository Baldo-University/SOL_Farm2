/*
PROGETTO FARM2 2023/24
Autore: Baldini Enrico

-----------

Questa sezione di codice contiene il thread MasterWorker.
Il masterworker prende i filename passati da linea di comando e le opzioni.
Crea un thread che gestisca i segnali (TODO)
Crea un thread che inserisca i filepath in una lista (TODO)
Crea un thread che tolga i file dalla suddetta lista e li metta sulla coda di produzione (TODO)
Crea il threadpool di worker (TODO)
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "masterworker.h"
#include "utils.h"

//struct dei file da valutare.
typedef struct node {
	char name[MAX_NAMELENGTH];	//filename
	struct node *next;			//prossimo elemento di lista
} node_t;

void masterworker(int argc, char *argv[], char *socket) {
	long workers=DEFAULT_N;
	size_t qlen=DEFAULT_Q;
	long delay=DEFAULT_T;
	
	//crea due liste, una di filename e l'altra di dirname con rispettivi lock
	//la prima ha i nomi dei file da mandare a collector, la seconda le directory da esplorare
	//un thread esplora le dir e salva i file nella prima e le directory nella seconda
	//un thread prende i file della prima lista e li mette nella coda di produzione
	node_t *files=NULL;		//lista di directory
	node_t *dirs=NULL;		//lista di filename
	node_t *dirs_aux=dirs;	//puntatore ausiliario
	
	/*Analisi delle opzioni*/
	int opt;
	while((opt=getopt(argc,argv,"n:q:d:t:"))!=-1) {
		
	}
}
