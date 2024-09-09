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
static int mw_running;
static pthread_mutex_t mw_running_mtx=PTHREAD_MUTEX_INITIALIZER;
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
			pthread_mutex_lock(&mw_running_mtx);
			mw_running=0;
			pthread_mutex_unlock(&mw_running_mtx);
			return NULL;
		}
		switch(sig) {
		//se riceve tali segnali, si smette di inserire flie in coda.
		//il programma esaurisce i file in coda e termina come farebbe normalmente
		case SIGHUP:	//segnale di chiusura del terminale
		case SIGINT:
		case SIGQUIT:
		case SIGTERM:
			pthread_mutex_lock(&mw_running_mtx);
			mw_running=0;
			pthread_mutex_unlock(&mw_running_mtx);
			return NULL;
		
		case SIGUSR1:	//incrementa di uno i worker
			pthread_mutex_lock(&thread_num_mtx);
			thread_num_change++;
			pthread_mutex_unlock(&thread_num_mtx);
			break;
		
		case SIGUSR2:	//decrementa di uno i worker (NB: minimo 1 thread nel pool)
			pthread_mutex_lock(&thread_num_mtx);
			thread_num_change--;
			pthread_mutex_unlock(&thread_num_mtx);
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

//deallocazione memoria lista
void list_free(node_t *head) {
	node_t *aux=head;
	while(aux!=NULL) {
		head=head->next;
		free(aux);
		aux=head;
	}
}

void masterworker(int argc, char *argv[], char *socket) {
	fprintf(stderr,"---MasterWorker parte---\n");
	
	/*impostazione valori globali*/
	mw_running=1;
	thread_num_change=0;
	
	/*gestione segnali*/
	sigset_t mask;
	ec_is(sigemptyset(&mask),-1,"masterworker, sigemptyset");
	ec_is(sigaddset(&mask,SIGHUP),-1,"masterworker, sigaddset SIGHUP");
	ec_is(sigaddset(&mask,SIGINT),-1,"masterworker, sigaddset SIGINT");
	ec_is(sigaddset(&mask,SIGQUIT),-1,"masterworker, sigaddset SIGQUIT");
	ec_is(sigaddset(&mask,SIGTERM),-1,"masterworker, sigaddset SIGTERM");
	ec_is(sigaddset(&mask,SIGUSR1),-1,"masterworker, sigaddset SIGUSR1");
	ec_is(sigaddset(&mask,SIGUSR2),-1,"masterworker, sigaddset SIGUSR2");
	//impostazione della nuova mask
	ec_isnot(pthread_sigmask(SIG_SETMASK,&mask,NULL),0,"masterworker, pthread_sigmask FATAL ERROR");
	
	//sigaction per ignorare SIGPIPE
	struct sigaction master_sa;
	memset(&master_sa,0,sizeof(master_sa));
	master_sa.sa_handler=SIG_IGN;
	ec_is(sigaction(SIGPIPE,&master_sa,NULL),-1,"masterworker, sigaction");
	
	//thread detached per la gestione sincrona dei segnali
	pthread_t sighandler_thread;
	ec_isnot(pthread_create(&sighandler_thread,NULL,&sighandler,&mask),0,"masterworker, pthread_create sighandler");
	ec_isnot(pthread_detach(sighandler_thread),0,"masterworker, pthread_detach");
	fprintf(stderr,"Masterworker: segnali settati\n");
	
	/*
	//test sigmask
	if(pthread_kill(sighandler_thread,SIGTERM))
		perror("master kill");
	*/
	
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
	fprintf(stderr,"Masterworker: getopt completata\n");
	fprintf(stderr,"dim. iniziale threadpool: %ld\nlungh. coda: %ld\nritardo inserimento: %ld secondi\n", workers,queue_length,queue_delay);

	/*creazione threadpool*/
	threadpool_t *pool=initialize_pool(workers,queue_length,socket);
	ec_is(pool,NULL,"masterworker, initialize_pool");
	fprintf(stderr,"Masterworker: threadpool inizializzato\n");
	
	/*impostazione del ritardo di inserimento*/
	struct timespec delay, delay_rem;
	delay.tv_sec=queue_delay/1000;
	delay.tv_nsec=(queue_delay%1000)*1000000;
	int sleep_result=0;
	fprintf(stderr,"Masterworker: ritardo impostato\n");

	/*ricerca dei file da linea di comando*/
	int i;
	for(i=optind;i<argc;i++) {
		fprintf(stderr,"Filefinder: argv[%d] inizia\n",i);
		pthread_mutex_lock(&mw_running_mtx);
		if(!mw_running) {	//se bisogna chiudere anticipatamente il programma
			fprintf(stderr,"Master: ho preso segnale di chiusura\n");
			pthread_mutex_unlock(&mw_running_mtx);
			break;
		}
		pthread_mutex_unlock(&mw_running_mtx);
		
		//se bisogna cambiare il numero di worker
		pthread_mutex_lock(&thread_num_mtx);
		while(thread_num_change>0) {
			fprintf(stderr,"Master: ricevuto SIGUSR1\n");
			add_worker(pool);
			thread_num_change--;
		}
		while(thread_num_change<0) {
			fprintf(stderr,"Master: ricevuto SIGUSR2\n");
			remove_worker(pool);
			thread_num_change++;
		}
		pthread_mutex_unlock(&thread_num_mtx);
		
		struct stat info;
		int check_stat=stat(argv[i],&info);
		if(check_stat==-1) {
			perror("Masterworker, stat di file regolare");
			continue;
		}
		if(!S_ISREG(info.st_mode))
			fprintf(stderr,"Passato file %s non regolare da linea di comando, scartato.\n",argv[i]);
		else {	//inserimento con eventuale ritardo
			sleep_result=nanosleep(&delay,&delay_rem);
			while(sleep_result!=0) {
				if(errno!=EINTR)
					continue;
				errno=0;
				sleep_result=nanosleep(&delay_rem,&delay_rem);
			}
			
			int res=enqueue_task(pool,argv[i]);
			if(res)	//invio file
				fprintf(stderr,"Filefinder: argv[%d], enqueue fallita con errore %d",i,res);
			fprintf(stderr,"Filefinder: inserito file %s in coda\n",argv[i]);
		}
	}
	
	/*ricerca dei file dalle directory passate con -d*/
	char fullpathname[MAX_PATHNAME_LEN];	//stringa per memorizzare il nome completo dei file
	while(directories!=NULL) {
		//se bisogna chiudere anticipatamente il programma
		pthread_mutex_lock(&mw_running_mtx);
		if(!mw_running) {
			pthread_mutex_unlock(&mw_running_mtx);
			list_free(directories);
			break;	
		}
		pthread_mutex_unlock(&mw_running_mtx);
		//se bisogna cambiare il numero di worker
		pthread_mutex_lock(&thread_num_mtx);
		while(thread_num_change>0) {	
			fprintf(stderr,"Master: ricevuto SIGUSR1\n");
			add_worker(pool);
			thread_num_change--;
		}
		while(thread_num_change<0) {
			fprintf(stderr,"Master: ricevuto SIGUSR2\n");
			remove_worker(pool);
			thread_num_change++;
		}
		pthread_mutex_unlock(&thread_num_mtx);
		
		//apertura e ricerca nella directory corrente
		DIR *dir=opendir(directories->name);
		if(dir==NULL) {
			fprintf(stderr,"Directory %s non trovata\n",directories->name);
			continue;
		}
		struct dirent *file;
		while((errno=0, file=readdir(dir))!=NULL) {
			//controllo errno
			if(errno!=0) {
				fprintf(stderr,"Errore nell'apertura di %s\n",directories->name);
				ec_isnot(pthread_mutex_lock(&mw_running_mtx),0,"masterworker, mutex lock in dirsearch");
				mw_running=0;
				ec_isnot(pthread_mutex_unlock(&mw_running_mtx),0,"masterworker, mutex unlock in dirsearch");
				break;
			}
			//controllo se sono arrivati segnali di arresto
			pthread_mutex_lock(&mw_running_mtx);
			if(!mw_running) {
				//libera lo spazio delle directory successive, quella corrente viene liberata dopo il while
				list_free(directories->next);
				pthread_mutex_unlock(&mw_running_mtx);
				break;
			}
			pthread_mutex_unlock(&mw_running_mtx);
			//se bisogna cambiare il numero di worker
			pthread_mutex_lock(&thread_num_mtx);
			while(thread_num_change>0) {	
				fprintf(stderr,"Master: ricevuto SIGUSR1\n");
				add_worker(pool);
				thread_num_change--;
			}
			while(thread_num_change<0) {
				fprintf(stderr,"Master: ricevuto SIGUSR2\n");
				remove_worker(pool);
				thread_num_change++;
			}
			pthread_mutex_unlock(&thread_num_mtx);
			
			if(!strncmp(file->d_name,".",2) || !strncmp(file->d_name,"..",3))	//ignora se stessa e la dir padre
				continue;	//vai al prossimo file
				
			else if(file->d_type==DT_DIR) {	//trovata directory
				fprintf(stderr,"trovata directory %s\n",file->d_name);
				dirs_add(&directories,file->d_name,directories->name);	//aggiunta in coda
			}
			
			else if(file->d_type==DT_REG) {	//trovato file normale
				//inserimento con eventuale ritardo
				sleep_result=nanosleep(&delay,&delay_rem);
				while(sleep_result!=0) {
					if(errno!=EINTR)
						continue;
					errno=0;
					sleep_result=nanosleep(&delay_rem,&delay_rem);
				}
				//inserisci il nome completo nella stringa di supporto prima di produrre il file in coda
				memset(fullpathname,0,MAX_PATHNAME_LEN);
				snprintf(fullpathname,MAX_PATHNAME_LEN,"%s/%s",directories->name,file->d_name);
				enqueue_task(pool,fullpathname);	//invio file
				fprintf(stderr,"Filefinder: inserisco file %s\n",fullpathname);
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
	fprintf(stderr,"masterworker: filefinder termina\n");
	
	/*master non inserisce piu' file in coda e attende che il threadpool termini*/
	long finalworkers=await_pool_completion(pool);
	if(finalworkers<1)
		fprintf(stderr,"masterworker, await_pool_completion resituisce numero non valido di thread\n");
	else {
		FILE *workers_file=fopen("nworkeratexit.txt","w");
		if(workers_file==NULL)
			perror("masterworker, fopen nworkeratexit");
		else {
			fprintf(workers_file,"%ld\n",finalworkers);	//inserisci il numero di thread alla fine
			fprintf(stderr,"Masterworker: ricevuti %ld worker\n",finalworkers);
			fclose(workers_file);
		}
	}
	//fprintf(stderr,"Masterworker: threadpool ha concluso il suo lavoro\n");
	
	/*chiusura forzata del signal handler thread*/
	if(mw_running) 
		ec_isnot(pthread_kill(sighandler_thread,SIGQUIT),0,"masterworker, pthread_kill di signal handler");
	fprintf(stderr,"Masterworker: chiusura\n");
}
