#ifndef THREADPOOL
	#define THREADPOOL
	
	typedef struct ThreadPool ThreadPool;
	
	//crea il threadpool
	ThreadPool *create_pool(unsigned int);
	
	//attende che il threadpool finisca di elaborare i task passati
	void await_completion(ThreadPool*);
	
	//distrugge il threadpool
	void destroy_pool(Threadpool*);
	
	//Inserisce task in coda
	void enqueue_task(Threadpool*,char*);
	
	void add_worker(Threadpool*);
	void remove_worker(Threadpool*);
	
#endif
