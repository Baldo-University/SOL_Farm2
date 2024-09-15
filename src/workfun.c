/*
Questa sezione di codice contiene la funzione usata dai worker thread per calcolare il valore di un dato file.
Il risultato atteso e' la sommatoria "pesata" che va da 0 a N-1, dove N e' il numero di interi long nel file, del
prodotto tra 'i' e l'i-esimo intero.
*/

#include <errno.h>
#include <stdio.h>

#include "headers/utils.h"
long workfun(char *filename) {
	long result=0;
	FILE *task=fopen(filename,"r");	//apertura del file con bufferized I/O
	if(task==NULL) {
		DEBUGGER(perror("workfun, fopen"));
		return -1;
	}
	long buf;	//buffer che contiene il byte corrente
	long i=0;	//indice i di sommatoria
	while((fread(&buf,sizeof(long),1,task))>0) {
		result+=(i*buf);	// i*file[i]
		i++;
	}
	if(ferror(task) || !feof(task))
		DEBUGGER(fprintf(stderr,"Workfun: errore durante la lettura del file. Risultato non affidabile\n"));
	if(fclose(task)!=0)
		DEBUGGER(perror("workfun, fclose"));
	return result;
}
