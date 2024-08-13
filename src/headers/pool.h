#ifndef THREADPOOL
	#define THREADPOOL
	
	//linked list di thread worker
	typedef struct workerlist {
		pthread_t worker;
		struct workerlist *next;
	} workerlist_t;
	
	//Struttura principale del threadpool
	typedef struct threadpool {
		volatile unsigned short initialized;	//coda inizializzata o no
		_Atomic unsigned short running;			//coda attiva o no
		workerlist *workers_head;	//puntatore di testa alla lista dei worker
		workerlist *workers_tail;	//puntatore di coda alla suddetta lista
		size_t num_threads;			//numero totale di worker
		size_t modify_thread_num;	//worker da aggiungere o rimuovere
		unsigned int threadID;		//singolo identificatore dei thread
		char **tasks;				//coda di produzione di tipo circolare
		int tasks_head;				//indice task in testa
		int tasks_tail;				//indice task in coda
		pthread_mutex_t task_mtx;	//mutex coda task
		pthread_cond_t task_full;	//condizione coda piena
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
	void add_worker();
	
	//Rimuove thread worker
	void remove_worker();
	
#endif
