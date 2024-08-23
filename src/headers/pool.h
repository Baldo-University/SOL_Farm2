#ifndef THREADPOOL
	#define THREADPOOL
	
	//Struttura principale del threadpool
	typedef struct threadpool threadpool_t;

	//inizializzazione threadpool
	//param: numero di worker iniziali, lunghezza coda, socket
	threadpool_t initialize_pool(long, size_t, char*);
	
	//attende che il threadpool finisca di elaborare i task passati
	int await_pool_completion(threadpool_t*);
	
	//Inserisce task in coda. Usato da masterworker
	void enqueue_task(threadpool_t*,char*);
	
	//Aggiunge un thread worker
	void add_worker(threadpool_t*);
	
	//Rimuove un thread worker
	void remove_worker(threadpool_t*);
	
#endif
