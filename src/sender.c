#include "sender.h"
#include <unistd.h>

int main(int argc, char *argv[]){
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
		int port = atoi(argv[optind+1]);
		send_data(hostname,port);
}
