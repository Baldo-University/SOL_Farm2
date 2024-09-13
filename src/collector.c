/*
Questa sezione di codice contiene la parte del processo Collector
Collector e' composta da due thread, uno dei quali svolge la maggior parte dei compiti mentre l'altro si limita a
stampare la lista incompleta di risultati ogni secondo.
Il primo thread fa utilizzo di pool() invece che select per comportarsi da server single threaded che gestisce piu'
connessioni client 
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

#define POLL_SIZE 16		//dimensione degli incrementi lineari degli indici di struct pollfd
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
		fprintf(stdout,"%ld %s\n",aux->total,aux->pathname);
		aux=aux->next;
	}
}

//Funzione thread che stampa la lista incompleta di risultati ogni secondo
static void *partial_print(void *arg) {
	DEBUG("Printer: parte\n");
	result_t **results=(result_t**)arg;
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
		
		DEBUG("Printer: attende un secondo...\n");
		sleep_result=nanosleep(&print_time,&print_rem);
		while(sleep_result!=0) {
			if(errno!=EINTR) {	//se errore non dovuto ad interruzione esci per sicurezza
				DEBUG("Printer, nanosleep terminata ne' da interruzione ne' da fine collector\n");
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
		DEBUG("Printer: stampa i risultati\n");
		printlist(*results);
		pthread_mutex_unlock(&mtx);
	}
	DEBUG("Printer: termina\n");
	pthread_exit((void*)NULL);
}

//Inserimento ordinato nella lista dei risultati
void list_insert(result_t **list, result_t *newnode) {
	if(*list==NULL || (*list)->total >= newnode->total) {	//lista vuota o elemento minore stretto del primo
		newnode->next=*list;
		*list=newnode;
		DEBUG("Collector: inserito file in testa alla lista\n");
	}
	else {
		result_t *aux=*list;
		while(aux->next!=NULL && aux->next->total < newnode->total)
			aux=aux->next;
		newnode->next=aux->next;
		aux->next=newnode;
		DEBUG("Collector: inserito file in maniera ordinata nella lista\n");
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
		DEBUG("Collector: passato un numero sbagliato di argomenti\n");
		exit(EXIT_FAILURE);
	}
	DEBUG("Collector: partenza\n");
	
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
	DEBUG("Collector: segnali settati\n");
	
	/*Setup variabili per il funzionamento del collector*/
	running=1;					//server in funzionamento
	int any_closed=0;			//settato a 1 quando viene chiusa una connessione, necessario per chiudere il server
	int close_conn=0;			//per capire se chiudere una connessione client
	int on=1;					//per ioctl
	result_t *results=NULL;		//lista dei risultati
	char buf[BUFFER_SIZE];		//buffer per salvare i dati client
	int i,j;					//indici scorrimento array
	
	/*Creazione del thread che stampa la lista ogni secondo*/
	pthread_t printer_thread;
	ec_isnot(pthread_create(&printer_thread,NULL,&partial_print,(void*)&results),0,"collector, pthread_create printer_thread");
	DEBUG("Collector: thread printer lanciato\n");
	
	/*Setup connessione*/
	int fd_skt=-1, fd_c=-1;		//socket di server e di client
	struct sockaddr_un sa;
	strncpy(sa.sun_path,argv[1],UNIX_PATH_MAX);
	sa.sun_family=AF_UNIX;
	ec_is(fd_skt=socket(AF_UNIX,SOCK_STREAM,0),-1,"collector, socket");
	//set server socket a nonblocking. Le socket per le connessioni client erediteranno lo stato nonblocking da essa
	if(ioctl(fd_skt,FIONBIO,(char*)&on)==-1) {
		perror("collector, ioctl");
		close(fd_skt);
		exit(EXIT_FAILURE);
	}
	if(bind(fd_skt,(struct sockaddr*)&sa,sizeof(sa))==-1) {
		perror("collector, bind");
		close(fd_skt);
		exit(EXIT_FAILURE);
	}
	if(listen(fd_skt,SOMAXCONN)==-1) {
		perror("collector, listen");
		close(fd_skt);
		exit(EXIT_FAILURE);
	}
	DEBUG("Collector: aperta socket di listen\n");
	
	/*Setup poll*/
	struct pollfd *pfds;
	int poll_size=POLL_SIZE;	//dimensione iniziale del poll array
	int nfds=1;					//numero di fd su coi fare poll() al momento del lancio
	int poll_ret=0;				//restituito da poll(), numero di elementi di pfds con un evento o un errore
	int cur_nfds=nfds;			//valore per l'iterazione su poll
	//calloc alloca memoria e inizializza pollfd
	pfds=calloc(poll_size,sizeof(struct pollfd));
	if(pfds==NULL) {
		perror("collector, allocazione memoria iniziale struct pollfd");
		close(fd_skt);
		exit(EXIT_FAILURE);
	}
	pfds[0].fd=fd_skt;		//poll prende come primo elemento il socket di ascolto di nuove connessioni
	pfds[0].events=POLLIN;	//lettura dati
	DEBUG("Collector: inizializzato\n");
	
	/*loop di poll*/
	for(;;) {
		DEBUG("Collector: inizio loop principale\n");
		
		//controllo di running
		pthread_mutex_lock(&mtx);
		if(!running) {
			DEBUG("Collector: il loop cessa\n");
			pthread_mutex_unlock(&mtx);
			break;
		}
		pthread_mutex_unlock(&mtx);
		
		DEBUG("Collector: in attesa di poll\n");
		poll_ret=poll(pfds,nfds,POLL_TIMEOUT);
		if(poll_ret<0) {	//errore poll
			DEBUG_PERROR("collector, poll fallisce");
			running=0;
			break;
		}
		if(poll_ret==0) {	//timeout raggiunto
			DEBUG("Collector: raggiunto timeout poll. Inizio chiusura\n");
			running=0;
			break;
		}
		
		/*iterazione sulle connessioni stabilite*/
		cur_nfds=nfds;
		for(i=0;i<cur_nfds;i++) {
			DEBUG("Collector: in loop cur_nfds\n");
			if(pfds[i].revents==0) {	//nessun evento/errore
				DEBUG("Collector: non ricevuto alcun evento da %d\n",pfds[i].fd);
				continue;	
			}
			if(pfds[i].revents != POLLIN)	//errore
				DEBUG("Collector: ricevuto evento diverso da POLLIN\n");
			
			//ricevuta richiesta di nuova connessione client
			if(pfds[i].fd==fd_skt) {
				DEBUG("Collector: richiesta nuova connessione\n");
				do {
					fd_c=accept(fd_skt,NULL,NULL);
					if(fd_c<0) {	//errore
						if(errno!=EAGAIN || errno!=EWOULDBLOCK) {	//controllo nonblocking
							DEBUG_PERROR("collector, accept");
							running=0;
						}
						break;	//questa socket non ha piu' nulla da scrivere
					}
					else {	//stabilita connessione
						DEBUG("Collector: accettato client con fd %d\n",fd_c);
						int reallocable=1;		//per indicare se abbiamo spazio in memoria per allargare poll
						//controlla che ci sia spazio nel poll_size	
						if(nfds+1==poll_size) {	//spazio esaurito
							DEBUG("Collector: poll piena\n");
							pfds=(struct pollfd*)realloc(pfds,(poll_size+(int)POLL_SIZE)*sizeof(struct pollfd));
							if(errno==ENOMEM) {	//errore di memoria terminata
								DEBUG_PERROR("collector, realloc in loop");
								reallocable=0;
							}
							else {	//rialloca la memoria di pollfd e la 'allarga'
								DEBUG("Collector: aggiunta memoria extra\n");
								memset(pfds+POLL_SIZE,0,POLL_SIZE);	//necessaria, realloc non la fa automaticamente
								poll_size+=POLL_SIZE;
							}
						}
						if(reallocable) {	//spazio di poll disponibile
							DEBUG("Collector: assegna indice %d di poll al client\n",nfds);
							pfds[nfds].fd=fd_c;	//si salva il socket del client in una posizione vuota di poll
							pfds[nfds].events=POLLIN;	//aperto alla lettura di dati
							nfds++;	//aumenta di uno il numero di connessioni e punta all;indice successivo di poll
						}
					}
				} while(fd_c!=-1);
			}
			
			//ricevuti dati da client
			else {
				DEBUG("Collector: client %d invia uno o piu' risultati\n",pfds[i].fd);
				close_conn=0;	//settato a zero, se si verificano problemi si setta ad 1
				do {	//loop lettura dati fino a che read non restituisce EAGAIN/EWOULDBLOCK
					DEBUG("Collector: client %d, lettura messaggio\n",pfds[i].fd);
					int already_read=0;				//mantiene la posizione dell'ultimo byte letto
					int just_read=0;				//byte letti con read()
					int to_read=sizeof(message_t);	//byte restanti da leggere
					while(to_read>0) {	//lettura
						just_read=read(pfds[i].fd,&(buf[already_read]),to_read);
						if(just_read<0){
							if(errno!=EAGAIN || errno!=EWOULDBLOCK) {	//chiusura connessione
								DEBUG_PERROR("collector, read");
								close_conn=1;	//la connessione verra' chiusa per sicurezza
							}
							DEBUG("Collector: client %d, while di lettura, EAGAIN o EWOULDBLOCK\n",pfds[i].fd);
							break;	//esce dal while di lettura messaggio
						}
						if(just_read==0) {	//client ha chiuso la socket
							DEBUG("Collector: client %d chiude la connessione\n",pfds[i].fd);
							close_conn=1;
							break;	//va a chiudere la socket lato client
						}
						already_read+=just_read;
						to_read-=just_read;
					}
					if(to_read!=0) {	//errore, read uscita per errore o read non completata
						if(to_read>0 && !close_conn)	//qualcosa non va nella read...
							DEBUG_PERROR("collector, read non completata");
						break;	//in ogni caso, esce dal do-while
					}
					if(close_conn)
						break;	//uscita dal do-while
					
					//ricevuto risultato da mettere in lista
					DEBUG("Collector: risultato di %d ricevuto\n",pfds[i].fd);
					result_t *new_res;
					ec_is(new_res=(result_t*)malloc(sizeof(result_t)),NULL,"collector, allocazione memoria temp");
					strncpy(new_res->pathname,buf,sizeof(new_res->pathname));
					memcpy(&(new_res->total),buf+sizeof(new_res->pathname),sizeof(new_res->total));
					pthread_mutex_lock(&mtx);
					DEBUG("Collector: client %d inserisce un risultato\n",pfds[i].fd);
					list_insert(&results,new_res);
					pthread_mutex_unlock(&mtx);
					
				} while(1);
				
				if(close_conn) {	//chiusura socket lato server
					close(pfds[i].fd);	//close vera e propria
					pfds[i].fd=-1;		//segnala alla struct pollfd che va pulito un file descriptor
					//da ora in poi se nfds==1 si chiude il server perche' non ci sono piu' worker collegati
					any_closed=1;
				}
			}
		}
		
		/*pulizia poll*/
		DEBUG("Collector: inizio loop pulizia\n");
		for(i=0;i<nfds;i++) {
			if(pfds[i].fd==-1) {	//connessione chiusa
				for(j=i;j<nfds;j++) {	//shift di posizioni dei restanti indici
					pfds[j].fd=pfds[j+1].fd;
					pfds[j].events=pfds[j+1].events;
				}
				i--;
				nfds--;
				DEBUG("Collector: rimosso un client\n");
			}
		}
		
		/*controllo chiusura server*/
		if(any_closed) {	//almeno un client si e' disconnesso
			if(nfds==1) {	//solo fd_skt rimane aperta, tutti i client si sono disconnessi
				pthread_mutex_lock(&mtx);
				running=0;
				DEBUG("Collector: inizio terminazione\n");
				pthread_mutex_unlock(&mtx);
			}
		}
	}
	
	free(pfds);		//dealloca la memoria di poll
	pthread_join(printer_thread,(void*)NULL);	//aspetta che il printer thread termini da solo
	printlist(results);		//stampa la lista finale dei risultati
	list_free(results);		//dealloca la memoria dei risultati
	pthread_mutex_destroy(&mtx);	//dealloca mutex
	close(fd_skt);					//chiude la connessione
	DEBUG("Collector: termina\n");
	//arrivato qui il processo termina con successo
	return 0;
}
