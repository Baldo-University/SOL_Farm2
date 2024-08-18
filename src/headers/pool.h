#ifndef THREADPOOL
	#define THREADPOOL
	
	//linked list di thread worker
	typedef struct workerlist {
		pthread_t worker;
		struct workerlist *next;
	} workerlist_t;
	
	//Struttura principale del threadpool
	typedef struct threadpool {
		volatile unsigned short initialized;	//coda inizializzata (per attesa attiva che i thread siano pronti)
		_Atomic unsigned short running;			//coda attiva o no
		workerlist *worker_head;				//puntatore di testa alla lista dei worker
		workerlist *worker_tail;				//puntatore di coda alla suddetta lista
		unsigned int num_threads;				//numero totale di worker, minimo 1
		int modify_thread_num;					//worker da aggiungere o rimuovere
		volatile unsigned int waiting_workers;	//quanti thread sono in attesa di task
		unsigned int threadID;					//singolo identificatore dei thread
		pthread_mutex_t worker_mtx;				//worker attivo o idling
		pthread_cond_t worker_cond;				//condizione relativa al suddetto mutex
		char **tasks;							//coda di produzione di tipo circolare
		size_t queue_size;						//lunghezza della coda
		int tasks_head;							//indice task in testa
		int tasks_tail;							//indice task in coda
		pthread_mutex_t task_mtx;				//mutex coda task
		pthread_cond_t task_full;				//condizione coda piena
		char *socket;							//socket a cui si devono collegare i worker
	} threadpool_t;

	//inizializzazione threadpool
	//param: numero di worker iniziali, lunghezza coda, socket
	threadpool_t initialize_pool(long, size_t, char*);
	
	//attende che il threadpool finisca di elaborare i task passati
	void await_pool_completion();
	
	//distrugge il threadpool
	void destroy_pool();
	
	//Inserisce task in coda. Usato da masterworker
	void enqueue_task(char*);
	
	//Aggiunge thread worker
	int add_workers(threadpool_t*,long);
	
	//Rimuove thread worker
	void remove_worker();
	
#endif
