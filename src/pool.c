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
#include <unistd.h>

#include "pool.h"
#include "utils.h"
#include "workfun.h"

//lunghezza max di stringa aumentata di 1 per accomodare il carattere di terminazione
#define MAX_PATHNAME_LEN 1+MAX_NAMELENGTH

//linked list di thread worker
struct workerlist {
	pthread_t worker;
	struct workerlist *next;
};

//linked list di task
struct tasklist {
	char task[MAX_PATHNAME_LEN];	//nome file da elaborare
	int endtask;					//0 se task normale, 1 se task conclusivo
	struct tasklist *next;
};

//struct del tipo di messaggio da inviare al collector
typedef struct result_path {
	long result;
	char *pathname;
} result_path_t;

//funzionamento del singolo thread
static void *thread_func(void *arg) {
	fprintf(stderr,"Worker: inizializzazione parte\n");
	threadpool_t *pool=(threadpool_t*)arg;
	pthread_mutex_lock(&pool->worker_mtx);
	unsigned int id=++pool->threadID;
	pthread_mutex_unlock(&pool->worker_mtx);
	char taskname[MAX_PATHNAME_LEN];	//nome del task consumato dalla coda
	long result=0;			//risultato della funzione workfun()
	tasklist_t *aux=NULL;	//puntatore ausiliario per l'eliminazione di task dalla coda
	
	fprintf(stderr,"Worker %d: inizializzato, pronto a svolgere task\n",id);
	
	//connessione al collector TODO
	
	pthread_mutex_lock(&pool->worker_mtx);
	pool->started_threads++;	//il thread viene contato come partito
	pthread_cond_signal(&pool->worker_cond);	//avvisa add_worker()
	pthread_mutex_unlock(&pool->worker_mtx);
	
	while(pool->running) {
		//fprintf(stdout,"worker %d: in task loop\n",id);
		pthread_mutex_lock(&pool->worker_mtx);
		if(pool->remove_threads>0) {	//bisogna rimuovere un thread, quello corrente esce dal loop
			pool->remove_threads--;
			pool->num_threads--;
			pthread_mutex_unlock(&pool->worker_mtx);
			break;
		}
		pthread_mutex_unlock(&pool->worker_mtx);
		memset(taskname,0,MAX_PATHNAME_LEN);	//rimuove il task precedente
		pthread_mutex_lock(&pool->task_mtx);
		while(pool->tasks_head==NULL) {	//nessun task in coda
			//fprintf(stdout,"worker %d: nessun task in coda\n",id);
			pthread_cond_wait(&pool->empty_queue_cond,&pool->task_mtx);	//si mette in attesa di task
		}
		//controlla se il task in coda sia quello conclusivo
		if(pool->tasks_head->endtask) {
			fprintf(stdout,"Worker %d: trovato task finale\n",id);
			pthread_mutex_unlock(&pool->task_mtx);
			break;
		}
		//thread prende ed esegue il task
		strncpy(taskname,pool->tasks_head->task,MAX_PATHNAME_LEN);
		aux=pool->tasks_head;
		pool->tasks_head=pool->tasks_head->next;
		pool->cur_queue_size--;
		if(pool->tasks_head==NULL) {
			pool->tasks_tail=NULL;
			fprintf(stdout,"Worker %d: coda svuotata\n",id);
		}
		free(aux);
		pthread_cond_signal(&pool->full_queue_cond); //segnala la presenza di spazio libero nella coda
		pthread_mutex_unlock(&pool->task_mtx);
		result=workfun(taskname);
		if(result<0)
			fprintf(stderr,"Worker %d: errore di workfun %ld\n",id,result);
		else
			fprintf(stderr,"Worker %d: il risultato di %s e' %ld\n",id,taskname,result);
	}
	
	//disconnessione dal collector TODO
	fprintf(stderr,"worker %d: uscita\n",id);
	pthread_exit((void*)NULL);
}

//Inserisce task in coda
int enqueue_task(threadpool_t *pool, char* filename) {
	if(pool==NULL) {
		fprintf(stderr,"pool, enqueue_task, pool non inizializzato\n");
		return -1;
	}
	if(filename==NULL) {
		fprintf(stderr,"pool, enqueue_task, task vuoto\n");
		return -2;
	}
	tasklist_t *newtask=malloc(sizeof(tasklist_t));
	if(newtask==NULL) {
		fprintf(stderr,"pool, enqueue_task, non e' stato possibile allocare memoria al task\n");
		return -3;
	}
	//fprintf(stderr,"pool: enqueue parte\n");
	//creazione nuovo task
	strncpy(newtask->task,filename,MAX_PATHNAME_LEN);
	newtask->endtask=0;		//task non finale
	newtask->next=NULL;
	//inserimento in fondo alla coda di produzione
	//fprintf(stderr,"pool: enqueue prelock\n");
	pthread_mutex_lock(&pool->task_mtx);
	//fprintf(stderr,"pool: enqueue postlock\n");
	while(pool->queue_size==pool->cur_queue_size) {	//attendi che ci sia spazio
		//fprintf(stderr,"pool: enqueue coda piena\n");
		pthread_cond_wait(&pool->full_queue_cond,&pool->task_mtx);
	}
	//fprintf(stderr,"pool: enqueue pre inserimento\n");
	if(pool->tasks_head==NULL)	//coda di produzione vuota
		pool->tasks_head=pool->tasks_tail=newtask;
	else {	//inserimento in coda non vuota
		pool->tasks_tail->next=newtask;
		pool->tasks_tail=newtask;
	}
	fprintf(stderr,"pool: inserito task %s\n",filename);
	pool->cur_queue_size++;
	//fprintf(stderr,"pool: enqueue task inserito\n");
	pthread_cond_signal(&pool->empty_queue_cond);	//segnala i thread della presenza di task in coda
	//fprintf(stderr,"pool: enqueue preunlock\n");
	pthread_mutex_unlock(&pool->task_mtx);
	//fprintf(stderr,"pool: enqueue postunlock\n");
	return 0;	//terminato con successo
}


//crea un worker nel pool
void add_worker(threadpool_t *pool) {
	if(pool==NULL) {
		fprintf(stderr,"pool, add_worker, threadpool non inizializzato\n");
		return;
	}
	workerlist_t *new_worker=malloc(sizeof(workerlist_t));
	if(new_worker==NULL) {
		fprintf(stderr,"pool, add_worker, non e' stato possibile allocare memoria al worker\n");
		return;
	}
	new_worker->next=NULL;
	int res=pthread_create(&new_worker->worker,NULL,thread_func,(void*)pool);
	if(res) {
		fprintf(stderr,"pool, add_worker, errore in pthread_create(), errore %d\n",res);
		free(new_worker);
		return;
	}
	pthread_mutex_lock(&pool->worker_mtx);
	if(pool->worker_tail==NULL)
		pool->worker_head=pool->worker_tail=new_worker;
	else
		pool->worker_tail->next=new_worker;
	pool->worker_tail=new_worker;
	pool->num_threads++;
	while(pool->num_threads!=pool->started_threads)	//aspetta finche' worker creato non possa entrare in loop
		pthread_cond_wait(&pool->worker_cond,&pool->worker_mtx);
	pthread_mutex_unlock(&pool->worker_mtx);
	fprintf(stderr,"pool, addworker ha inserito un thread attivo nel pool\n");
	return;
}

//aumenta di uno il numero di thread da rimuovere dal pool in seguito a SIGUSR2
void remove_worker(threadpool_t *pool) {
	pthread_mutex_lock(&pool->worker_mtx);
	if(pool->num_threads>1)	//si diminuiscono i worker se ce n'e' piu' di uno nel pool
		pool->remove_threads++;
	pthread_mutex_unlock(&pool->worker_mtx);
}

//attende che il threadpool finisca di elaborare i task passati
long await_pool_completion(threadpool_t *pool) {
	if(pool==NULL) {
		fprintf(stderr,"pool, await_pool_completion su threadpool non inizializzato\n");
		return -1;
	}
	fprintf(stderr,"closepool: inizio\n");
	//salva il numero di worker al momento della chiamata
	pthread_mutex_lock(&pool->worker_mtx);
	long workersatexit=pool->num_threads;
	pthread_mutex_unlock(&pool->worker_mtx);
	fprintf(stderr,"closepool: salvato numero di worker alla fine\n");
	
	//inserisce il task finale in coda
	tasklist_t *finaltask=malloc(sizeof(tasklist_t));
	if(finaltask==NULL) {	//fatal error!
		fprintf(stderr,"pool, await_pool_completion, non e' stato possibile allocare memoria al task fianle \n");
		pool->running=0;
		return -1;
	}
	fprintf(stderr,"closepool: allocazione memoria del finaltask\n");
	finaltask->endtask=1;
	fprintf(stderr,"closepool: endtask settato\n");
	finaltask->next=NULL;
	fprintf(stderr,"closepool: task successivo al finale settato a NULL\n");
	pthread_mutex_lock(&pool->task_mtx);
	fprintf(stderr,"closepool: lock dei task\n");
	if(pool->tasks_head==NULL) {
		fprintf(stderr,"closepool: inserimento task finale in coda vuota\n");
		pool->tasks_head=pool->tasks_tail=finaltask;	//NB: si aggiunge senza controllare la lungh. di coda
	}
	else {
		fprintf(stderr,"closepool: inserimento task finale in coda non vuota\n");
		pool->tasks_tail->next=finaltask;
		fprintf(stderr,"closepool: inserito task finale\n");
		pool->tasks_tail=finaltask;
		fprintf(stderr,"closepool: puntatore di fine coda spostato al task finale\n");
	}
	fprintf(stderr,"closepool: inserimento task finale completato\n");
	pthread_cond_broadcast(&pool->empty_queue_cond);	//segnala tutti i thread della presenza di task in coda
	fprintf(stderr,"closepool: broadcast\n");
	pthread_mutex_unlock(&pool->task_mtx);
	fprintf(stderr,"closepool: unlock dei task\n");
	//dealloca i worker
	workerlist_t *aux=pool->worker_head;
	fprintf(stderr,"closepool: allocazione memoria del finaltask\n");
	while(pool->worker_head!=NULL) {
		fprintf(stderr,"closepool: loop join thread\n");
		pthread_join(pool->worker_head->worker,NULL);
		pool->worker_head=pool->worker_head->next;
		free(aux);
		aux=pool->worker_head;
	}
	free(pool->tasks_head);	//dealloca il task conclusivo
	fprintf(stderr,"closepool: rimosso task finale\n");
	
	//distruggi mutex e cond
	if(pthread_mutex_destroy(&pool->worker_mtx))
		perror("pool, destroy_pool, pthread_mutex_destroy worker_mtx");
	if(pthread_mutex_destroy(&pool->task_mtx))
		perror("pool, destroy_pool, pthread_mutex_destroy task_mtx");
	if(pthread_cond_destroy(&pool->worker_cond))
		perror("pool, destroy_pool, pthread_cond_destroy worker_cond");
	if(pthread_cond_destroy(&pool->empty_queue_cond))
		perror("pool, destroy_pool, pthread_cond_destroy empty_queue_cond");
	if(pthread_cond_destroy(&pool->full_queue_cond))
		perror("pool, destroy_pool, pthread_cond_destroy full_queue_cond");
	free(pool->socket);
	free(pool);
	return workersatexit;
}

threadpool_t *initialize_pool(long pool_size, size_t queue_len, char* socket) {
	fprintf(stdout,"Pool: parte l'inizializzazione\n");
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
		fprintf(stderr,"pool, initialize_pool, non e' stato possibile allocare memoria alla stringa del socket\n");
		free(pool);
		return NULL;
	}
	strncpy(pool->socket,socket,strlen(socket));
	pool->queue_size=queue_len;
	
	//inizializzazione mutex e cond
	pthread_mutex_init(&pool->worker_mtx,NULL);
	pthread_cond_init(&pool->worker_cond,NULL);
	pthread_mutex_init(&pool->task_mtx,NULL);
	pthread_cond_init(&pool->empty_queue_cond,NULL);
	pthread_cond_init(&pool->full_queue_cond,NULL);
	pool->running=1;
	
	//crea coda di produzione
	int i;
	for(i=0;i<pool_size;i++)
		add_worker(pool);
	pthread_mutex_lock(&pool->worker_mtx);
	if(pool->num_threads==0) {
		fprintf(stderr,"pool, initialize_pool, threadpool parte con zero worker\n");
		pthread_mutex_unlock(&pool->worker_mtx);
		if(pthread_mutex_destroy(&pool->worker_mtx))
			perror("pool, initialize_pool, pthread_mutex_destroy worker_mtx");
		if(pthread_mutex_destroy(&pool->task_mtx))
			perror("pool, initialize_pool, pthread_mutex_destroy task_mtx");
		if(pthread_cond_destroy(&pool->worker_cond))
			perror("pool, initialize_pool, pthread_cond_destroy worker_cond");
		if(pthread_cond_destroy(&pool->empty_queue_cond))
			perror("pool, initialize_pool, pthread_cond_destroy empty_queue_cond");
		if(pthread_cond_destroy(&pool->full_queue_cond))
			perror("pool, initialize_pool, pthread_cond_destroy full_queue_cond");
		free(pool->socket);
		free(pool);
		return NULL;
	}
	fprintf(stderr,"Pool: creati %d thread\n",pool->num_threads);
	pthread_mutex_unlock(&pool->worker_mtx);
	
	return pool;
}
