#ifndef MESSAGE
	#define MESSAGE
	
	#define MAX_PATHNAME_LEN 256
	
	//struct del tipo di messaggio che un worker deve inviare al collector
	typedef struct result {
		long total;
		char pathname[MAX_PATHNAME_LEN];
	} result_t;
	
#endif
