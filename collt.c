#include <errno.h>
#include <stdio.h>

#include "message.h"

long workfun(char *filename) {
	long result=0;
	FILE *task=fopen(filename,"r");	//apertura del file con bufferized I/O
	if(task==NULL) {
		perror("workfun, fopen");
		return -1;
	}
	long buf;	//buffer che contiene il byte corrente
	long i=0;	//indice i di sommatoria
	while((fread(&buf,sizeof(long),1,task))>0) {
		result+=(i*buf);	// i*file[i]
		i++;
	}
	if(ferror(task) || !feof(task))
		fprintf(stderr,"Workfun: errore durante la lettura del file. Risultato non affidabile\n");
	if(fclose(task)!=0)
		perror("workfun, fclose");
	return result;
}
