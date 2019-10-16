#include "stdafx.h"
#pragma auto_inline(off)
#include <string.h>
#pragma comment(lib,"Miracl.lib")
extern "C"
{
	#include "miracl.h"
}
#define BLOCK 32

// p
char* module_value="115792089210356248762697446949407573530086143415290314195533631308867097853951";
// a=p-3; b
char* b_value="41058363725152142129326129780047268409114441015993725554835256314039467401291";
epoint *G,*F;
//F=backdoor * G; G-base point
char *backdoor_value="238541930543";
char *x_value="48439561293906451759052585252797914202762949526041747995844080717082404635286";


long get_file_size(FILE* fd)
{
	
	fseek(fd,0,SEEK_END);
	long result=ftell(fd);
	fseek(fd,0,SEEK_SET);
	return result;
}

void cipher(FILE* fd,FILE* wfd,char* gamma,int size)
{
	for(int i=0;i<size;i++)
	{
		unsigned char c=fgetc(fd);
		fputc(c^gamma[i],wfd);
	}
}


//mistake is here!
void stage(big* IV,big* coordinate,epoint* TEMP,char* output_buffer)
{
	ecurve_mult(*IV,G,TEMP);
	epoint_get(TEMP,*IV,*IV);
	ecurve_mult(*IV,F,TEMP);
	epoint_get(TEMP,*coordinate,*coordinate);
	big_to_bytes(32,*coordinate,output_buffer,TRUE);
}

int main(int argc, char* argv[])
{
	//parsing command line arguments 
	if (argc !=4)
	{
		printf("Usage: cryptor.exe  key   file_to_encrypt   output_file\n");
		return 0;
	}
	char* key=argv[1];
	char* filename=argv[2];
	FILE *fd,*wfd;
	if ((fd=fopen(filename,"rb"))==NULL)
	{
		printf("No file found\n");
		return 0;
	}
	wfd=fopen(argv[3],"wb");
	long filesize=get_file_size(fd);
	miracl *mip=mirsys(64,16);
	mip->IOBASE=10;
	//first of all, let's initialize IV (it's derived from key)
	char output_buffer[32];
	sha256 sh;
	shs256_init(&sh);
	for(size_t i=0;i<strlen(key);i++)
	{
		shs256_process(&sh,key[i]);
	}
	shs256_hash(&sh,output_buffer);
	big IV=mirvar(0);
	bytes_to_big(32,output_buffer,IV);
	
	//Now let's init elliptic curve
	
	big backdoor=mirvar(0);
	big prime_module=mirvar(0);
	big a=mirvar(0);
	big b=mirvar(0);
	cinstr(backdoor,backdoor_value);
	cinstr(prime_module,module_value);
	decr(prime_module,3,a);
	cinstr(b,b_value);
	ecurve_init(a,b,prime_module,MR_PROJECTIVE);
	/*a,b and p are no longer used
	 Let's init G and F*/
	cinstr(prime_module,x_value);
	G=epoint_init();
	F=epoint_init();
	epoint* TEMP=epoint_init();
	epoint_set(prime_module,prime_module,1,G);
	ecurve_mult(backdoor,G,F);

	//main cycle

	for(int i=0;i<(filesize / BLOCK);i++)
	{
		stage(&IV,&prime_module,TEMP,output_buffer);
		cipher(fd,wfd,output_buffer,BLOCK);
	}
	stage(&IV,&prime_module,TEMP,output_buffer);
	cipher(fd,wfd,output_buffer,filesize % BLOCK);
	fclose(fd);
	fclose(wfd);
	epoint_free(G);
	epoint_free(F);
	epoint_free(TEMP);
	mirkill(prime_module);
	mirkill(a);
	mirkill(b);
	mirkill(IV);
	mirkill(backdoor);
	mirexit();
	printf("done");
	return 0;
}
