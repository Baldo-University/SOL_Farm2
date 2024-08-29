#ifndef MESSAGE
	#define MESSAGE
	
	#define MAX_PATHNAME_LEN 256
	
	//struct del tipo di messaggio che un worker deve inviare al collector
	typedef struct result_path {
		long result;
		char pathname[MAX_PATHNAME_LEN];
	} result_path_t;
	
#endif
