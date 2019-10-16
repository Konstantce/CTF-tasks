#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <malloc.h>
/* We will use pure C library functions instead of std::map */
#include <glib.h>

#define MAX_CHAR_LENGTH 64
#define REQUEST_SIZE 128
#define ADMIN_PASSWORD_FILE "password.txt"
#define FLAG_FILE "flag.txt"
#define ADMIN "admin"
#define ERROR_CODE -1

char* invitation = ">> ";
char* incorrect_command = "Unknown command or incorrect number of parameteres.\n";
char* get_flag_prohibited = "This command is prohibited to non-admin users.\n";
char* user_not_found = "Target user is not registered.\n";
char* incorrect_password = "The password is incorrect.\n";
char* user_exists = "This user is already exists.\n";
char* no_user = "You must be logged in first!\n";
char* no_info = "There is no specific info for current user.";
char* no_data = "You have to specify user's name and password.\n";
char* no_info_specified = "You have to specify correct user's info.\n";
char* logout_error = "You are not even logged in!\n";

typedef struct _USER_INFO
{
	char password[MAX_CHAR_LENGTH];
	char info[MAX_CHAR_LENGTH];
}USER_INFO;

GHashTable* users;

char* rtrim(char *str)
{
	for (char *p = str+strlen(str)-1; p >= str; p--)
    	if (*p != '\r' && *p != '\n' && *p != ' ' && *p != '\t')
    		return str;
    	else
    		*p = '\0';
    return str;
}

char* insert_new_user(char* name,char* password,char* info)
{
	char* key = (char*)calloc(MAX_CHAR_LENGTH,1);
	/* The vulnerability is in the next line. */
	strncpy(key,rtrim(name),MAX_CHAR_LENGTH);
	key[MAX_CHAR_LENGTH] = 0x0;
	USER_INFO* value = (USER_INFO*)calloc(sizeof(USER_INFO),1);
	strncpy(value->password,rtrim(password),MAX_CHAR_LENGTH);
	value->password[MAX_CHAR_LENGTH] = 0x0;
	if (info)
	{
		strncpy(value->info,rtrim(info),MAX_CHAR_LENGTH);
		value->info[MAX_CHAR_LENGTH] = 0x0;
	}
	g_hash_table_insert(users,key,value);
	return key;
}

void get_flag(int sockfd,char* active_user)
{
	if ( (!active_user) || strcmp(active_user,ADMIN) )
		send(sockfd,get_flag_prohibited,strlen(get_flag_prohibited),0);
	else
	{
		FILE* fd = fopen(FLAG_FILE,"r");
		char flag[MAX_CHAR_LENGTH];
		memset(flag,0,sizeof(flag));
		fgets(flag,sizeof(flag),fd);
		flag[strlen(flag)] = '\n';
		send(sockfd,flag,strlen(flag),0);
	}
}

void login(int sockfd,char** pactive_user,char* user,char* password)
{
	if ( (!user) || (!password))
	{
		send(sockfd,no_data,strlen(no_data),0);
		return;
	}
	USER_INFO* target;
	char* orig_key;
	gboolean found = g_hash_table_lookup_extended(users,user,(gpointer*) &orig_key,
		(gpointer*) &target);
	if ( found == FALSE )
	{
		send(sockfd,user_not_found,strlen(user_not_found),0);
		return;
	}
	if ( strcmp(target->password,password) )
	{
		send(sockfd,incorrect_password,strlen(incorrect_password),0);
		return;
	}
	*pactive_user = orig_key;
}

void register_user(int sockfd,char** pactive_user,char* user,char* password)
{
	if ( (!user) || (!password))
	{
		send(sockfd,no_data,strlen(no_data),0);
		return;
	}
	USER_INFO* target = g_hash_table_lookup(users,user);
	if ( target )
		send(sockfd,user_exists,strlen(user_exists),0);
	else
		*pactive_user = insert_new_user(user,password,NULL);
}

void set_info(int sockfd,char* active_user,char* info)
{
	if ( !(active_user) )
	{
		send(sockfd,no_user,strlen(no_user),0);
		return;
	}
	if (!info)
		send(sockfd,no_info_specified,strlen(no_info_specified),0);
	else
	{
		USER_INFO* value = g_hash_table_lookup(users,active_user);
		strncpy(value->info,rtrim(info),MAX_CHAR_LENGTH);
		value->info[MAX_CHAR_LENGTH] = 0x0;
	}
}

void get_info(char* name,USER_INFO* info,int* psockfd)
{
	
	char* data =(strlen(info->info) ? info->info : no_info);
	char buffer[REQUEST_SIZE];
	memset(buffer,0,REQUEST_SIZE);
	int bytes_written = sprintf(buffer,"%s : %s\n",name,data);
	send(*psockfd,buffer,bytes_written,0);
}

void logout(int sockfd, char** pactive_user)
{
	if (!*pactive_user)
		send(sockfd,logout_error,strlen(logout_error),0);
	else
		*pactive_user = NULL;
}

void whoami(int sockfd, char* active_user)
{
	if (active_user)
	{
		char buffer[REQUEST_SIZE * 2];
		snprintf(buffer, REQUEST_SIZE * 2, "You are %s.\n", active_user);
		send(sockfd, buffer, strlen(buffer), 0);
	}
	else
		send(sockfd, logout_error, strlen(logout_error), 0);
}


int process_connection(int sockfd)
{
	ssize_t bytes_read;
	char request[REQUEST_SIZE];
	char* active_user = NULL;
	puts("New user arrived!");

	while ( send(sockfd,invitation,strlen(invitation),0) )
	{
		memset(request,0,REQUEST_SIZE);
		bytes_read = recv(sockfd,request,REQUEST_SIZE - 1,0);
		if (bytes_read <= 0)
		{
			fputs("Failed to read socket\n",stderr);
			return ERROR_CODE;
		}
		rtrim(request);
		printf("%d fd: User request: %s.\n", sockfd, request);
		char* param[2] = {NULL, NULL};
		char* line = request;
		int param_number =0;
		for (int i=0; i < 2; i++)
		{
			param[i] = strchr(line, ' ');
    		if ( param[i] )
    		{
      			*param[i] = 0x0;
      			line = ++param[i];
      			param_number++;
    		}
    		else
    			break;
    	}

		if ( !strcmp(request,"get_flag") && !param_number)
		{
			get_flag(sockfd, active_user);
			continue;
		}

		if ( !strcmp(request, "whoami") && !param_number)
		{
			whoami(sockfd, active_user);
			continue;
		}

		if ( !strcmp(request,"login") && (param_number == 2))
		{
			login(sockfd,&active_user,param[0],param[1]);
			continue;
		}
		if ( !strcmp(request,"register") && (param_number == 2))
		{
			register_user(sockfd,&active_user,param[0],param[1]);
			continue;
		}
		if ( !strcmp(request,"exit") && !param_number)
			break;
		if ( !strcmp(request,"get_info") && !param_number)
		{
			g_hash_table_foreach(users,(GHFunc)get_info,&sockfd);
			continue;
		}
		if ( !strcmp(request,"set_info") && (param_number == 1))
		{
			set_info(sockfd,active_user,param[0]);
			continue;
		}
		if ( !strcmp(request,"logout") && !param_number)
		{
			logout(sockfd,&active_user);
			continue;
		}
		send(sockfd,incorrect_command,strlen(incorrect_command),0);
	}
	printf("User with fd = %d has left.\n", sockfd);
}


int main(int argc,char* argv[])
{
	
	if ((argc != 2) || !atoi(argv[1]))
	{
		puts("Usage: database port");
		exit(0);
	}
	size_t port = atoi(argv[1]);	
	
	users = g_hash_table_new(g_str_hash,g_str_equal);
	FILE* fd = fopen(ADMIN_PASSWORD_FILE,"r");
	if (!fd)
	{
		fputs("Failed to open admin's password file\n",stderr);
		return ERROR_CODE;
	}
	char buffer[MAX_CHAR_LENGTH * 2];
	fgets(buffer,sizeof(buffer),fd);
	fclose(fd);
	char* info = strchr(buffer, ' ');
	(*info++) = 0x0;
	insert_new_user(ADMIN,buffer,info);

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