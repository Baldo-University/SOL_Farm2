#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "message.h"
#include "utils.h"

#define SOCKNAME ".littlefarm.sck"

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
	int fd_skt;
	struct sockaddr_un sa;
	int already_written=0, just_written=0, to_write=0; 
	
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
		
		already_written=0;
		to_write=
		
		
	}
	
	int collector_status;
	collector=waitpid(collector,&collector_status,0);
	if(WIFEXITED(collector_status))
		fprintf(stderr,"Main: stato collector: %d\n",WEXITSTATUS(collector_status));
	
	fprintf(stderr,"Fine programma\n");
	return 0;
}
