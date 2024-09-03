#ifndef MESSAGE	
	#define MESSAGE
	
	#include "utils.h"
	
	#define MAX_PATHNAME_LEN 1+MAX_NAMELENGTH
	
	typedef struct message {
		char name[MAX_PATHNAME_LEN];
		long total;
	} message_t;
	
#endif
