#ifndef THREADPOOL
	#define THREADPOOL
	
	//Struttura principale del threadpool
	
	//linked list di thread worker
	typedef struct workerlist workerlist_t;
		
	//linked list di task
	typedef struct tasklist tasklist_t;

	typedef struct threadpool {	//struct threadpool;
		_Atomic unsigned short running;		//coda attiva o no
		char *socket;						//socket a cui si devono collegare i worker
		workerlist_t *worker_head;			//puntatore di testa alla lista dei worker
		workerlist_t *worker_tail;			//puntatore di coda alla suddetta lista
		unsigned int num_threads;			//numero totale di worker, minimo 1
		unsigned int started_threads;		//thread inizializzati e pronti ad eseguire task
		unsigned int remove_threads;		//worker da rimuovere
		unsigned int threadID;				//singolo identificatore dei thread
		pthread_mutex_t worker_mtx;			//mutex della lista di worker
		pthread_cond_t worker_cond;			//condizione usata per verificare se un worker giri o meno
		tasklist_t *tasks_head;				//coda di produzione
		tasklist_t *tasks_tail;				//ultimo elemento della coda
		size_t queue_size;					//lunghezza della coda
		size_t cur_queue_size;				//dimensione corrente della coda 
		pthread_mutex_t task_mtx;			//mutex coda task
		pthread_cond_t empty_queue_cond;	//condizione per attesa che coda non sia piu' vuota
		pthread_cond_t full_queue_cond;		//condizione per attesa che coda non sia piu' piena
	} threadpool_t;

	//inizializzazione threadpool
	//param: numero di worker iniziali, lunghezza coda, socket
	threadpool_t *initialize_pool(long, size_t, char*);
	
	//attende che il threadpool finisca di elaborare i task passati
	int await_pool_completion(threadpool_t*);
	
	//Inserisce task in coda. Usato da masterworker
	int enqueue_task(threadpool_t*,char*);
	
	//Aggiunge un thread worker
	void add_worker(threadpool_t*);
	
	//Rimuove un thread worker
	void remove_worker(threadpool_t*);
	
#endif
