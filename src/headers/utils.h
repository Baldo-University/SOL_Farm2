#ifndef MY_UTILS
	#define MY_UTILS
	
	//test che controlla se il valore restituito e' pari ad un valore di errore
	#define ec_is(res,val,txt) \
		if((res)==val) {perror(txt); exit(EXIT_FAILURE);}
	//test che controlla se il valore restituito e' diverso da un valore di errore
	#define ec_isnot(res,val,txt) \
		if((res)!=val) {perror(txt); exit(EXIT_FAILURE);}
#endif
