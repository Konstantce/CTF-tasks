#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <string.h>
#include <gmp.h>
#include <sys/prctl.h>

/* 
flag="NotBadFor300Points!" 
Special: Rabin-Karp cyclic hash, some big math, some anti-debugging
*/
char* hardcoded="1000000011011111011100010000100011111101110000111001000001001001100011010111111001\
01110010001010110000010011100001001001110001000000110110111101111001101110";



void main(int argc,char* argv[])
{
	if (argc !=2)
	{
		printf("You should give me the flag!\n");
		exit(0);
	}
	int status;
	prctl(PR_SET_PTRACER,PR_SET_PTRACER_ANY,NULL,NULL,NULL);
	pid_t child=fork();
	if (!child)
	{
		pid_t parent=getppid();
		int result=ptrace(PTRACE_ATTACH,parent,0,0);
		sleep(1);
		ptrace(PTRACE_DETACH,parent,0,0);
		exit(result || (int)getenv("LD_PRELOAD"));
	}
	else
	{
		wait(&status);
		if (status)
			exit(0);
	}
	char* flag=argv[1];
	if (strlen(flag) == 19)
	{
		mpz_t result;
		mpz_init(result);
		for(int i=0;i<strlen(flag);i++)
		{
			mpz_mul_ui(result,result,307);
			mpz_add_ui(result,result,flag[i]);
		}
		char* converted_result=mpz_get_str(NULL,2,result);
		mpz_clear(result);
		if (!strcmp(converted_result,hardcoded))
			printf("Success! You've found the right flag!\n");
	}
}