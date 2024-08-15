#ifndef WORKFUN
	#define WORKFUN
	
	typedef struct result_path {
		long result;
		char *pathname;
	} result_path_t;
	
	int workfun(char *filename);
	
#endif
