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
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "message.h"

#define POLL_SIZE 16		//dimensione degli incrementi lineari di poll
#define POLL_TIMEOUT 10000	//timeout di poll() in ms

int running;		//collector in funzionamento o meno
pthread_mutex_t mtx=PTHREAD_MUTEX_INITIALIZER;	//mutex per la lista dei risultati e per running

//lista di risultati
typedef struct result {
	long total;
	char pathname[MAX_PATHNAME_LEN];
	struct result *next;
} result_t;

//Stampa lista dei risultati
void printlist(result_t *list) {
	result_t *aux=list;
	while(aux!=NULL) {
		fprintf(stdout,"%ld\t%s\n",aux->total,aux->pathname);
		aux=aux->next;
	}
}
/*
//Funzione thread che stampa la lista incompleta di risultati ogni secondo
static void *partial_print(void *arg) {
	fprintf(stderr,"Printer: parte\n");
	result_t *results=(result_t*)arg;
	struct timespec print_time, print_rem;
	print_time.tv_sec=1;
	print_time.tv_nsec=0;
	int sleep_result=0;
	for(;;) {
		pthread_mutex_lock(&mtx);
		if(!running) {
			pthread_mutex_unlock(&mtx);
			break;
		}
		pthread_mutex_unlock(&mtx);
		
		fprintf(stderr,"Printer: attende un secondo...\n");
		sleep_result=nanosleep(&print_time,&print_rem);
		while(sleep_result!=0) {
			if(errno!=EINTR) {	//se errore non dovuto ad interruzione esci
				fprintf(stderr,"Printer: terminato ne' da interruzione ne' da fine collector\n");
				pthread_exit((void*)NULL);
			}
			errno=0;
			sleep_result=nanosleep(&print_rem,&print_rem);
		}
		
		pthread_mutex_lock(&mtx);
		if(!running) {
			pthread_mutex_unlock(&mtx);
			break;
		}
		fprintf(stderr,"Printer: stampa i risultati\n");
		printlist(results);
		pthread_mutex_unlock(&mtx);
	}
	fprintf(stderr,"Printer: termina\n");
	pthread_exit((void*)NULL);
}
*/
//Inserimento ordinato nella lista dei risultati
void list_insert(result_t **list, result_t *newnode) {
	if(*list==NULL || (*list)->total >= newnode->total) {	//lista vuota o elemento minore stretto del primo
		newnode->next=*list;
		*list=newnode;
		fprintf(stderr,"Collector: inserito file in testa alla lista\n");
	}
	else {
		result_t *aux=*list;
		while(aux->next!=NULL && aux->next->total < newnode->total)
			aux=aux->next;
		newnode->next=aux->next;
		aux->next=newnode;
	}
}

//deallocazione memoria lista
void list_free(result_t *head) {
	result_t *aux=head;
	while(aux!=NULL) {
		head=head->next;
		free(aux);
		aux=head;
	}
}

int main(int argc, char *argv[]) {
	/*controllo argomenti*/
	if(argc!=2) {	//al collector deve essere passato solo il nome della socket
		fprintf(stderr,"Collector: passato un numero sbagliato di argomenti\n");
		exit(EXIT_FAILURE);
	}
	fprintf(stderr,"---Collector parte---\n");
	
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
	
	fprintf(stderr,"Collector: segnali settati\n");
	
	/*Setup variabili per il funzionamento del collector*/
	running=1;					//server in funzionamento
	int any_closed=0;			//settato a 1 quando viene chiusa una connessione, necessario per chiudere il server
	int on=1;					//per ioctl
	result_t *results=NULL;		//lista dei risultati
	char buf[BUFFER_SIZE];		//buffer per salvare i dati client
	int i,j;					//indici scorrimento array
	
	/*Creazione del thread che stampa la lista ogni secondo*/
	/*
	pthread_t printer_thread;
	ec_isnot(pthread_create(&printer_thread,NULL,&partial_print,(void*)results),0,"collector, pthread_create printer_thread");
	fprintf(stderr,"Collector: thread printer lanciato\n");
	*/
	
	/*Setup connessione*/
	int fd_skt=-1, fd_c=-1;		//socket di server e di client
	struct sockaddr_un sa;
	strncpy(sa.sun_path,argv[1],UNIX_PATH_MAX);
	sa.sun_family=AF_UNIX;
	ec_is(fd_skt=socket(AF_UNIX,SOCK_STREAM,0),-1,"collector, socket");
	ec_is(ioctl(fd_skt,FIONBIO,(char*)&on),-1,"collector, ioctl");				//server nonblocking
	ec_is(bind(fd_skt,(struct sockaddr*)&sa,sizeof(sa)),-1,"collector, bind");
	ec_is(listen(fd_skt,SOMAXCONN),-1,"collector, listen");
	fprintf(stderr,"Collector: aperta socket di listen\n");
	
	/*Setup poll*/
	struct pollfd *pfds;
	int poll_size=POLL_SIZE;	//dimensione iniziale del poll array
	int nfds=1;					//numero di fd su coi fare poll() al momento del lancio
	int poll_ret=0;				//restituito da poll(), numero di elementi di pfds con un evento o un errore
	int cur_nfds=nfds;			//valore per l'iterazione su poll
	ec_is(pfds=calloc(poll_size,sizeof(struct pollfd)),NULL,"collector, calloc pollfd");
	pfds[0].fd=fd_skt;		//poll prende come primo elemento il socket di ascolto di nuove connessioni
	pfds[0].events=POLLIN;	//lettura dati
	fprintf(stderr,"Collector: inizializzato\n");
	
	/*loop di poll*/
	for(;;) {
		fprintf(stderr,"Collector: inizio loop\n");
		
		//controllo di running
		pthread_mutex_lock(&mtx);
		if(!running) {
			fprintf(stderr,"Collector: cessa il loop\n");
			pthread_mutex_unlock(&mtx);
			break;
		}
		pthread_mutex_unlock(&mtx);
		
		fprintf(stderr,"Collector: in attesa di poll\n");
		ec_is(poll_ret=poll(pfds,nfds,POLL_TIMEOUT),-1,"collector, poll");
		if(poll_ret<0) {
			perror("collector, poll");
			break;
		}
		if(poll_ret==0) {
			fprintf(stderr,"Collector: raggiunto timeout poll. Inizio chiusura\n");
			break;
		}
		
		/*iterazione sulle connessioni stabilite*/
		cur_nfds=nfds;
		for(i=0;i<cur_nfds;i++) {
			fprintf(stderr,"Collector: in loop cur_nfds\n");
			if(pfds[i].revents==0)	//nessun evento/errore
				continue;
			if(pfds[i].revents != POLLIN) {	//errore
				fprintf(stderr,"Collector: questo errore non dovrebbe accadere\n");
				running=0;
				break;
			}
			//ricevuta richiesta di nuova connessione client
			if(pfds[i].fd==fd_skt) {
				fprintf(stderr,"Collector: richiesta nuova connessione\n");
				do {
					fprintf(stderr,"Collector: prima di accept\n");
					fd_c=accept(fd_skt,NULL,NULL);
					fprintf(stderr,"Collector: dopo accept\n");
					if(fd_c<0) {
						if(errno==EAGAIN || errno==EWOULDBLOCK)	//controllo per portabilita'
							errno=0;
						else
							perror("collector, accept");
							
					}
					else {
						fprintf(stderr,"Collector: accettato client con fd %d\n",fd_c);
						int reallocable=1;		//per indicare se abbiamo spazio in memoria per allargare poll
						//controlla che ci sia spazio nel poll_size	
						if(nfds+1==poll_size) {	//spazio esaurito
							fprintf(stderr,"Collector: poll piena\n");
							pfds=(struct pollfd*)realloc(pfds,(poll_size+(int)POLL_SIZE)*sizeof(struct pollfd));
							if(errno==ENOMEM) {	//errore di memoria terminata
								perror("collector, realloc in loop");
								reallocable=0;
							}
							else {
								fprintf(stderr,"Collector: aggiunta memoria extra\n");
								memset(pfds+POLL_SIZE,0,POLL_SIZE);	//necessaria, realloc non la fa automaticamente
								poll_size+=POLL_SIZE;
							}
						}
						if(reallocable) {	//spazio di poll disponibile
							fprintf(stderr,"Collector: assegna indice %d di poll al client\n",nfds);
							pfds[nfds].fd=fd_c;	//si salva il socket del client in una posizione vuota di poll
							pfds[nfds].events=POLLIN;	//aperto alla lettura di dati
							nfds++;	//aumenta di uno il numero di connessioni e punta all;indice successivo di poll
						}
					}
				} while(fd_c!=-1);
			}
			
			//ricevuti dati da client
			else {
				fprintf(stderr,"Collector: ricevuto risultato da client %d\n",pfds[i].fd);
				int already_read=0;				//mantiene la posizione dell'ultimo byte letto
				int just_read;					//byte letti con read()
				int to_read=sizeof(message_t);	//byte restanti da leggere
				while(to_read>0) {	//lettura
					just_read=read(pfds[i].fd,&(buf[already_read]),to_read);
					if(just_read<0) 
						perror("collector, read");
					already_read+=just_read;
					to_read-=just_read;
				}
				if(to_read!=0)
					perror("collector, read non completata");
				
				result_t *new_res;		//copia lato server del risultato
				ec_is(new_res=(result_t*)malloc(sizeof(result_t)),NULL,"collector, allocazione memoria temp");
				strncpy(new_res->pathname,buf,sizeof(new_res->pathname));
				memcpy(&(new_res->total),buf+sizeof(new_res->pathname),sizeof(new_res->total));
				
				//controlla se ha ricevuto messaggio di disconnessione
				if(!strncmp(new_res->pathname,DISCONNECT,strlen(DISCONNECT)) && new_res->total<0) {
					fprintf(stderr,"Collector: chiusura del client %d\n",pfds[i].fd);
					free(new_res);	//non si inserisce nella lista dei risultati
					close(pfds[i].fd);
					pfds[i].fd=-1;	//indice poll da ripulire in seguito
					//nfds--;
					any_closed=1;	//da questo punto in poi se abbiamo solo la socket fd_skt il server chiude
				}
				else {	//inserimento in lista
					pthread_mutex_lock(&mtx);
					list_insert(&results,new_res);
					fprintf(stderr,"Collector: client %d inserisce un risultato\n",pfds[i].fd);
					pthread_mutex_unlock(&mtx);
				}
			}
		}
		
		/*pulizia poll*/
		for(i=0;i<nfds;i++) {
			fprintf(stderr,"Collector: loop pulizia\n");
			if(pfds[i].fd==-1) {	//connessione chiusa
				for(j=i;j<nfds;j++) {	//shift di posizioni dei restanti indici
					pfds[j].fd=pfds[j+1].fd;
					pfds[j].events=pfds[j+1].events;
				}
				i--;
				nfds--;
				fprintf(stderr,"Collector: rimosso un client\n");
			}
		}
		
		/*controllo chiusura server*/
		if(any_closed) {	//almeno un client si e' disconnesso
			if(nfds==1) {	//solo fd_skt rimane aperta, tutti i client si sono disconnessi
				pthread_mutex_lock(&mtx);
				running=0;
				fprintf(stderr,"Collector: running settato a zero\n");
				pthread_mutex_unlock(&mtx);
			}
		}
	}
	
	free(pfds);		//dealloca la memoria di poll
	//pthread_join(printer_thread,(void*)NULL);	//aspetta che il printer thread termini da solo
	printlist(results);		//stampa la lista finale dei risultati
	list_free(results);		//dealloca la memoria dei risultati
	pthread_mutex_destroy(&mtx);	//dealloca mutex
	close(fd_skt);					//chiude la connessione
	fprintf(stderr,"Collector: termina\n");
	//arrivato qui il processo termina con successo
	return 0;
}
