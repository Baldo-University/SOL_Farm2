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
void destroy_pool(threadpool_t *pool) {
	if(pool==NULL || !pool->initialized) {
		fprintf(stderr,"pool, destroy_pool su threadpool non inizializzato\n");
		return;
	}
	pool->running=0;
	
	int res;
	
	//ferma i worker TODO
	
	//dealloca la coda
	int i;
	size_t dim=pool->queue_size;
	if(pool->task_queue==NULL)
		fprintf(stderr,"pool, destroy_pool trova coda dei task non inizializzata\n");
	else {
		for(i=0;i<dim;i++)
			free(pool->task_queue[i]);
		free(pool->task_queue);
	}
	
	//distruggi mutex e cond
	res=pthread_mutex_destroy(&pool->worker_mtx);
	res=pthread_mutex_destroy(&pool->task_mtx);
	if(res)
		fprintf(stderr,"pool, destroy_pool. Uno o piu' mutex non sono stati distrutti\n");
	res=pthread_cond_destroy(&pool->worker_cond);
	res=pthread_cond_destroy(&pool->task_full);
	if(res)
		fprintf(stderr,"pool, destroy_pool. Uno o piu' cond non sono state distrutte\n");
	free(pool);
}

//Inserisce task in coda
void enqueue_task(char* filename) {
	
}

//Aggiunge thread worker
int add_workers(threadpool_t *pool, long num_threads) {
	if(pool==NULL) {
		fprintf(stderr,"pool, add_workers, pool non inizializzato\n");
		return 1;
	}
	if(pool->running==0) {
		fprintf(stderr,"pool, add_workers, pool non sta girando\n");
		return 2;
	}
	if(num_threads<1) {
		fprintf(stderr,"pool, add_workers, numero di thread non valido\n");
		return 3;
	}
	
	if(pthread_mutex_lock()
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
	memset(pool,0,sizeof(*pool));
	//inizializzazione mutex e cond
	pthread_mutex_init(&pool->worker_mtx,NULL);
	pthread_cond_init(&pool->worker_cond,NULL);
	pthread_mutex_init(&pool->task_mtx,NULL);
	pthread_cond_init(&pool->task_full,NULL);
	
	pool->running=1;
	
	//crea coda di produzione
	char **task_queue=malloc(queue_len*sizeof(char*));
	if(task_queue==NULL) {
		perror("pool, malloc coda");
		destroy_pool(pool);
		return NULL;
	}
	size_t i;
	for(i=0;i<queue_len;i++)
		task_queue[i]=ec_is(malloc(MAX_PATHNAME_LEN*sizeof(char)),NULL,"pool, malloc coda 2");
	pool->queue_size=queue_len;
	pool->tasks_head=-1;
	pool->tasks_tail=-1;
	
	int res;
	res=add_workers(pool,pool_size);
	if(res!=0) {
		destroy_pool(pool);
		return NULL;
	}
	
	return pool;
}
