/*
PROGETTO FARM2 2023/24
Autore: Baldini Enrico

-----------

Questa sezione di codice contiene il threadpool del processo MasterWorker
Il threadpool mantiene la coda di produzione e in questo file sono contenute le funzioni che masterworker
usa per inserire task in coda.
*/

#include <stdio.h>

#include "pool.h"

/*
//coda
typedef struct queue_node {
	
	
} queue;
*/

//crea il threadpool e la coda dei task
//unsigned int: numero iniziale di worker
//char*: nome socket
int create_pool(size_t pool_size, size_t queue_len, char* socket) {
	//controllo valori validi
	if(pool_size<=0)
		return -1;
	if(queue_len<=0)
		return -1;
	
}

//attende che il threadpool finisca di elaborare i task passati
void await_pool_completion() {

}

//distrugge il threadpool
void destroy_pool() {

}

//Inserisce task in coda
void enqueue_task(char*) {

}

void add_worker() {

}

void remove_worker() {

}
