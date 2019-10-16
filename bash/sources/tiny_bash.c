#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <malloc.h>
#include <fcntl.h>

#define BUF_SIZE 16
#define FLAG_FILE "this_file_contains_flag_cat_it.txt"
#define ERROR_CODE -1

char* greeting = "Welcome to our small secure shell.You are disallowed to execute several types of\
commands.Are you able to bypass these restrictions?\n";
char* invitation = ">> ";
char* prohibited_command = "This command is prohibited.\n";
char* invalid_command = "This command is incorrect.\n";

char* commands[] = {"cat", "flag", "txt", "bash", "python", "sh", "ls", "vi", "netcat", "nc", "perl", "args", "awk", "sed", "wc", "pico", "ed", "echo",
	"grep", "find", "bin", "su", "sudo", "system", "exec", "find", "regexp", "tail", "head", "less", "more", "cut", "pg", NULL};

void reap_exited_processes(int signum)
{
	pid_t pid;
	while (1)
	{
		pid = waitpid(-1,NULL,WNOHANG);
		if ( (!pid) || (pid == ERROR_CODE))
			break;
	}
}

void sock_send(int sockfd,char* msg)
{
	send(sockfd,msg,strlen(msg),0);
}

int check_command(char* cmd)
{
	char** target = commands;
	while (*target)
	{
		if (strstr(cmd,*target))
			return 0;
		target++;
	}
	return 1;
}

int process_connection(int sockfd)
{
	ssize_t bytes_read;
	char request[BUF_SIZE];
	char buffer[BUF_SIZE];

	memset(buffer,0,BUF_SIZE);
	sock_send(sockfd,greeting);

	for(;;)
	{
		sock_send(sockfd,invitation);
		memset(request,0,BUF_SIZE);
		bytes_read = recv(sockfd,request,BUF_SIZE - 1,0);
		if (bytes_read <= 0)
		{
			fputs("Failed to read socket\n",stderr);
			return ERROR_CODE;
		}
		request[bytes_read] = 0x0;
		printf("User request: %s\n",request);
		if (!check_command(request))
			sock_send(sockfd,prohibited_command);
		else
		{
			memcpy(buffer,request,bytes_read);
			if (system(buffer) == ERROR_CODE)
				sock_send(sockfd,invalid_command);
		}
	}
}
	

int main(int argc,char* argv[])
{	
	if ((argc != 2) || !atoi(argv[1]))
	{
		puts("Usage: bash port");
		exit(0);
	}
	size_t port = atoi(argv[1]);		
	
	struct sigaction sig_manager;
	sig_manager.sa_handler = reap_exited_processes;
	sig_manager.sa_flags = SA_RESTART;
	sigfillset(&sig_manager.sa_mask);
	sigaction(SIGCHLD,&sig_manager,NULL);

	int sd, client_sd;
	struct sockaddr_in server,client;
	socklen_t address_len;
	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd < 0)
	{
		fputs("Failed to acquire socket\n",stderr);
		return ERROR_CODE;
	}
	address_len = sizeof(struct sockaddr);
	memset(&server, 0, sizeof(server));
	memset(&client, 0, sizeof(client));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = INADDR_ANY;
	int set_reuse_addr = 1;
	if ( setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&set_reuse_addr,sizeof(int)) )
	{
		fputs("Setsockopt function failed.\n",stderr);
		return ERROR_CODE;
	}
	if ( bind(sd,(struct sockaddr*)&server,address_len) == ERROR_CODE)
	{
		fputs("Unable to bind socket.\n",stderr);
		return ERROR_CODE;
	}
	if ( listen(sd,SOMAXCONN) == ERROR_CODE)
	{
		fputs("Failed to listen on socket\n",stderr);
		return ERROR_CODE;
	}
	while (1)
	{
		client_sd = accept(sd, (struct sockaddr*)&client,&address_len);
		if (client_sd == ERROR_CODE)
		{
			fputs("Failed accepting connection, continuing\n",stderr);
			continue;
		}
		printf("Accepted connection from %s\n",inet_ntoa(client.sin_addr));
		int pid = fork();
		if (!pid)
		{
			process_connection(client_sd);
			close(client_sd);
			close(sd);
			exit(0);
		}
		close(client_sd);
	}
}
