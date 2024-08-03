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
#include "utils.h"

//lunghezza max di stringa aumentata di 1 per accomodare il carattere di terminazione
#define FILENAME_LEN 1+MAX_NAMELENGTH

//La coda di produzione e' una coda circolare, gestita da pool.c 
//Array di stringhe di lunghezza FILENAME_LEN con indici di prima ed ultima posizione

//I worker thread sono una lista pthread_t e tipo_lista*

//numero di array. Non necessita di lock
size_t workers_num;

//per la coda di produzione
pthread_mutex_t task_mtx=PTHREAD_MUTEX_INITIALIZER;	//mutex coda task
pthread_cond_t task_full=PTHREAD_COND_INITIALIZER;	//coda task piena
pthread_cond_t task_empty=PTHREAD_COND_INITIALIZER;	//coda task vuota

//Funzionamento base del threadpool 
int pool_function(size_t pool_size, size_t queue_len, char* socket) {
	//controllo valori validi
	if(pool_size<=0)
		return -1;
	if(queue_len<=0)
		return -1;
	
	//crea threadpool
	
	
	//crea coda di produzione
	char **task_queue;
	ec_is(task_queue=malloc(queue_len*sizeof(char*)),NULL,"pool, malloc coda 1");
	int i;
	for(i=0;i<queue_len;i++)
		task_queue[i]=ec_is(malloc(FILENAME_LEN*sizeof(char)),NULL,"pool, malloc coda 2");
	int front=-1;
	int rear=-1;
	
	
	//inizializzazione worker
	
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

//Aggiunge thread worker
void add_worker() {

}

//Rimuove thread worker
void remove_worker() {

}
