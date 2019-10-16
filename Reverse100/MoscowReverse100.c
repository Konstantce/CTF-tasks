#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <inttypes.h>
#include <string.h>

/*
flag="linear_equations"
Special: A system of linear equations over Z(0x100); multithreading
*/

#define VEC_LEN 16
uint8_t* matrix;
uint8_t* vector;
uint16_t* result;
uint16_t hardcoded[]={108,210,330,404,485,684,665,808,1017,1170,1067,1392,1365,1554,1650,1840};

void* thread_func(void* arg)
{
	uint8_t index=*(uint8_t*)arg;
	uint32_t current=0;
	for(int i=0;i < VEC_LEN;i++)
	{
		current+=matrix[VEC_LEN*index+i]*vector[i];
	}
	result[index]=current;
	pthread_exit(0);
}

int main(int argc,char* argv)
{
	printf("Hello! Give me the flag please!\n");
	uint8_t buffer[VEC_LEN+2];
	uint16_t* current=NULL;
	fread(buffer,1,VEC_LEN+1,stdin);
	char* newline=strchr(buffer,0xa);
	if (newline)
		*newline=0x0;
	if ( strlen(buffer) != VEC_LEN)
		exit(0);
	matrix=(uint8_t*)malloc(VEC_LEN * VEC_LEN);
	vector=(uint8_t*)malloc(VEC_LEN);
	result=(uint16_t*)malloc(VEC_LEN * sizeof(uint16_t));
	memcpy(vector,buffer,VEC_LEN);
	memset(matrix,0,VEC_LEN*VEC_LEN);
	for(uint8_t i=0; i< VEC_LEN; i++)
		matrix[i*(VEC_LEN+1)]=i+1;
	pthread_t threads[VEC_LEN];
	for (uint16_t i=0; i< VEC_LEN; i++)
	{
		result[i]=i;
		pthread_create(&threads[i],NULL,thread_func,(void*)&result[i]);
	}
	for (uint8_t i=0;i<VEC_LEN;i++)
	{
		pthread_join(threads[i],NULL);
	}
	if (!memcmp(result,hardcoded,VEC_LEN*sizeof(uint16_t)))
		printf("Success!\n");
}