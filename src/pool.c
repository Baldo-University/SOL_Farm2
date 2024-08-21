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
/*
//lunghezza max di stringa aumentata di 1 per accomodare il carattere di terminazione
#define MAX_PATHNAME_LEN 1+MAX_NAMELENGTH
*/
//funzionamento del singolo thread
static void *thread_func(void *arg) {
	fprintf(stdout,"Inizializzazione thread\n");
	threadpool_t *pool=(threadpool_t)arg;
	pthread_mutex_lock(&pool->task_mtx);
	unsigned int id=++pool->threadID;
	pthread_mutex_unlock(&pool->task_mtx);
	//qui mantiene il nome dei task che consuma dalla coda
	char taskname[MAX_PATHNAME_LEN];
	
	//connessione al collector TODO
	
	while(pool->running) {
		pthread_mutex_lock(&pool->task_mtx);
		if(pool->modify_thread_num<0) {	//bisogna rimuovere un thread, quello corrente esce dal loop
			pthread_mutex_lock(&pool->worker_mtx);
			pool->waiting_workers++;
			pthread_mutex_unlock(&pool->worker_mtx);
			break;
		}
		
		memset(taskname,0,MAX_PATHNAME_LEN);
		while(pool->tasks_head==NULL)	//nessun task in coda
			pthread_cond_wait(&pool->worker_cond,&pool->task_mtx);	//si mette in attesa di task
		else {
			//pthread_cond_signal() per dire che c'e' spazio in coda
		}
	}
	
	if(pool->running) {	//thread uscito dal loop
		pool->modify_thread_num++;
		pthread_mutex_unlock(&pool->task_mtx);
	}
	
	//disconnessione dal collector TODO
	
	pthread_exit((void*)NULL);
}

//distrugge il threadpool
void destroy_pool(threadpool_t *pool) {
	if(pool==NULL || !pool->initialized) {
		fprintf(stderr,"pool, destroy_pool su threadpool non inizializzato\n");
		return;
	}
	
	pool->running=0;
	
	pthread_mutex_lock(&pool->worker_mtx);
	pthread_cond_broadcast(&pool
	
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
	res=pthread_cond_destroy(&pool->task_cond);
	if(res)
		fprintf(stderr,"pool, destroy_pool. Uno o piu' cond non sono state distrutte\n");
	free(pool);
}

//Inserisce task in coda
int enqueue_task(threadpool_t *pool, char* filename) {
	if(pool==NULL || !pool->initialized) {
		fprintf(stderr,"pool, enqueue_task, pool non inizializzato\n");
		return -1;
	}
	if(filename==NULL) {
		fprintf(stderr,"pool, enqueue_task, task vuoto\n");
		return -2;
	}
	tasklist_t *newtask=malloc(sizeof(tasklist_t));
	if(newtask==NULL) {
		fprintf(stderr,"pool, enqueue_task, malloc fallita\n");
		return -3;
	}
	//creazione nuovo task
	strncpy(newtask->task,filename,MAX_PATHNAME_LEN);
	newtask->next=NULL;
	//inserimento in coda
	pthrad_mutex_lock(&pool->task_mtx);
	while(pool->queue_size==pool->cur_queue_size)	//attendi che ci sia spazio
		pthread_cond_wait(&pool->task_cond,&pool->task_mtx);
	pool->worker_tail->next=newtask;
	pool->worker_tail=newtask;
	pool->cur_queue_size++;
	pthread_cond_signal(&pool->worker_cond);	//segnala i thread della presenza di task in coda
	pthrad_mutex_unlock(&pool->task_mtx);
	return 0;	//terminato con successo
}

//Aggiunge thread worker
int add_workers(threadpool_t *pool, unsigned int num) {
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
	
	int i,res;	//valori ausiliari
	for(i=0;i<num;i++) {
		workerlist *new_worker=(workerlist*)malloc(sizeof(workerlist));
		if(new_worker==NULL) {	//controllo errore malloc
			fprintf(stderr,"pool, impossibile allocare memoria al thread\n");
			continue;
		}
		new_worker->next=NULL;
		res=pthread_create(&new_worker->thread,NULL,thread_func,(void*)pool);
		if(res) {	//controllo errore pthread_create
			fprintf(stderr,"pool, add_workers, errore in pthread_create() del thread %d, errore %d\n",i+1,res);
			free(new_worker);
			continue;
		}
		else {
			if(pool->worker_tail==NULL)	//primo thread
				pool->worker_head = pool->worker_tail = new_worker;
			else
				pool->worker_tail->next=new_worker;
			pool->worker_tail=new_worker;
			pthread_mutex_lock(&pool->worker_mtx);
			pool->num_threads++;
			pthread_mutex_unlock(&pool->worker_mtx);
		}
	}
	return 0;
}

//Rimuove thread worker
void remove_worker(threadpool_t *pool) {
	pthread_mutex_lock(&pool->worker_mtx);
	if(pool->num_threads>1)	//si diminuiscono i worker se ce n'e' piu' di uno nel pool
		pool->modify_thread_num--;
	pthread_mutex_unlock(&pool->worker_mtx);
}

//attende che il threadpool finisca di elaborare i task passati
void await_pool_completion(threadpool_t *pool) {
	if(pool==NULL || !pool->initialized) {
		fprintf(stderr,"pool, await_pool_completion su threadpool non inizializzato\n");
		return;
	}
	//TODO
	//la funzione e' chiamata da master quando ha terminato i task
	//questa fz. attende che i waiting thread siano pari al numero corrente di thread
	//quando lo sono, questa fz. chiama destroy_pool()
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
	//inizializzazione variabili di pool
	memset(pool,0,sizeof(*pool));
	pool->worker_head=NULL;
	pool->worker_tail=NULL;
	pool->tasks_head=NULL;
	pool->tasks_tail=NULL;
	pool->socket=malloc(strlen(socket)*sizeof(char));
	if(pool->socket==NULL) {
		fprintf(stderr,"pool, initialize_pool, fallita malloc di stringa socket\n");
		return NULL;
	}
	strncpy(pool->socket,socket,9);
	
	//inizializzazione mutex e cond
	pool->queue_size=queue_len;
	pthread_mutex_init(&pool->worker_mtx,NULL);
	pthread_cond_init(&pool->worker_cond,NULL);
	pthread_mutex_init(&pool->task_mtx,NULL);
	pthread_cond_init(&pool->task_cond,NULL);
	pool->running=1;
	
	//crea coda di produzione
	int res;
	res=add_workers(pool,pool_size);
	if(res) {
		fprintf(stderr,"pool, initialize_pool, add_workers esce con errore %d\n",res);
		pthread_mutex_destroy(&pool->worker_mtx);
		pthread_cond_destroy(&pool->worker_cond);
		pthread_mutex_destroy(&pool->task_mtx);
		pthread_cond_destroy(&pool->task_cond);
		free(pool->socket);
		free(pool);
		return NULL;
	}
	
	while(!pool->initialized);	//attesa attiva di inizializzazione pool
	
	return pool;
}
