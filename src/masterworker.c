/*
Questa sezione di codice contiene il thread MasterWorker.
Il masterworker prende i filename passati da linea di comando e le opzioni.
Crea un thread che gestisca i segnali in modo sincrono (TODO)
Crea un thread che inserisca i filepath in una lista (TODO)
Crea un thread che tolga i file dalla suddetta lista e li metta sulla coda di produzione (TODO)
Crea il threadpool di worker (TODO)
*/

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "masterworker.h"
#include "utils.h"

//struct dei file da valutare.
typedef struct node {
	char name[MAX_NAMELENGTH];	//filename
	struct node *next;			//prossimo elemento di lista
} node_t;

//struct necessaria per cercare file delle directory passate con -d usando un thread
typedef struct filesearch_arg {
	int argind;				//optind
	int argc;
	char **argv;
	node_t *files;			//lista di file
	pthread_mutex_t *mtx;	//lock mutex sulla lista di file
	node_t *directories;	//lista di directory
} filesearch_arg_t;


//gestore sincrono di segnali
static void *sighandler(void *arg) {
	sigset_t *set=(sigset_t*)arg;
	
	for(;;) {
		int sig;
		int r=sigwait(set,&sig);
		if(r!=0) {
			errno=r;
			perror("ERRORE FATALE 'sigwait'");
			//TODO decidere cosa fare se sigwait fallisce
		}
		switch(sig) {
		case SIGHUP:	//chiusura terminale
		case SIGINT:
		case SIGQUIT:
		case SIGTERM:	//stesso comportamento per questi quattro segnali
			
			break;
		case SIGUSR1:	//incrementa di uno i thread
			
			break;
		case SIGUSR2:	//decrementa di uno i thread (minimo 1 thread)
			
			break;
		default: ;
		}
	}
	return NULL;
}

//Aggiunta di un elemento in testa ad una lista
void list_add(node_t **head, char *name) {
	node_t *new=malloc(sizeof(node_t));
	ec_is(new,NULL,"masterworker, listadd, malloc");
	strncpy(new->name,name,MAX_NAMELENGTH);
	node_t *aux=*head;	//aggiunta in testa
	new->next=*head;
	*head=new;
}

void printlist(node_t **head) {
	node_t *aux=*head;
	while(aux!=NULL && aux->next!=NULL) {
		fprintf(stdout,"%s\n",aux->name);
	}
}

//Aggiunta di file binari alla lista appropriata
static void *file_search(filesearch_arg *thread_args) {
	int i;
	FILE *file;
	struct stat info;
	for(i=thread_args->argind;i<thread_args->numargs;i++) {
		ec_is(file=fopen(thread_args.argv[i],"r"),NULL,"masterworker, file_search, fopen");
		ec_is(stat(thread_args.argv[i],&info),-1,"masterworker, file_search, stat");
		if(!S_ISREG(info.st_mode)) {	//se il file non e' binario si chiude
			ec_is(fclose(file),EOF,"masterworker, file_search, fclose");
			continue;
		}
		ec_isnot(pthread_mutex_lock(args->mtx),0,"masterworker, file_search, lock");
		list_add(thread_args.files,thread_args.argv[i],1);
	}
}

void masterworker(int argc, char *argv[], char *socket) {
	long workers=DEFAULT_N;
	size_t queue_length=DEFAULT_Q;
	long queue_delay=DEFAULT_T;
	
	fprintf(stdout,"---MasterWorker Parte---\n");
	fflush(stdout);
	
	/*Gestione segnali*/

	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask,SIGHUP);
	sigaddset(&mask,SIGINT);
	sigaddset(&mask,SIGQUIT);
	sigaddset(&mask,SIGTERM);
	//sigaction per ignorare SIGPIPE
	struct sigaction s;
	memset(&s,0,sizeof(s));
	s.sa_handler=SIG_IGN;
	ec_is(sigaction(SIGPIPE,&s,NULL),-1,"masterworker, sigaction");
	fprintf(stdout,"Segnali settati\n");
	fflush(stdout);
	//thread dedicato alla gestione sincrona dei segnali TODO
	
	
	//lista di directory passate con -d o all'interno delle suddette.
	node_t *directories=NULL;		//lista di filename
	node_t *directories_aux=directories;	//puntatore ausiliario
	
	
	/*Analisi delle opzioni*/
	int opt;
	long optlong;
	while((opt=getopt(argc,argv,"n:q:d:t:"))!=-1) {
		switch(opt) {
		
		//cambia il numero di thread
		case 'n':
			if(!(optarg && *optarg)) {
				fprintf(stderr,"Errore nella lettura dell'opzione -%c.\n",opt);
				break;
			}
			optlong=strtol(optarg,NULL,10);
			//controlla che il valore passato sia valido
			if(errno==ERANGE || optlong<MIN_THREADS)	//di default, MIN_THREADS e' 1
				fprintf(stderr,"Ignorata opzione -%c non valida (numero di thread worker).\n",opt);
			else
				workers=optlong;
			break;
		
		//modifica la lunghezza della coda di produzione
		case 'q':
			if(!(optarg && *optarg)) {
				fprintf(stderr,"Errore nella lettura dell'opzione -%c.\n",opt);
				break;
			}
			optlong=strtol(optarg,NULL,10);
			if(errno==ERANGE || optlong<=0)
				fprintf(stderr,"Ignorata opzione -%c non valida (dimensione coda di produzione).\n",opt);
			else
				queue_length=optlong;
			break;
		
		//passa una directory in cui cercare ricorsivamente i file destinati alla coda di produzione
		case 'd':
			if(!(optarg && *optarg)) {
				fprintf(stderr,"Errore nella lettura dell'opzione -%c.\n",opt);
				break;
			}
			//controllo lunghezza pathname accettabile
			if(strlen(optarg)>MAX_NAMELENGTH) {
				fprintf(stderr,"Opzione -%c, pathname troppo lungo scartato.\n",opt);
				break;
			}
			//controlla che la stringa passata indichi una directory
			struct stat info;
			ec_is(stat(optarg,&info),-1,"masterworker, getopt, stat");
			if(!S_ISDIR(info.st_mode)) {
				fprintf(stderr,"Opzione -%c, pathname indicato %s non e' directory.\n",opt,optarg);
				break;
			}
			//aggiunge la directory alla lista di directory in cui fare la ricerca di file
			list_add(&directories,optarg);
			break;
		
		//introduce ritardo di inserimento in coda
		case 't':
			if(!(optarg && *optarg)) {
				fprintf(stderr,"Errore nella lettura dell'opzione -%c.\n",opt);
				break;
			}
			optlong=strtol(optarg,NULL,10);
			if(errno==ERANGE || optlong<0)
				fprintf(stderr,"Ignorata opzione -%c non valida (attesa di inserimento in coda di produzione).\n",opt);
			else
				queue_delay=optlong;
			break;
		default:
			fprintf(stderr,"Passata opzione -%c non riconosciuta.\n",opt);
		}
	}

	//lancia thread che inserisce file nella lista di file
	pthread_t file_finder;
	filesaerch_arg_t file_finder_arg
	file_finder_arg.argind=optind;
	file_finder_arg.argc=argc;
	file_finder_arg.argv=argv;
	file_finder_arg.files=files;
	file_finder_arg.mtx=files_mtx;
	file_finder_arg.directories=directories;
	ec_isnot(pthread_create(&file_finder,NULL,&file_search,&file_finder_args),0,"masterworker, pthread_create");
	
	fprintf(stdout,"Numero thread: %ld\nLunghezza coda: %ld\nRitardo di inserimento: %ld\n",workers,queue_length,queue_delay);
	fflush(stdout);
}
