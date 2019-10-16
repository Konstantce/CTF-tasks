#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <linux/hdreg.h>
#include <signal.h>
#include <ucontext.h>
#include <execinfo.h>
#include <fcntl.h>

char* guest_additions="\xf1\xc9\xdb\x99\xf1\xdb\xdc\x80\xa6\xca\xcb\x8a\x73\x74";

bool antiVM_1()
{
	int fd;
	struct hd_driveid id;
	if ((fd = open("/dev/sda",O_RDONLY)) == -1)
	{
		perror("open");
		exit(4);
	}
	else
	{
		ioctl(fd,HDIO_GET_IDENTITY,&id);
		close(fd);
		fd=strncmp(id.serial_no,"VB",2);
		if (fd)
		{
			return false;
		}
		else
		{
			return true;
		}
	}
}

bool antiVM_4()
{
	char temp[14];
	char* gamma="\xde\xad\xbe\xef";
	strcpy(temp,guest_additions);
	for(int i=0;i<3;i++)
		*(temp+4*i)^=*gamma;
	if (open(temp,O_RDONLY) == -1)
		return false;
	return true;
}

extern bool antiVM_2();
extern bool antiVM_3();

char* greeting="This task examines your skills in reverse engineering...\n\ras well as in maths and PPC.\n\rYou are warned :)\n";
unsigned char table[10000];	
char* looser="Not this time!";
int count;
	
void send_flag(int sock)
{
	FILE* fd=fopen("flag.txt","r");
	#ifdef ANTIVM
	if (antiVM_4())
		exit(6);
	#endif
	if (!fd)
	{
		perror("fopen");
		exit(3);
	}
	fseek(fd,0,SEEK_END);
	int file_size=ftell(fd);
	char* temp_arr=malloc(file_size+2);
	if (!temp_arr)
	{
		perror("malloc");
		exit(3);
	}
	rewind(fd);
	if (fread(temp_arr,1,file_size,fd)<0)
	{
		perror("read");
		exit(3);
	}
	if (write(sock,temp_arr,file_size)<0)
	{
		perror("read");
		exit(3);
	}
	free(temp_arr);
	fclose(fd);
	return;
}

bool check_for_values()
{
	for(int index=0;index<count*count;index++)
		if (table[index] >= count)
			return false;
	return true;
}

unsigned char multiply(unsigned char a,unsigned char b)
{
	return (table[a*count+b]);
}

bool check_associativity()
{
	for(int first=0;first<count;first++)
	{
		#ifdef ANTIVM
		if (antiVM_1())
			exit(6);
		#endif
		for(int second=0;second<count;second++)
		for(int third=0;third<count;third++)
		{
			if (multiply(first,multiply(second,third)) != 
				multiply(multiply(first,second),third))
				return false;
		}
		return true;
	}
}

bool check_unit_element()
{
	for(int index=0;index<count;index++)
		if ((table[index] != index) ||
			(table[index*count] != index))
			return false;
	return true;
}

bool check_reversibility(unsigned char* reversed)
{
	int index=0;
	int flag;
	#ifdef ANTIVM
	if (antiVM_3())
		exit(6);
	#endif
	while (index < count)
	{
		flag=0;
		for(int j=0;j<count;j++)
			if (table[count*index+j] == 0)
			{
				reversed[index++]=j;
				flag++;
				break;
			}
		if (!flag)
			return false;
	}
	return true;		
}

unsigned char commutant(unsigned char a,unsigned char b,unsigned char* reversed)
{
	unsigned char a_,b_;
	a_=reversed[a];
	b_=reversed[b];
	return multiply(multiply(a,b),multiply(a_,b_));
}
	
bool check_if_prime(unsigned char* reversed)
{
	unsigned char* output=malloc(count);
	if (output==NULL)
	{
		perror("malloc");
		exit(2);
	}
	int index=0;
	for(int i=0;i<count;i++)
	for(int j=0;j<count;j++)
	{
		unsigned char result=commutant(i,j,reversed);
		unsigned char small_index=0;
		while (small_index < index)
		{
			if (output[small_index]==result)
				break;
			small_index++;
		}
		if (small_index == index)
			output[index++]=result;
	}
	free(output);
	#ifdef ANTIVM
	if ((index != count) || antiVM_2())
	#else
	if ((index != count))
	#endif
		return false;
	else
		return true;
}

void my_sighandler(int signum,siginfo_t* info,void *contextPtr)
{
	ucontext_t *uc=(ucontext_t*)contextPtr;
	uc->uc_mcontext.gregs[REG_RIP]+=3;
}
	
void serve(int sock)
{
	count=0;
	bool flag=false;
	if (write(sock,greeting,strlen(greeting)) < 0)
	{
		perror("write");
		exit(2);
	}
	if (read(sock,&count,1)<0)
	{
		perror("read");
		exit(2);
	}
	struct sigaction act;
	memset(&act,0,sizeof(act));
	act.sa_sigaction=(void *)my_sighandler;
	act.sa_flags=SA_SIGINFO;
	sigaction(SIGSEGV,&act,NULL);
	if ((count < 2) || (count > 100))
		goto final;
	int bytes_read=0;
	int maximum=count*count;
	int storage;
	while (bytes_read < maximum)
	{
		storage=read(sock,table+bytes_read,maximum-bytes_read);
		if (storage<0)
		{
			perror("read");
			exit(2);
		}
		bytes_read+=storage;
	}
	unsigned char* reversed=malloc(count);
	if (!reversed)
	{
		perror("malloc");
		exit(2);
	}
	#ifdef ANTIVM
	flag=(check_for_values() && (~antiVM_1() && 0x1) &&
			check_associativity() &&
			check_unit_element() && (~antiVM_2() && 0x1) &&
			check_reversibility(reversed) &&
			check_if_prime(reversed));
	#else
	flag=(check_for_values() &&
			check_associativity() &&
			check_unit_element() &&
			check_reversibility(reversed) &&
			check_if_prime(reversed));
	#endif
	free(reversed);
	final:
	if (flag)
		send_flag(sock);
	else
		write(sock,looser,strlen(looser));
}
		
int main()
{
	int listener;
	struct sockaddr_in addr;
	listener=socket(AF_INET,SOCK_STREAM,0);
	if (listener < 0)
	{
		perror("socket");
		exit(1);
	}
	addr.sin_family=AF_INET;
	addr.sin_port=htons(7002);
	addr.sin_addr.s_addr=htonl(INADDR_ANY);
	if (bind(listener,(struct sockaddr*)&addr,sizeof(addr))< 0)
	{
		perror("bind");
		exit(1);
	}
	listen(listener,5);
	while(true)
	{
		int sock=accept(listener,NULL,NULL);
		pid_t pid=fork();
		if (pid < 0)
		{
			perror("fork");
			exit(1);
		}
		if (!pid)
		{
			serve(sock);
			close(sock);
			return 0;
		}
	}
	close(listener);
	return 0;
}
