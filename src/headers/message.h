#ifndef MESSAGE	
	#define MESSAGE
	
	#include "utils.h"
	
	#define MAX_PATHNAME_LEN 1+MAX_NAMELENGTH
	
	#define UNIX_PATH_MAX 108	//lunghezza massima pathname socket
	#define BUFFER_SIZE 264		//dimensione del buffer di invio dati
	
	typedef struct message {
		char name[MAX_PATHNAME_LEN];
		long total;
	} message_t;
	
#endif
