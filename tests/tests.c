
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>
#include <errno.h>
#include <sys/wait.h>

#include "../sender.h"
#include "../receiver.h"

const char *port = "12345";
const char *host = "::1";
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
		pthread thread[2];
		int err = pthread_create(&th[0],NULL,(void *)&receive_helper,NULL);
		if(err != 0){
				fprintf(stderr,"Error while creating thread for test %s (function receive)\n",test_name);
				return -1;
		}
		err =pthread_create(&th[1],NULL,(void *)&send_helper,NULL);
		if(err != 0){
			fprintf(stderr,"Error while creating thread for test %s (function send)\n",test_name);	
			return -1;
		}
		err = pthread_join(th[0],NULL);
		err = pthread_join(th[1],NULL);
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
		
		CU_basic_run_tests();
		CU_basic_show_failures(CU_get_failure_list());
}

