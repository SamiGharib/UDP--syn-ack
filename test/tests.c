
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <errno.h>
#include <sys/wait.h>

#include "../sender.h"
#include "../receiver.h"

int port = 12345;
const char *host = "::1"
const int saved_stdin = dup(fileno(stdin));
const int saved_stdout = dup(fileno(stdout));

int test_helper(const char *file_in,const char *file_out,const char *test_name){
		int fd_in = open(file_in,O_RDONLY);
		if(fd_in == -1){
				fprintf(stderr,"Error open %s : %s\n",file_in,strerror(errno));
				return -1;;
		}
		int fd_out = open(file_out,O_WRONLY | O_CREAT);
		if(fd_out == -1){
				fprintf(stderr,"Error open %s : %s\n",file_out,strerror(errno));
				return -1;;
		}
		int err = dup2(fd_in,fileno(stdin));
		if(err == -1){
				fprintf(stderr,"Error dup2 stdin %s : %s\n",file_in,strerror(errno));
				return -1;;
		}
		err = dup2(fd_out,fileno(stdout));
		if(err == -1){
				fprintf(stderr,"Error dup2 stdout %s : %s\n",file_out,strerror(errno));
				return -1;;
		}
		int pid=fork();
		if(pid == -1){
				fprintf(stderr,"Error fork %s : %s\n",test_name,strerror(errno));
				return -1;;
		}
		if(pid == 0){
				receiver(host,port);
		}
		else{
				int status;
				sender(host,port);
				pid_t w = waitpid(pid,&status,0);
				if(w == -1){
						fprintf(stderr,"Error waitpid %s : %s\n",test_name,strerror(errno));
						return -1;;
				}
				if(WIEXITED(status)){
						dup2(saved_stdin,fileno(stdin));
						dup2(saved_stdout,fileno(stdout));
						close(fd_in);
						close(fd_out);
						FILE *f_in = fopen(file_in,"r");
						if(f_in == NULL){
								fprintf(stderr,"Error fopen %s : %s\n",file_in,strerror(errno));
								return -1;
						}
						FILE *f_out = fopen(file_out,"r");
						if(f_out == NULL){
								fpritnf(stderr,"Error fopen %s : %s\n",file_out,strerror(errno));
								return -1;
						}
						char c1 = getc(f_in);
						char c2 = getc(f_out);
						while((c1 != EOF) && (c2 != EOF) && (c1 == c2)){
								c1 = getc(f_in);
								c2 = getc(f_out);
						}
						fclose(f_in);
						fclose(f_out);
						if(c1 == c2)
								return 1;
						else
								return 0;
				}
				else{
					if(WIFSIGNALED(status))
						fprintf(stderr,"Child process killed by signal in %s\n",test_name);
					return -1;;
				}
		}
}
				
