/*SCRIVERE DESCRIZIONE*/

#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "masterworker.h"
#include "pool.h"
#include "utils.h"

#define MAX_PATHNAME_LEN 1+MAX_NAMELENGTH

//valore pari ad 1 quando parte, settato a zero solo quando viene terminato prima di terminare
static int running;
static pthread_mutex_t running_mtx=PTHREAD_MUTEX_INITIALIZER;
//Contatore di segnali SIGUSR, inizialmente zero
static int thread_num_change;
static pthread_mutex_t thread_num_mtx=PTHREAD_MUTEX_INITIALIZER;

//struct dei file da valutare.
typedef struct node {
	char name[MAX_PATHNAME_LEN];	//filename
	struct node *next;			//prossimo elemento di lista
} node_t;

//gestore sincrono di segnali
static void *sighandler(void *arg) {
	sigset_t *set=(sigset_t*)arg;
	
	for(;;) {
		int sig;
		int r=sigwait(set,&sig);
		if(r!=0) {
			errno=r;
			perror("ERRORE FATALE 'sigwait'");
			ec_isnot(pthread_mutex_lock(&running_mtx),0,"masterworker, sighandler, lock");
			running=0;
			ec_isnot(pthread_mutex_unlock(&running_mtx),0,"masterworker, sighandler, unlock");
			return NULL;
		}
		switch(sig) {
		//se riceve tali segnali, si smette di inserire flie in coda.
		//il programma esaurisce i file in coda e termina come farebbe normalmente
		case SIGHUP:	//segnale di chiusura del terminale
		case SIGINT:
		case SIGQUIT:
		case SIGTERM:
			ec_isnot(pthread_mutex_lock(&running_mtx),0,"masterworker, sighandler, lock running");
			running=0;
			ec_isnot(pthread_mutex_unlock(&running_mtx),0,"masterworker, sighandler, mutex unlock running");
			return NULL;
		
		case SIGUSR1:	//incrementa di uno i worker
			ec_isnot(pthread_mutex_lock(&thread_num_mtx),0,"masterworker, sighandler, lock thread_num SIGUSR1");
			thread_num_change++;
			ec_isnot(pthread_mutex_unlock(&thread_num_mtx),0,"masterworker, sighandler, unlock thread_num SIGUSR1");
			break;
		
		case SIGUSR2:	//decrementa di uno i worker (NB: minimo 1 thread nel pool)
			ec_isnot(pthread_mutex_lock(&thread_num_mtx),0,"masterworker, sighandler, lock thread_num SIGUSR2");
			thread_num_change--;
			ec_isnot(pthread_mutex_unlock(&thread_num_mtx),0,"masterworker, sighandler, unlock thread_num SIGUSR2");
			break;
		
		default: ;
		}
	}
	return NULL;
}

//Aggiunta di un elemento in testa ad una lista
void list_add_head(node_t **head, char *name) {
	node_t *new=malloc(sizeof(node_t));
	ec_is(new,NULL,"masterworker, listadd, malloc");
	strncpy(new->name,name,MAX_NAMELENGTH);
	new->next=*head;
	*head=new;
}

void dirs_add(node_t **head, char *name, char *fulldirname) {
	node_t *new=malloc(sizeof(node_t));
	ec_is(new,NULL,"masterworker, listadd, malloc");
	snprintf(new->name,MAX_PATHNAME_LEN,"%s/%s",fulldirname,name);	//il nome e' il pathname relativo
	new->next=NULL;
	node_t *aux=*head;
	if(*head==NULL) {
		*head=new;
		return;
	}
	while(aux->next!=NULL)
		aux=aux->next;
	aux->next=new;
}

/*
//stampa lista (tenere?)
void list_print(node_t *head) {
	node_t *aux=head;
	while(aux!=NULL) {
		fprintf(stdout,"%s\n",aux->name);
		aux=aux->next;
	}
}
*/

//deallocazione memoria lista
void list_free(node_t *head) {
	node_t *aux=head;;
	while(aux!=NULL) {
		head=head->next;
		free(aux);
		aux=head;
	}
}

void masterworker(int argc, char *argv[], char *socket) {
	fprintf(stderr,"---MasterWorker Parte---\n");
	
	running=1;
	thread_num_change=0;
	
	//gestione segnali
	sigset_t mask;
	ec_is(sigemptyset(&mask),-1,"masterworker, sigemptyset");
	ec_is(sigaddset(&mask,SIGHUP),-1,"masterworker, sigaddset SIGHUP");
	ec_is(sigaddset(&mask,SIGINT),-1,"masterworker, sigaddset SIGINT");
	ec_is(sigaddset(&mask,SIGQUIT),-1,"masterworker, sigaddset SIGQUIT");
	ec_is(sigaddset(&mask,SIGTERM),-1,"masterworker, sigaddset SIGTERM");
	ec_is(sigaddset(&mask,SIGUSR1),-1,"masterworker, sigaddset SIGUSR1");
	ec_is(sigaddset(&mask,SIGUSR2),-1,"masterworker, sigaddset SIGUSR2");
	//impostazione della nuova mask
	ec_isnot(pthread_sigmask(SIG_BLOCK,&mask,NULL),0,"masterworker, pthread_sigmask FATAL ERROR");
	
	//sigaction per ignorare SIGPIPE
	struct sigaction s;
	memset(&s,0,sizeof(s));
	s.sa_handler=SIG_IGN;
	ec_is(sigaction(SIGPIPE,&s,NULL),-1,"masterworker, sigaction");
	
	//thread detached per la gestione sincrona dei segnali
	pthread_t sighandler_thread;
	ec_isnot(pthread_create(&sighandler_thread,NULL,&sighandler,&mask),0,"masterworker, pthread_create sighandler");
	ec_isnot(pthread_detach(sighandler_thread),0,"masterworker, pthread_detach");
	
	//dichiarazione ed iniaizlizzazione delle variabili default. Se necessario verranno sovrascritte
	long workers=DEFAULT_N;
	size_t queue_length=DEFAULT_Q;
	long queue_delay=DEFAULT_T;
	
	node_t *directories=NULL;	//lista di directory passate con -d o all'interno delle suddette.
	node_t *aux=directories;	//puntatore ausiliario
	
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
				queue_length=(size_t)optlong;
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
			list_add_head(&directories,optarg);
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

	//creazione threadpool
	threadpool_t pool=initialize_pool(workers,queue_length,&socket);
	ec_is(pool,NULL,"masterworker, initialize_pool");
	
	//imposta il ritardo di inserimento
	struct timespec delay_struct, delay_rem;
	delay_struct.tv_sec=queue_delay/1000;
	delay_struct.tv_nsec=(queue_delay%1000)*1000000;

	/*FILEFINDER*/
	//ricerca file da linea di comando
	int i;
	for(i=optind;i<argc;i++) {
		//se bisogna chiudere anticipatamente il programma
		if(!running)
			break;
			
		//se bisogna cambiare il numero di worker
		//TODO
		
		struct stat info;
		int check_stat=stat(argv[i],&info);
		if(check_stat==-1) {
			perror("masterworker, stat di file regolare");
			ec_isnot(pthread_mutex_lock(&running_mtx),0,"masterworker, sighandler, lock running");
			running=0;
			ec_isnot(pthread_mutex_unlock(&running_mtx),0,"masterworker, sighandler, mutex unlock running");
			break;
		}
		if(!S_ISREG(info.st_mode))
			fprintf(stderr,"Passato file %s non regolare da linea di comando, scartato.\n",argv[i]);
		else {
			//inserisci in coda con ritardo appropriato
		}
	}
	
	//ricerca nelle directory -d
	while(directories!=NULL) {
		//se bisogna chiudere anticipatamente il programma
		if(!running) {
			list_free(directories);
			break;	
		}
		
		//se bisogna cambiare il numero di worker
		//TODO
		
		//apertura e ricerca nella directory corrente
		DIR *dir=opendir(directories->name);
		if(dir==NULL)
			fprintf(stderr,"Directory %s non trovata\n",directories->name);
		struct dirent *file;
		while((errno=0, file=readdir(dir))!=NULL) {
			//controllo se sono arrivati segnali di arresto
			if(!running) {
				//libera lo spazio delle directory successive, quella corrente viene liberata dopo il while
				list_free(directories->next); 
				break;
			}
			//controllo errno
			if(errno!=0) {
				fprintf(stderr,"Errore nell'apertura di %s\n",directories->name);
				ec_isnot(pthread_mutex_lock(&running_mtx),0,"masterworker, mutex lock in dirsearch");
				running=0;
				ec_isnot(pthread_mutex_unlock(&running_mtx),0,"masterworker, mutex unlock in dirsearch");
				break;
			}
			
			if(!strncmp(file->d_name,".",2) || !strncmp(file->d_name,"..",3))	//controlla che non siano directory padre
				continue;	//vai al prossimo file
				
			else if(file->d_type==DT_DIR) {	//trovata directory
				fprintf(stdout,"trovata directory %s\n",file->d_name);
			dirs_add(&directories,file->d_name,directories->name);	//aggiunta in coda per scorrimento breadth-first
			}
			
			else if(file->d_type==DT_REG) {	//trovato file normale
				//ritardo di inserimento (versione basic)
				if(sleep_result==0) {
					sleep_result=nanosleep(&delay_struct,&delay_rem);
					if(sleep_result==-1) {	//errore
						if(errno==EINTR)	//verifica se nanosleep interrotto da segnale
							errno=0;
					}
				}
				else {
					sleep_result=nanosleep(&delay_rem,&delay_rem);
					if(sleep_result==-1) {
						if(errno==EINTR)	//verifica se nanosleep interrotto da segnale
							errno=0;
					}
				}
				if(sleep_result!=0)		//nanosleep restituisce errore e non interrotto da segnale
					continue;
				//invio file al threadpool
				fprintf(stdout,"inserisco file %s\n",file->d_name);
			}
			
			else if(file->d_type==DT_UNKNOWN)
				perror("masterworker, file di tipo sconosciuto");
		}
		
		//chiusura directory
		ec_is(closedir(dir),-1,"masterworker, closedir");
		//libera la memoria e passa alla prossima directory. Fine qui se non ci sono altre directory
		aux=directories;
		directories=directories->next;
		free(aux);
	}
	
	//finito filefinder, se TP ancora attivo mettiti in attesa con while sul running di TP
	//Il while contiene un if su thread_num_change, se !=0 cambia il numero di thread
	//Dopodiche' chiudi e pulisci quello che resta da pulire
	
	
	
	//chiusura forzata del signal handler thread
	if(running) 
		ec_isnot(pthread_kill(sighandler_thread,SIGQUIT),0,"masterworker, pthread_kill di signal handler");
}

/*DEBUG PRINTS
	fprintf(stdout,"Numero thread: %ld\nLunghezza coda: %ld\nRitardo di inserimento: %ld millisecondi\n",workers,queue_length, queue_delay);
	int i;
	fprintf(stdout,"File passati direttamente da linea di comando:\n");
	for(i=optind;i<argc;i++)
		fprintf(stdout,"%s\n",argv[i]);
	fflush(stdout);
	list_free(directories);
	*/
	/*
	//creazione nworkeratexit
	FILE *workers_file=fopen("nworkeratexit.txt","w");
	if(workers_file==NULL)
		perror("masterworker, fopen nworkeratexit");
	else {
		fprintf(workers_file,"%d\n",);	//inserisci il numero di thread alla fine / INCOMPLETO
		fclose(workers_file);
	}
	*/
