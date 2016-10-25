
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <errno.h>
#include <sys/wait.h>

#include "../src/sender/sender_help.h"
#include "../src/shared/packet_interface.h"
#include "../src/receiver/gbnHelper.h"

char *port = "12345";
char *host = "::1";

/* GENERAL TEST */
int test_helper(const char *file_in,const char *file_out,const char *test_name){
		int saved_stdin = dup(fileno(stdin));
		int saved_stdout = dup(fileno(stdout));
		int fd_in = open(file_in,O_RDONLY);
		if(fd_in == -1){
				fprintf(stderr,"Error open %s : %s\n",file_in,strerror(errno));
				return -1;;
		}
		int fd_out = open(file_out,O_WRONLY | O_CREAT, 0666);
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
		/* Lancement des process */
		int pid = fork();
		if(pid == 0){
				char *arg[2]={host,port};
				err = execve("../src/sender",arg,NULL);
				if(err == -1){
						fprintf(stderr,"Error w/ execve() for lauching sender in test %s : %s\n",test_name,strerror(errno));
						return -1;
				}
				return -1;
		}
		else{
				int pid2 = fork();
				if(pid2 == 0){
						char *arg2[2]={host,port};
						err = execve("../src/receiver",arg2,NULL);
						if(err == -1){
								fprintf(stderr,"Error w/ execve() for lauching receiver in test %s : %s\n",test_name,strerror(errno));
								return -1;
						}
						return -1;
				}
				else{
						int status1,status2;
						if(waitpid(pid,&status1,0) == -1){
								fprintf(stderr,"Error w/ waitpid() for the sender in test %s\n",test_name);
								return -1;
						}
						if(waitpid(pid2,&status2,0) == -1){
								fprintf(stderr,"Error w/ waitpid() for the receiver in test %s\n",test_name);
								return -1;
						}
						if(!WIFEXITED(status1) || !WIFEXITED(status2)){
								fprintf(stderr,"The receiver or the sender did not end with exit() or return statement in test %s\n",test_name);
								return -1;
						}
						if(WEXITSTATUS(status1) == -1){
								fprintf(stderr, "The sender exit with an error in test %s\n",test_name);
								return -1;
						}
						if(WEXITSTATUS(status2) == -1){
								fprintf(stderr,"The receiver exit with and error in test %s\n",test_name);
								return -1;
						}
						/* COMPARAISON DES FICHIERS */
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
							fprintf(stderr,"Error fopen %s : %s\n",file_out,strerror(errno));
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
		}
}

void test_512_random(void){
		int err = test_helper("random_512.in","random_512.out","test_512_random");
		if(err == -1){
				CU_FAIL("Error while testing. See stderr\n");
		}
		else if(err == 0){
				CU_ASSERT(1);
		}
		else{
				CU_ASSERT(0);
		}
}

void test_300_random(void){
		int err = test_helper("random_300.in","random_300.out","test_300_random");
		if(err == -1){
				CU_FAIL("Error while testing. See stderr\n");
		}
		else if(err == 0){
				CU_ASSERT(1);
		}
		else{
				CU_ASSERT(0);
		}
}

void test_1000_random(void){
		int err = test_helper("random_1000.in","random_1000.out","test_1000_random");
		if(err == -1){
				CU_FAIL("Error while testing. See stderr\n");
		}
		else if(err == 0){
				CU_ASSERT(1);
		}
		else{
				CU_ASSERT(0);
		}
}

void test_512_zero(void){
		int err = test_helper("zero_512.in","zero_512.out","test_512_zero");
		if(err == -1){
				CU_FAIL("Error while testing. See stderr\n");
		}
		else if(err == 0){
				CU_ASSERT(1);
		}
		else{
				CU_ASSERT(0);
		}
}

void test_300_zero(void){
		int err = test_helper("zero_300.in","zero_300.out","test_300_zero");
		if(err == -1){
				CU_FAIL("Error while testing. See stderr\n");
		}
		else if(err == 0){
				CU_ASSERT(1);
		}
		else{
				CU_ASSERT(0);
		}
}

void test_1000_zero(void){
		int err = test_helper("zero_1000.in","zero_1000.out","test_1000_zero");
		if(err == -1){
				CU_FAIL("Error while testing. See stderr\n");
		}
		else if(err == 0){
				CU_ASSERT(1);
		}
		else{
				CU_ASSERT(0);
		}
}

void test_512_ones(void){
		int err = test_helper("ones_512.in","ones_512.out","test_512_ones");
		if(err == -1){
				CU_FAIL("Error while testing. See stderr\n");
		}
		else if(err == 0){
				CU_ASSERT(1);
		}
		else{
				CU_ASSERT(0);
		}
}

void test_300_ones(void){
		int err = test_helper("ones_300.in","ones_300.out","test_300_ones");
		if(err == -1){
				CU_FAIL("Error while testing. See stderr\n");
		}
		else if(err == 0){
				CU_ASSERT(1);
		}
		else{
				CU_ASSERT(0);
		}
}

void test_1000_ones(void){
		int err = test_helper("ones_1000.in","ones_1000.out","test_1000_ones");
		if(err == -1){
				CU_FAIL("Error while testing. See stderr\n");
		}
		else if(err == 0){
				CU_ASSERT(1);
		}
		else{
				CU_ASSERT(0);
		}
}

void test_no_bytes(void){
		int err = test_helper("no_bytes.in","no_bytes.out","test_no_bytes");
		if(err == -1){
				CU_FAIL("Error while testing. See stderr\n");
		}
		else if(err == 0){
				CU_ASSERT(1);
		}
		else{
				CU_ASSERT(0);
		}
}

/* TEST OF THE SENDER AUXILIARIES FUNCTIONS */
void test_prepare_packet_null(void){
		pkt_t *pkt = prepare_packet(NULL,0);
		if(pkt == NULL){
				CU_FAIL();
		}
		else{
				CU_ASSERT_EQUAL(pkt_get_window(pkt),1);
				CU_ASSERT_PTR_EQUAL(pkt_get_payload(pkt),NULL);
				CU_ASSERT_EQUAL(pkt_get_length(pkt),0);
				pkt_del(pkt);
		}
}

void test_prepare_packet_in_length(void){
		uint8_t *data = (uint8_t *)malloc(250*sizeof(uint8_t));
		if(data == NULL){
				fprintf(stderr,"Not enough memory for test_prepare_in_length\n");
				CU_FAIL();
		}
		pkt_t *pkt = prepare_packet(data,250*sizeof(uint8_t));
		if(pkt == NULL){
				CU_FAIL();
		}
		else{
				CU_ASSERT_PTR_NOT_NULL(pkt_get_payload(pkt));
				CU_ASSERT_EQUAL(pkt_get_length(pkt),250);
				pkt_del(pkt);
				free(data);
		}
}

void test_prepare_packet_just_length(void){
		uint8_t *data = (uint8_t *)malloc(512*sizeof(uint8_t));
		if(data == NULL){
				fprintf(stderr,"Not enough memory for test_prepare_in_length\n");
				CU_FAIL();
		}
		pkt_t *pkt = prepare_packet(data,512*sizeof(uint8_t));
		if(pkt == NULL){
				CU_FAIL();
		}
		else{
				CU_ASSERT_PTR_NOT_NULL(pkt_get_payload(pkt));
				CU_ASSERT_EQUAL(pkt_get_length(pkt),512);
				pkt_del(pkt);
				free(data);
		}
}
				
void test_prepare_packet_out_length(void){
		uint8_t *data = (uint8_t *)malloc(600*sizeof(uint8_t));
		if(data == NULL){
				fprintf(stderr,"Not enough memory for test_prepare_packet_out_length\n");
				CU_FAIL();
		}
		pkt_t *pkt = prepare_packet(data,600*sizeof(uint8_t));
		CU_ASSERT_PTR_NULL(pkt);
		pkt_del(pkt);
		free(data);
}

int main(void){
		if(CUE_SUCCESS != CU_initialize_registry())
				return CU_get_error();
		CU_pSuite pSuite = NULL;
		pSuite = CU_add_suite("Suite de test",NULL,NULL);
		if(NULL == pSuite){
				CU_cleanup_registry();
				return CU_get_error();
		}
		/** ADD TESTS HERE **/
		if(CU_add_test(pSuite,"Test prepare packet w/ payload NULL",test_prepare_packet_null) == NULL ||
						CU_add_test(pSuite,"Test prepare packet w/ payload 250",test_prepare_packet_in_length) == NULL||
						CU_add_test(pSuite,"Test prepare packet w/ payload 512",test_prepare_packet_just_length)== NULL ||
						CU_add_test(pSuite,"Test prepare packet w/ payload 600",test_prepare_packet_out_length) == NULL ||
						CU_add_test(pSuite,"Test random file 300 bytes",test_300_random) == NULL ||
						CU_add_test(pSuite,"Test random file 512 bytes",test_512_random) == NULL ||
						CU_add_test(pSuite,"Test random file 1000 bytes",test_1000_random) == NULL ||
						CU_add_test(pSuite,"Test zero file 300 bytes",test_300_zero) == NULL ||
						CU_add_test(pSuite,"Test zero file 512 bytes",test_512_zero) == NULL ||
						CU_add_test(pSuite,"Test zero file 1000 bytes",test_1000_zero) == NULL ||
						CU_add_test(pSuite,"Test ones file 300 bytes",test_300_ones) == NULL ||
						CU_add_test(pSuite,"Test ones file 512 bytes",test_512_ones) == NULL ||
						CU_add_test(pSuite,"Test ones file 1000 bytes",test_1000_ones) == NULL ||
						CU_add_test(pSuite,"Test no bytes file",test_no_bytes) == NULL){
				CU_cleanup_registry();
				return CU_get_error();
		}
		CU_basic_run_tests();
		CU_basic_show_failures(CU_get_failure_list());
}

