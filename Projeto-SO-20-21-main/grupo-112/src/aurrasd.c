#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/wait.h>
#define MAXFILTERS 10

int ultimoProcesso = 1;

typedef struct filter{
	char* name;
	char* executable;
	int max;
	int inUse;
}Filter;

typedef struct pedido{
	char* ficheiroOrigem;
	char* ficheiroDestino;
	char* filtros[50];
}Pedido;

Filter* filtros;

char** executing;

void filhoAcaba(int signum){
	int status;
	if(signum == SIGUSR1){
		wait(&status);
		executing[WEXITSTATUS(status)] = NULL;	
	}
}

int executeTransform(char* args,int process){
	int numeroComandos = 0;
	Pedido p;
	char** comandos;

	comandos = malloc(sizeof(char*)*numeroComandos);
	
	//inicializa tudo a NULL
	for(int i = 0; i < 50; i++){
		p.filtros[i] = NULL;
	}

	p.ficheiroOrigem = strdup(strsep(&args," "));
	p.ficheiroDestino = strdup(strsep(&args," "));
	

	//Conta os comandos ao mesmo tempo adiciona o nome do filtro a uma lista
	while(args != NULL){
		p.filtros[numeroComandos] = strdup(strsep(&args," "));
		numeroComandos++;
	}

	//Adiciona os comandos a uma lista de strings
	//A alteração da variável filtros[j].unUse++ tem que ser feita no processo pai
	for(int i = 0; p.filtros[i] != NULL; i++){
		for(int j = 0; j < MAXFILTERS; j++){
			if(strcmp(p.filtros[i],filtros[j].name) == 0){
				if(filtros[j].inUse < filtros[j].max){
					comandos[i] = strdup(filtros[j].executable);
					filtros[j].inUse++;
					
							
				}else{
					int sc_fd = open("../tmp/fifo_server_client", O_RDWR);
					write(sc_fd,"Filtro indisponível\n",21);
					close(sc_fd);
					return process;
				}
				break;
			}
		}
	}


	int input = open(p.ficheiroOrigem, O_RDONLY);
	int output = open(p.ficheiroDestino, O_CREAT | O_TRUNC | O_WRONLY,0666);

	int pipes[numeroComandos-1][2];
	int status[numeroComandos];

	if(numeroComandos == 1){

		dup2(input,0);
		dup2(output,1);
		switch(fork()){
			case -1:
				perror("fork");
				return -1;

			case 0:
				execlp(comandos[0],comandos[0],NULL);
				_exit(0);

			default:
				wait(&status[0]);

		}
		

	}else{

		for(int c = 0; c<numeroComandos;c++){

			if(c==0){

				if(pipe(pipes[c]) != 0){
					perror("pipe");
					return -1;
				}

				switch (fork()){
					case -1:
						perror("fork");
						return -1;

					case 0:
					//falta ligar o ficheiro mp4 ao input
						close(pipes[c][0]);
						dup2(input,0);
						dup2(pipes[c][1],1);
						close(pipes[c][1]);
						execlp(comandos[c],comandos[c],NULL);
						_exit(0);

					default:
						close(pipes[c][1]);
				}
			}else if(c == numeroComandos-1){

				switch(fork()){
					case -1:
						perror("fork");
						return -1;

					case 0:
						dup2(pipes[c-1][0],0);
						dup2(output,1);
						close(pipes[c-1][0]);
						execlp(comandos[c],comandos[c],NULL);
						_exit(0);

					default:
						close(pipes[c-1][0]);
				}

			}else{

				if(pipe(pipes[c]) != 0){
				perror("pipe");
				return -1;
				}

				switch(fork()){
					case -1:
						perror("fork");
						return -1;

					case 0:
						close(pipes[c][0]);
						dup2(pipes[c][1],1);
						close(pipes[c][1]);
						dup2(pipes[c-1][0],0);
						close(pipes[c-1][0]);
						execlp(comandos[c],comandos[c],NULL);
						_exit(0);

					default:
						close(pipes[c][1]);
						close(pipes[c-1][0]);
				}
			}

			for(int k = 0; k<numeroComandos;k++){
				wait(&status[k]);
			}

		}
	}
	int sc_fd = open("../tmp/fifo_server_client", O_RDWR);
	write(sc_fd,"Concluído!\n",12);	
	close(sc_fd);

	return process;
}

int executeStatus(){
	char buf[1024];

	int sc_fd = open("../tmp/fifo_server_client", O_RDWR);

	for(int i = 0; i < 800; i++){
		if(executing[i] != NULL){
			//printf("task %d: %s\n",i+1,executing[i]);	
			char aux[1024];
			sprintf(aux,"task %d: %s\n",i+1,executing[i])	;
			write(sc_fd,aux,strlen(aux));
		}
	}
	int i = 0;
	while(filtros[i].name != NULL){
		//printf("filter %s: %d/%d (running/max)\n",filtros[i].name,filtros[i].inUse,filtros[i].max);
		sprintf(buf,"filter %s: %d/%d (running/max)\n",filtros[i].name,filtros[i].inUse,filtros[i].max);
		write(sc_fd,buf,strlen(buf));
		i++;
	}	
	write(sc_fd,"fim",4);
	close(sc_fd);


	return 0;
}

void execute(char* line){
	pid_t pid;
	char* dup = strdup(line);
	char* op = strsep(&line," ");

	signal(SIGUSR1,filhoAcaba);
	


	//cria worker
	if((pid = fork()) == 0){
       if(strcmp(op, "status")==0) {
       		executeStatus();   		
       }else{
			if(strcmp(op,"transform")==0){ 	
				int process = executeTransform(line,ultimoProcesso-1);
				int sc_fd = open("../tmp/fifo_server_client", O_RDWR);
				write(sc_fd,"fim",4);	
				close(sc_fd);
				kill(getppid(),SIGUSR1);
				_exit(process);		
       		}else{
       			perror("opcao invalida");
       		}
       	}
       	_exit(0);
    }else{
    	if(strcmp(op,"transform") == 0){
	    	executing[ultimoProcesso-1] = malloc(sizeof(char)*1024);
			strcpy(executing[ultimoProcesso-1],dup);
			ultimoProcesso++;
		}
    }
  
}


int main(int argc, char const *argv[]) {

	int bytesRead = 0;

	int k = 0;
	char* auxConfig = malloc(sizeof(char)*1024);

	mkfifo("../tmp/fifo_client_server",0644);
	mkfifo("../tmp/fifo_server_client",0644);
    int config = open(argv[1], O_RDONLY);

    filtros = malloc(sizeof(Filter)*MAXFILTERS);
    executing = malloc(sizeof(char*)*800);

    read(config,auxConfig,1024);
    char* field;
    char path[] = "../bin/aurrasd-filters/";

    while(auxConfig != NULL){
    	field = strdup(strsep(&auxConfig,"\n"));
    	filtros[k].name = strdup(strsep(&field," "));
    	filtros[k].executable = strdup(path);
    	char* aux = strdup(strsep(&field," "));
    	strcat(filtros[k].executable,aux);
    	filtros[k].max = atoi(field);
    	filtros[k].inUse = 0;
    	k++;

    }
	
	char* buffer = malloc(sizeof(char)*1024);

	while(1){

		for(int j = 0;j<1024;j++){
			buffer[j] = '\0';
		}

		int cs_fd = open("../tmp/fifo_client_server", O_RDONLY);		
	    bytesRead = read(cs_fd,buffer,1024);

	    if(bytesRead>0) {
	    	execute(buffer);
	    	
	    }	
	}
		
   return 0;
}


