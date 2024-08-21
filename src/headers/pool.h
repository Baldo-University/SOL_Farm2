#ifndef THREADPOOL
	#define THREADPOOL
	
	//lunghezza max di stringa aumentata di 1 per accomodare il carattere di terminazione
	#define MAX_PATHNAME_LEN 256

	//linked list di thread worker
	typedef struct workerlist {
		pthread_t worker;
		struct workerlist *next;
	} workerlist_t;
	
	//linked list di task
	typedef struct tasklist {
		char task[MAX_PATHNAME_LEN];
		struct tasklist *next;
	} tasklist_t;
	
	//Struttura principale del threadpool
	typedef struct threadpool {
		volatile unsigned short initialized;	//coda inizializzata (per attesa attiva che i thread siano pronti)
		_Atomic unsigned short running;			//coda attiva o no
		char *socket;							//socket a cui si devono collegare i worker
		workerlist *worker_head;				//puntatore di testa alla lista dei worker
		workerlist *worker_tail;				//puntatore di coda alla suddetta lista
		unsigned int num_threads;				//numero totale di worker, minimo 1
		int modify_thread_num;					//worker da aggiungere o rimuovere
		volatile unsigned int waiting_workers;	//quanti thread sono in attesa di task
		unsigned int threadID;					//singolo identificatore dei thread
		pthread_mutex_t worker_mtx;				//worker attivo o idling
		pthread_cond_t worker_cond;				//condizione relativa al suddetto mutex
		tasklist_t *tasks_head;					//coda di produzione
		tasklist_t *tasks_tail;					//ultimo elemento della coda
		size_t queue_size;						//lunghezza della coda
		size_t cur_queue_size;					//dimensione corrente della coda 
		pthread_mutex_t task_mtx;				//mutex coda task
		pthread_cond_t task_cond;				//condizione di spazio presente in coda
	} threadpool_t;

	//inizializzazione threadpool
	//param: numero di worker iniziali, lunghezza coda, socket
	threadpool_t initialize_pool(long, size_t, char*);
	
	//attende che il threadpool finisca di elaborare i task passati
	void await_pool_completion(threadpool_t*);
	
	//distrugge il threadpool
	void destroy_pool(threadpool_t*);
	
	//Inserisce task in coda. Usato da masterworker
	void enqueue_task(threadpool_t*,char*);
	
	//Aggiunge thread worker
	int add_workers(threadpool_t*,unsigned int);
	
	//Rimuove thread worker
	void remove_worker(threadpool_t*);
	
#endif
