#ifndef MASTERTHREAD
	#define MASTERTHREAD
	
	#define DEFAULT_N 4
	#define DEFAULT_Q 8
	#define DEFAULT_T 0
	#define MIN_THREADS 1	//numero minimo di thread
	
	//funzionamento del master worker
	//int e' argc, char** e' argv, char* e' il nome della socket
	void masterworker(int,char**,char*);
#endif
