/*
PROGETTO FARM2 2023/24
Autore: Baldini Enrico

-----------

Questa sezione di codice contiene il threadpool del processo MasterWorker
Il threadpool mantiene la coda di produzione e in questo file sono contenute le funzioni che masterworker
usa per inserire task in coda.
*/

#include <stdio.h>
#include <pthread.h>

#include "pool.h"
#include "utils.h"

//lunghezza max di stringa aumentata di 1 per accomodare il carattere di terminazione
#define MAX_PATHNAME_LEN 1+MAX_NAMELENGTH



//funzionamento del singolo thread
static void *thread_func(void *arg) {
	
}

//attende che il threadpool finisca di elaborare i task passati
void await_pool_completion() {

}

//distrugge il threadpool
void destroy_pool() {

}

//Inserisce task in coda
void enqueue_task(char* filename) {
	
}

//Aggiunge thread worker
void add_workers(long num) {

}

//Rimuove thread worker
void remove_worker() {

}

threadpool_t *initialize_pool(long pool_size, size_t queue_len, char* socket) {	
	//controllo valori validi
	if(pool_size<=0)
		return NULL;
	if(queue_len<=0)
		return NULL;
	
	//crea threadpool
	threadpool_t *pool=(threadpool_t*)malloc(sizeof(threadpool));
	if(pool==NULL) {
		fprintf(stderr,"Threadpool: non e' stato possibile allocare memoria al threadpool.\n");
		return NULL;
	}
	//inizializzazione valori di base
	pool->initialized=0;
	pool->workers_head=NULL;
	pool->workers_tail=NULL;
	pool->num_threads=0;
	pool->modify_thread_num=0;
	//inizializzazione mutex e cond
	pthread_mutex_init(&pool->task_mtx,NULL);
	pthread_cond_init(&pool->task_full,NULL);
	
	pool->run=1;
	
	//crea coda di produzione
	char **task_queue;
	ec_is(task_queue=malloc(queue_len*sizeof(char*)),NULL,"pool, malloc coda 1");
	int i;
	for(i=0;i<queue_len;i++)
		task_queue[i]=ec_is(malloc(MAX_PATHNAME_LEN*sizeof(char)),NULL,"pool, malloc coda 2");
	int front=-1;
	int rear=-1;
	
	add_workers(pool_size);
	
	return pool;
}
