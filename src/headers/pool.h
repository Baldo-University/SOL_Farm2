#ifndef THREADPOOL
	#define THREADPOOL
	
	#define MAX_NAMELENGTH 255
	
	//crea il threadpool
	//size_t: numero iniziale di worker
	//char*: nome socket
	//return: numero di worker
	int create_pool(size_t, char*);
	
	//attende che il threadpool finisca di elaborare i task passati
	void await_pool_completion();
	
	//distrugge il threadpool
	void destroy_pool();
	
	//Inserisce task in coda
	void enqueue_task(char*);
	
	void add_worker();
	void remove_worker();
	
#endif
