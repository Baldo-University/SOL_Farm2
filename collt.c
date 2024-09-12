#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "message.h"
#include "utils.h"

#define SOCKNAME "littlefarm.sck"
#define SAMPLE1 "cline1"
#define SAMPLE2 "cline2"
#define SAMPLE3 "cline3"

long workfun(char *filename) {
	long result=0;
	FILE *task=fopen(filename,"r");	//apertura del file con bufferized I/O
	if(task==NULL) {
		perror("workfun, fopen");
		return -1;
	}
	long buf;	//buffer che contiene il byte corrente
	long i=0;	//indice i di sommatoria
	while((fread(&buf,sizeof(long),1,task))>0) {
		result+=(i*buf);	// i*file[i]
		i++;
	}
	if(ferror(task) || !feof(task))
		fprintf(stderr,"Workfun: errore durante la lettura del file. Risultato non affidabile\n");
	if(fclose(task)!=0)
		perror("workfun, fclose");
	return result;
}

int main (void) {
	int fd_skt=0;
	struct sockaddr_un sa;
	int already_written=0, just_written=0, to_write=0;
	char buf[BUFFER_SIZE];
	long result=0;
	
	pid_t collector=fork();
	switch(collector) {
	case -1:	//errore fork
		perror("Main, fork");
		exit(EXIT_FAILURE);
	case 0:		//figlio
		execl("./collector","collector",SOCKNAME,(char*)NULL);
		//se si arriva a questa sezione di codice qualcosa e' andato storto
		perror("Main, execl collector");
		exit(EXIT_FAILURE);
	default:	//padre
		strncpy(sa.sun_path,SOCKNAME,UNIX_PATH_MAX);
		sa.sun_family=AF_UNIX;
		ec_is(fd_skt=socket(AF_UNIX,SOCK_STREAM,0),-1,"Client, socket");
		while(connect(fd_skt,(struct sockaddr*)&sa,sizeof(sa))==-1) {
			if(errno==ENOENT) {
				fprintf(stderr,"Client, ENOENT\n");
				sleep(1);
			}
			else {
				perror("client, connect");
				exit(EXIT_FAILURE);
			}
		}
		fprintf(stderr,"Client: connesso al collector\n");
		
		//setup ritardo
		struct timespec delay, delay_rem;
		delay.tv_sec=1;
		delay.tv_nsec=1000000;
		int sleep_result=0;
		
		//invio primo risultato di prova
		result=workfun(SAMPLE1);
		memset(buf,0,BUFFER_SIZE);
		strncpy(buf,SAMPLE1,MAX_PATHNAME_LEN);
		memcpy(buf+MAX_PATHNAME_LEN,&(result),sizeof(long));
		sleep_result=nanosleep(&delay,&delay_rem);
		//ritardo
		while(sleep_result!=0) {
			if(errno!=EINTR)
				fprintf(stderr,"whatever\n");
			errno=0;
			sleep_result=nanosleep(&delay_rem,&delay_rem);
		}			
		already_written=0;
		just_written=0;
		to_write=sizeof(message_t);
		while(to_write>0) {
			just_written=write(fd_skt,buf+already_written,to_write);
			if(just_written<0) {	//errore
				if(errno==EPIPE)	//connessione chiusa, SIGPIPE per fortuna viene ignorato
					fprintf(stderr,"Client: connessione terminata\n");
				else
					perror("worker, errore di write");
				break;
			}
			already_written+=just_written;
			to_write-=just_written;
		}
		if(to_write!=0 && errno!=EPIPE)
			perror("worker, write terminata male");
		fprintf(stderr,"Client: inviato messaggio con risultato\n");
		
		//invio secondo risultato di prova
		result=workfun(SAMPLE2);
		memset(buf,0,BUFFER_SIZE);
		strncpy(buf,SAMPLE2,MAX_PATHNAME_LEN);
		memcpy(buf+MAX_PATHNAME_LEN,&(result),sizeof(long));
		sleep_result=nanosleep(&delay,&delay_rem);
		//ritardo
		while(sleep_result!=0) {
			if(errno!=EINTR)
				fprintf(stderr,"whatever\n");
			errno=0;
			sleep_result=nanosleep(&delay_rem,&delay_rem);
		}			
		already_written=0;
		just_written=0;
		to_write=sizeof(message_t);
		while(to_write>0) {
			just_written=write(fd_skt,buf+already_written,to_write);
			if(just_written<0) {	//errore
				if(errno==EPIPE)	//connessione chiusa, SIGPIPE per fortuna viene ignorato
					fprintf(stderr,"Client: connessione terminata\n");
				else
					perror("worker, errore di write");
				break;
			}
			already_written+=just_written;
			to_write-=just_written;
		}
		if(to_write!=0 && errno!=EPIPE)
			perror("worker, write terminata male");
		fprintf(stderr,"Client: inviato messaggio con risultato\n");
		
		//invio terzo risultato di prova
		result=workfun(SAMPLE3);
		memset(buf,0,BUFFER_SIZE);
		strncpy(buf,SAMPLE3,MAX_PATHNAME_LEN);
		memcpy(buf+MAX_PATHNAME_LEN,&(result),sizeof(long));
		sleep_result=nanosleep(&delay,&delay_rem);
		//ritardo
		while(sleep_result!=0) {
			if(errno!=EINTR)
				fprintf(stderr,"whatever\n");
			errno=0;
			sleep_result=nanosleep(&delay_rem,&delay_rem);
		}			
		already_written=0;
		just_written=0;
		to_write=sizeof(message_t);
		while(to_write>0) {
			just_written=write(fd_skt,buf+already_written,to_write);
			if(just_written<0) {	//errore
				if(errno==EPIPE)	//connessione chiusa, SIGPIPE per fortuna viene ignorato
					fprintf(stderr,"Client: connessione terminata\n");
				else
					perror("worker, errore di write");
				break;
			}
			already_written+=just_written;
			to_write-=just_written;
		}
		if(to_write!=0 && errno!=EPIPE)
			perror("worker, write terminata male");
		fprintf(stderr,"Client: inviato messaggio con risultato\n");
		
		close(fd_skt);	//chiusura socket
		fprintf(stderr,"Client: inviato messaggio di chiusura\n");
	}
	
	int collector_status;
	collector=waitpid(collector,&collector_status,0);
	if(WIFEXITED(collector_status))
		fprintf(stderr,"Main: stato collector: %d\n",WEXITSTATUS(collector_status));
	
	fprintf(stderr,"Fine programma\n");
	return 0;
}
