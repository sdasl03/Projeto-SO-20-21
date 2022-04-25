#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const *argv[]) {

    int i = 1;
    char buffer[1024];    
    char buffer2[1024];
    char buffer3[1024];    
    int cs_fd = open("../tmp/fifo_client_server", O_RDWR);
    int sc_fd = open("../tmp/fifo_server_client", O_RDWR);

    if(argc == 1){
        write(1,"./aurras status\n",16);
        write(1,"./aurras transform <ficheiro original> <ficheiro destino> <filtros>\n",68);
    }else{  
        if(argc == 2){
            write(cs_fd,argv[1],strlen(argv[1]));
            close(cs_fd);
            int bytesRead = 0;
            while((bytesRead = read(sc_fd,buffer2,strlen(buffer2)))>0 && strcmp(buffer2,"fim")!=0){  
                write(1,buffer2,bytesRead);
            }
        }
        else{
            i=1;
            strcpy(buffer,argv[i]);
            i++;
            int bytesRead = 0;
            while(i < argc){
                strcat(buffer," ");
                strcat(buffer,argv[i]);
                i++;
            }
            write(cs_fd,buffer,strlen(buffer));
            close(cs_fd);
            while((bytesRead = read(sc_fd,buffer3,strlen(buffer3)))>0 && strcmp(buffer3,"fim")!=0){
                write(1,buffer3,bytesRead);
            }

        }
    }

    close(cs_fd);

    

    return 0;
}
