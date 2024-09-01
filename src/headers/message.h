#ifndef MESSAGE
	#define MESSAGE
	
	#include "utils.h"
	
	#define MAX_PATHNAME_LEN 1+MAX_NAMELENGTH
	#define BUFFER_SIZE 512		//dimensione del buffer di invio dati
	
	//struct del tipo di messaggio che un worker deve inviare al collector
	typedef struct result {
		long total;
		char pathname[MAX_PATHNAME_LEN];
	} result_t;
	
#endif
