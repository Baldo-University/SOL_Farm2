#ifndef THREADPOOL
	#define THREADPOOL
	
	typedef struct threadpool threadpool;
	
	//crea il threadpool
	//size_t: numero iniziale di worker
	//char*: nome socket
	//return: numero di worker
	int pool_function(size_t, size_t, char*);
	
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
