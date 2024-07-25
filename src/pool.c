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

//coda circolare
typedef struct queue {
	size_t len;
	char *spots;
	unsigned int front;
	unsigned int rear;
} queue;
	
//crea il threadpool e la coda dei task
//unsigned int: numero iniziale di worker
//char*: nome socket
ThreadPool *create_pool(unsigned int, char*) {
	
}

//attende che il threadpool finisca di elaborare i task passati
void await_completion(ThreadPool*) {

}

//distrugge il threadpool
void destroy_pool(Threadpool*) {

}

//Inserisce task in coda
void enqueue_task(Threadpool*,char*) {

}

void add_worker(Threadpool*) {

}

void remove_worker(Threadpool*) {

}
