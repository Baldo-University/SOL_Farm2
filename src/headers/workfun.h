#ifndef WORKFUN
	#define WORKFUN
	
	typedef struct result_path {
		long result;
		char *pathname;
	} result_path_t;
	
	long workfun(char *filename);
	
#endif
