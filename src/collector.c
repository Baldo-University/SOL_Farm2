/*
Questa sezione di codice contiene la parte del processo Collector
*/

#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "utils.h"

#define MAX_PATHNAME_LEN 1+MAX_NAMELENGTH
#define UNIX_PATH_MAX 108	//lunghezza massima pathname socket
#define POLL_SIZE 32		//dimensione degli incrementi lineari di poll
#define POLL_TIMEOUT 10000	//timeout di poll() in ms
#define BUFFER_SIZE 512		//dimensione del buffer di ricevimento

//mutex della lista dei risultati
pthread_mutex_t results_mtx=PTHREAD_MUTEX_INITIALIZER;

typedef struct result {
	long total;
	char pathname[MAX_PATHNAME_LEN];
	struct result *next;
} result_t;

//Stampa lista dei risultati
void printlist(result_t *list) {
	result_t *aux=list;
	pthread_mutex_lock(&results_mtx);
	while(aux!=NULL) {
		fprintf(stdout,"%ld\t%s\n",aux->total,aux->pathname);
		aux=aux->next;
	}
	pthread_mutex_unlock(&results_mtx);
}
//Funzione thread che stampa la lista incompleta di risultati ogni secondo
static void *part_print(void *arg) {
	result_t *list=(result_t*)arg;
	struct timespec print, print_rem;
	print.tv_sec=1;
	print.tv_nsec=0;
	int sleep_result=0;
	/* TODO trova modo per chiudere il thread
	for(;;) {
		if(sleep_result==0) {
			sleep_result=nanosleep(&print,&print_rem);
			if(sleep_result!=0 && errno!=EINTR)	//errore diverso da interruzione
				break;
		}
		else {
			sleep_result=nanosleep(&print_rem,&print_rem);
			printlist();
		}
	}
	*/
	return NULL;
}

//Inserimento ordinato nella lista dei risultati
void list_insert(result_t *list, result_t *newnode) {
	pthread_mutex_lock(&results_mtx);
	if(list==NULL) {	//inserimento in lista vuota
		newnode->next=NULL;
		list=newnode;
	}
	else {
		if(list->next==NULL) {	//coda ad un solo elemento
			if(list->total>newnode->total) {	//nuovo elemento strettamente minore
				newnode->next=list;
				list=newnode;
			}
			else {	//nuovo elemento uguale o maggiore
				newnode->next=NULL;
				list->next=newnode;
			}
		}
		else {	//coda con almeno due elementi
			result_t *aux=list;
			while(aux->next!=NULL && aux->total<newnode->total)	//scorri le posizioni
				aux=aux->next;
			newnode->next=aux->next;
			aux->next=newnode;
		}
	}
	pthread_mutex_unlock(&results_mtx);
}

int main(int argc, char *argv[]) {
	/*controllo argomenti*/
	if(argc!=2) {	//al collector deve essere passato solo il nome della socket
		fprintf(stderr,"Collector: passato un numero sbagliato di argomenti\n");
		exit(EXIT_FAILURE);
	}
	
	fprintf(stderr,"---Collector Parte---\n");
	
	/*gestione segnali*/
	sigset_t mask;	//maschera del collector
	ec_is(sigemptyset(&mask),-1,"collector, sigemtpyset");
	ec_isnot(pthread_sigmask(SIG_SETMASK,&mask,NULL),0,"collector, pthread_sigmask");
	//blocca i segnali specificati
	struct sigaction collector_sa;
	memset(&collector_sa,0,sizeof(collector_sa));
	collector_sa.sa_handler=SIG_IGN;	//i segnali vengono ignorati dal collector
	ec_is(sigaction(SIGHUP,&collector_sa,NULL),-1,"collector, sigaddset SIGHUP");
	ec_is(sigaction(SIGINT,&collector_sa,NULL),-1,"collector, sigaddset SIGINT");
	ec_is(sigaction(SIGQUIT,&collector_sa,NULL),-1,"collector, sigaddset SIGQUIT");
	ec_is(sigaction(SIGTERM,&collector_sa,NULL),-1,"collector, sigaddset SIGTERM");
	ec_is(sigaction(SIGUSR1,&collector_sa,NULL),-1,"collector, sigaddset SIGUSR1");
	ec_is(sigaction(SIGUSR2,&collector_sa,NULL),-1,"collector, sigaddset SIGUSR2");
	ec_is(sigaction(SIGPIPE,&collector_sa,NULL),-1,"collector, sigaddset SIGPIPE");
	
	/*Setup variabili per il funzionamento del collector*/
	long clients_num=0;				//numero totale di client connessi
	result_t *results=NULL;	//lista dei risultati
	char buf[BUFFER_SIZE];			//buffer per salvare i dati client
	
	/*Creazione del thread che stampa la lista ogni secondo*/
	pthread_t printer_thread;
	ec_isnot(pthread_create(&printer_thread,NULL,&part_print,(void*)results),0,"collector, pthread_create printer_thread");
	
	/*Setup connessione*/
	int fd_skt, fd_c;		//socket di server e di client
	struct sockaddr_un sa;
	strncpy(sa.sun_path,argv[1],UNIX_PATH_MAX);
	sa.sun_family=AF_UNIX;
	ec_is(fd_skt=socket(AF_UNIX,SOCK_STREAM,0),-1,"collector, socket");
	ec_is(bind(fd_skt,(struct sockaddr*)&sa,sizeof(sa)),-1,"collector, bind");
	ec_is(listen(fd_skt,SOMAXCONN),-1,"collector, listen");
	
	/*Setup poll*/
	struct pollfd *pfds;
	int poll_size=POLL_SIZE;	//dimensione iniziale del poll array
	int nfds=1;					//numero di fd su coi fare poll() al momento del lancio
	int poll_ret=0;				//restituito da poll(), numero di elementi di pfds con un evento o un errore
	ec_is(pfds=calloc(poll_size,sizeof(struct pollfd)),NULL,"collector, calloc pollfd");
	pfds[0].fd=fd_skt;		//poll prende come primo elemento il socket di ascolto di nuove connessioni
	pfds[0].events=POLLIN;	//lettura dati
	fprintf(stderr,"Collector creato\n");
	
	/*loop di poll*/
	for(;;) {
		ec_is(poll_ret=poll(pfds,nfds,POLL_TIMEOUT),-1,"collector, poll");
		if(poll_ret==0) {
			fprintf(stderr,"Raggiunto timeout\n");
			break;
		}
		
		int i;
		for(i=0;i<nfds;i++) {
			if(pfds[i].revents==0)	//nessun evento/errore
				continue;
			if(pfds[i].revents & POLLIN)	//errore
				fprintf(stderr,"aiuto\n");
			if(pfds[i].fd==fd_skt) {	//ricevuta richiesta di nuova connessione client
				fprintf(stderr,"Collector: richiesta nuova connessione\n");
				do {
					fd_c=accept(fd_skt,NULL,0);
					if(fd_c<0) {
						if(errno==EAGAIN || errno==EWOULDBLOCK)	//controllo per portabilita'
							errno=0;
						else
							perror("collector, accept nel loop");
					}
					else {
						fprintf(stderr,"Collector: accettato client con fd %d\n",fd_c);
						int reallocable=1;		//per indicare se abbiamo spazio in memoria per allargare poll
						if(nfds+1==poll_size) {	//controlla che ci sia spazio nel poll_size	
							fprintf(stderr,"Collector: poll piena\n");
							pfds=(struct pollfd*)realloc(pfds,(poll_size+(int)POLL_SIZE)*sizeof(struct pollfd));
							if(errno==ENOMEM) {	//errore di memoria terminata
								perror("collector, realloc in loop");
								reallocable=0;
							}
							else {
								memset(pfds+POLL_SIZE,0,POLL_SIZE);	//necessaria, realloc non la fa automaticamente
								poll_size+=POLL_SIZE;
							}
						}
						if(reallocable) {
							pfds[nfds].fd=fd_c;	//si salva il socket del client in una posizione vuota di poll
							pfds[nfds].events=POLLIN;	//aperto alla lettura di dati
							nfds++;
						}
					}
				} while(fd_c>=0);
			}
			else {	//ricevuti dati da client
				fprintf(stderr,"Collector: ricevuto risultato da client %d\n",pfds[i].fd);
				int already_read=0;				//mantiene la posizione dell'ultimo byte letto
				int just_read;					//byte letti con read()
				int to_read=sizeof(result_t);	//byte restanti da leggere
				while(to_read>0) {	//lettura
					just_read=read(pfds[i].fd,&(buf[already_read]),to_read);
					if(just_read<0) 
						perror("collector, read");
					already_read+=just_read;
					to_read-=just_read;
				}
				if(to_read>0)
					perror("collector, read non completata");
				
				result_t *temp;		//copia lato server del risultato
				ec_is(temp=(result_t*)malloc(sizeof(result_t)),NULL,"collector, allocazione memoria temp");
				//strncpy(temp->total,buf,sizeof(temp->name));
				
				free(temp);
			}
		}
	}
	
	return 0;
}
