/*
Questa sezione di codice contiene la parte del processo Collector

TODO timer ogni secondo per la stampa dei risultati parziali
*/

#include <pthread.h>
#include <stdio.h>

#include "utils.h"

//tipo dei risultati da stampare
//andrebbe messo in un unico header a cui fa riferimento anche workfun/pool
typedef struct result_path {
	long result;
	char *pathname;
} result_path_t;

int main(int argc, char *argv[]) {
	fprintf(stdout,"---Collector Parte---\n");
	fflush(stdout);
	return 0;
}
