#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/* Network's include */
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#include "sender.h"
#include "real_address.h"
#include "create_socket.h"

int start_window=0;
int end_windows=0;
const int window[32]={0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
void send(const char *dest_addr,int port){
}

int main(int argc, const char *argv[]){
		int opt = getopt(argc,argv,"f:");
		if(opt == 'f'){
				int fd_in = open(optarg,O_RDONLY);
				if(fd_in == -1){
						fprintf(stderr,"Error while opening the file %s : %s\n",optarg,strerror(errno));
						return -1;
				}
				int err = dup2(fd_in,fileno(stdin));
				if(err == -1){
						fprintf(stderr,"Error while redirecting stdin to the file %s : %s\n",optarg,strerror(errno));
						return -1;
				}
		}
		char *hostname = argv[optind];
		int port = argv[optind+1];
		send(hostname,port);
}
