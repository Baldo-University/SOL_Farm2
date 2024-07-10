#ifndef MASTERTHREAD
	#define MASTERTHREAD
	
	#define DEFAULT_N 4
	#define DEFAULT_Q 8
	#define DEFUALT_T 0
	#define MAX_NAMELENGTH 255
	
	//funzionamento del master worker
	//int e' argc, char** e' argv, char* e' il nome della socket
	void masterthread(int,char**,char*);
#endif
