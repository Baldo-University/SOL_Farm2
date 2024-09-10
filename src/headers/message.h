#ifndef MESSAGE	
	#define MESSAGE
	
	#include "utils.h"
	
	#ifndef MAX_PATHNAME_LEN
		#define MAX_PATHNAME_LEN 1+MAX_NAMELENGTH
	#endif
	
	#define UNIX_PATH_MAX 108	//lunghezza massima pathname socket
	#define BUFFER_SIZE 264		//dimensione del buffer di invio dati
	#define DISCONNECT "close"	//messaggio di disconnessione
	
	typedef struct message {
		char name[MAX_PATHNAME_LEN];
		long total;
	} message_t;
	
#endif
