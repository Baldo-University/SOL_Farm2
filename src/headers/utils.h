#ifndef MY_UTILS
	#define MY_UTILS
	
	//lunghezza massima dei filename
	#define MAX_NAMELENGTH 255
	
	//test che controlla se il valore restituito e' pari ad un valore di errore
	#define ec_is(res,val,txt) \
		if((res)==val) {perror(txt); exit(EXIT_FAILURE);}
	//test che controlla se il valore restituito e' diverso da un valore di errore
	#define ec_isnot(res,val,txt) \
		if((res)!=val) {perror(txt); exit(EXIT_FAILURE);}
		
	//stampe di debug
	#ifdef DEBUG
		#define DEBUG(FMT, ...)	fprintf(stderr,FMT,__VA_ARGS__)
		#define DEBUG_PERROR(string) perror(string)
	#else
		#define DEBUG(FMT, ...)
		#define DEBUG_PERROR(string)
	#endif
	
#endif
