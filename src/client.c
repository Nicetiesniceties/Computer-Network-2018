#include "client.h"
#include "common.h"
struct socket_information *read_server_info(int mode)//{{{
{
	struct socket_information *server =
	  (struct socket_information *) malloc(sizeof(struct socket_information));
	//mode 0: automatically reading infos in ../config/client.cfg
	//mode 1: reading standard input
	switch(mode)
	{
		case 0:
		{
			FILE * cfg_file = fopen("../config/client.cfg", "r");
			char c;
			if (cfg_file) 
			{
				int count_equal = 0;
				char temp[20];
				while ((c = getc(cfg_file)) != EOF)
					if(c == '=')
					{
						count_equal++;
						if(count_equal == 2)
							fscanf(cfg_file, "%s", server->ip);
						else if(count_equal == 3)
						{
							fscanf(cfg_file, "%s", temp);
							server->port = atoi(temp);
						}
					}
					//putchar(c);
				fclose(cfg_file);
			}
			else
				fprintf(stderr, "Fail to read read client.cfg.\n");
			break;
		}
		case 1:
		{
			fprintf(stdout, "Please input the server ip: ");
			fscanf(stdin, "%s", server->ip);
			fprintf(stdout, "Please input the server port: ");
			fscanf(stdin, "%d", &(server->port));
			break;
		}
		default:
		{
			fprintf(stderr, "Wrong read_server_info() mode number\n");
			break;
		}
	}
	fprintf(stdout, "Got it, server is at %s:%d\n", server->ip, server->port);
	return server;
}//}}}
int client_main_menu()//{{{
{
	int opt = -1;
	char login_opt[20];
	puts("----------------------------------------");
	puts("Entrance Hall");
	puts("> How can I help you? login/signup/exit");
	fprintf(stderr, "> ");
	while(1)
	{
		fgets(login_opt, 20, stdin);
		strtok(login_opt, "\n");
		if(strcmp(login_opt, "login") == 0)
			opt = client_login();
		else if(strcmp(login_opt, "signup") == 0)
			opt = client_sign_up();
		else if(strcmp(login_opt, "exit") == 0)
			opt = 0;
		else
			fprintf(stderr, "> Wrong instruction, please input again: ");
		if(opt != -1)
			break;
	}
	return opt;
}//}}}
int client_login()//{{{
{
	char acc[31], pwd[31];
	puts("----------------------------------------");
	puts("Login Hall");
	fprintf(stderr, "> Please input your account name: ");
	fgets(acc, 31, stdin);
	strtok(acc, "\n");
	fprintf(stderr, "> Please input your password: ");
	fgets(pwd, 31, stdin);
	strtok(pwd, "\n");
	return 0;
}//}}}
int client_sign_up()//{{{
{
	char acc[61], pwd[61];
	puts("----------------------------------------");
	puts("Sign up Counter");
	puts(">> Hello newbie!");
	fprintf(stderr, ">> Please input your account name: ");
	fgets(acc, 31, stdin);
	/*strlen error
	while(strlen(acc) > USER_LEN_MAX)
	{
		puts(">> Sorry, the length shall be at most 30 letters.");
		fprintf(stderr, "> Please input again: ");
	}*/
	
	fprintf(stderr, ">> Please input your password: ");
	fgets(pwd, 31, stdin);
	/*strlen error
	while(strlen(acc) > USER_LEN_MAX)
	{
		puts(">> Sorry, the length shall be at most 30 letters.");
		fprintf(stderr, "> Please input again: ");
	}*/
	
	return 0;
}//}}}

int start_connect(struct socket_information *server)//{{{
{
	int sockfd = 0;
	sockfd = socket(AF_INET , SOCK_STREAM , 0);
	# ifdef debug
	if (sockfd == -1)
		fprintf(stderr, "Fail to create a socket!\n");
	# endif
	struct sockaddr_in info;
	
	/* socket setting */
	memset(&info, 0, sizeof(info));//初始化，將struct涵蓋的bits設為0
	info.sin_family = PF_INET;//sockaddr_in為Ipv4結構

	/* localhost setting */
	info.sin_addr.s_addr = inet_addr(server -> ip);//IP address
	info.sin_port = htons(server -> port);

	/* connect! */
	int opt = 1;

	//set non-blocking
	if (ioctl(sockfd, FIONBIO, &opt) < 0) 
	{
		close(sockfd);
		perror("Fail to set non-bloakcing socket.");
		exit(0);
	}
	
	//printf("%d", connect(sockfd, (struct sockaddr *)&info, sizeof(info)));
	int connect_status = connect(sockfd, (struct sockaddr *)&info, sizeof(info));
	# ifdef debug
	//if(connect_status == -1)
	//	fprintf(stderr, "Connection Error!\n");
	//	exit(0);
	# endif
	
	//set back to blocking
	opt = 0;
	if (ioctl(sockfd, FIONBIO, &opt) < 0) 
	{
		close(sockfd);
		perror("ioctl");
		exit(0);
	}
	
	return sockfd;
}//}}}

int client_init()//{{{
{
	struct socket_information *server = read_server_info(0);
	fflush(stdout);
	int sockfd = start_connect(server);
	return 0;
}//}}}
int client_run()//{{{
{
	while(1)
	{
		int opt = client_main_menu();
		if(opt == 0)
			break;
	}
	
	return 0;
}
void client_destroy(struct socket_information *server)
{
	close(server -> sockfd);
	free(server);
}
