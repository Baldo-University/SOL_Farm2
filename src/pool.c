/*
PROGETTO FARM2 2023/24
Autore: Baldini Enrico

-----------

Questa sezione di codice contiene il threadpool del processo MasterWorker
Il threadpool mantiene la coda di produzione e in questo file sono contenute le funzioni che masterworker
usa per inserire task in coda.
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pool.h"
#include "utils.h"

//lunghezza max di stringa aumentata di 1 per accomodare il carattere di terminazione
#define MAX_PATHNAME_LEN 1+MAX_NAMELENGTH

//funzionamento del singolo thread
static void *thread_func(void *arg) {
	fprintf(stdout,"Inizializzazione thread\n");
	threadpool_t *pool=(threadpool_t)arg;
	pthread_mutex_lock(&pool->task_mtx);
	unsigned int id=++pool->threadID;
	pthread_mutex_unlock(&pool->task_mtx);
	
	while(pool->running) {
		pthread_mutex_lock(&pool->task_mtx);
		if(pool->modify_thread_num<0) {	//rimuovere uno o piu' thread
			pthread_mutex_lock(&pool->worker_mtx);
			
		}
	}
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
	*/
}

//Inserisce task in coda
void enqueue_task(char* filename) {
	
}

//Aggiunge thread worker
int add_workers(threadpool_t *pool, long num) {
	if(pool==NULL) {
		fprintf(stderr,"pool, add_workers, pool non inizializzato\n");
		return 1;
	}
	if(pool->running==0) {
		fprintf(stderr,"pool, add_workers, pool non sta girando\n");
		return 2;
	}
	if(num<1) {
		fprintf(stderr,"pool, add_workers, numero di thread non valido\n");
		return 3;
	}
	
	pthread_mutex_lock(&pool->worker_mtx)!=0);
	pool->num_threads+=num;
	pthread_mutex_unlock(&pool->worker_mtx);
	
	int i,res;	//valori ausiliari
	for(i=0;i<num;i++) {
		workerlist *new_worker=(workerlist*)malloc(sizeof(workerlist));
		if(new_worker==NULL) {	//controllo errore malloc
			fprintf(stderr,"pool, impossibile allocare memoria al thread\n");
			pthread_mutex_lock(&pool->worker_mtx)!=0);
			pool->num_threads--;
			pthread_mutex_unlock(&pool->worker_mtx);
		}
		new_worker->next=NULL;
		res=pthread_create(&new_worker->thread,NULL,thread_func,(void*)pool);
		if(res) {	//controllo errore pthread_create
			fprintf(stderr,"pool, add_workers, errore in pthread_create() del thread %d, errore %d\n",i+1,res);
			pthread_mutex_lock(&pool->worker_mtx)!=0);
			pool->num_threads--;
			pthread_mutex_unlock(&pool->worker_mtx);
		}
		else {
			if(pool->worker_tail==NULL)	//primo thread
				pool->worker_head = pool->worker_tail = new_worker;
			else
				pool->worker_tail->next=new_worker;
			pool->worker_tail=new_worker;
		}
	}
	return 0;
}

//Rimuove thread worker
void remove_worker() {

}

threadpool_t *initialize_pool(long pool_size, size_t queue_len, char* socket) {
	fprintf(stdout,"Inizializzazione threadpool\n");
	//controllo valori validi
	if(pool_size<=0)
		return NULL;
	if(queue_len<=0)
		return NULL;
	
	//crea threadpool
	threadpool_t *pool=(threadpool_t*)malloc(sizeof(threadpool_t));
	if(pool==NULL) {
		fprintf(stderr,"pool, initialize_pool, non e' stato possibile allocare memoria al threadpool.\n");
		return NULL;
	}
	memset(pool,0,sizeof(*pool));
	pool->worker_head=NULL;
	pool->worker_tail=NULL;
	pool->socket=malloc(9*sizeof(char));	//"farm2.sck" e' lunga 9 byte. Cambiare se si usa un altra socket!!
	if(pool->socket==NULL) {
		fprintf(stderr,"pool, initialize_pool, fallita malloc di stringa socket\n");
		return NULL;
	}
	strncpy(pool->socket,socket,9);
	//inizializzazione mutex e cond
	pthread_mutex_init(&pool->worker_mtx,NULL);
	pthread_cond_init(&pool->worker_cond,NULL);
	pthread_mutex_init(&pool->task_mtx,NULL);
	pthread_cond_init(&pool->task_full,NULL);
	
	pool->running=1;
	
	//crea coda di produzione
	char **task_queue=malloc(queue_len*sizeof(char*));
	if(task_queue==NULL) {
		fprintf(stderr,"pool, initialize_pool, fallita malloc di coda\n");
		pthread_mutex_destroy(&pool->worker_mtx);
		pthread_cond_destroy(&pool->worker_cond);
		pthread_mutex_destroy(&pool->task_mtx);
		pthread_cond_destroy(&pool->task_full);
		free(pool->socket);
		free(pool);
		return NULL;
	}
	int i;
	for(i=0;i<queue_len;i++) {
		task_queue[i]=malloc(MAX_PATHNAME_LEN*sizeof(char));
		if(task_queue[i]==NULL) {
			fprintf(stderr,"pool, initialize_pool, fallita malloc di queue[%d]\n",i);
			int j;
			for(j=0;j<i;j++)
				free(task_queue[j]);
			pthread_mutex_destroy(&pool->worker_mtx);
			pthread_cond_destroy(&pool->worker_cond);
			pthread_mutex_destroy(&pool->task_mtx);
			pthread_cond_destroy(&pool->task_full);
			free(pool->socket);
			free(pool);
			return NULL;
		}
	}
	pool->queue_size=queue_len;
	pool->tasks_head=-1;
	pool->tasks_tail=-1;
	
	int res;
	res=add_workers(pool,pool_size);
	if(res<0) {
		fprintf(stderr,"pool, initialize_pool, add_workers esce con errore %d\n",res);
		for(i=0;i<queue_len;i++)
			free(task_queue[j]);
		pthread_mutex_destroy(&pool->worker_mtx);
		pthread_cond_destroy(&pool->worker_cond);
		pthread_mutex_destroy(&pool->task_mtx);
		pthread_cond_destroy(&pool->task_full);
		free(pool->socket);
		free(pool);
		return NULL;
	}
	
	while(!pool->initialized);	//attesa attiva di inizializzazione pool
	
	return pool;
}
