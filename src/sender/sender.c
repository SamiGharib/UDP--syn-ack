#include "sender_help.h"

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
		char *hostname;
		int port;
		if(optind == 3){
			hostname = argv[optind];
			port = atoi(argv[optind+1]);
		}
		else{
			hostname = argv[1];
			port = atoi(argv[2]);
		}
		return send_data(hostname,port);
}
