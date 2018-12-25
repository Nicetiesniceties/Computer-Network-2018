#include<stdio.h>
#include<stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <sys/ioctl.h>

typedef struct user_information
{
	char name[31];
	int login_status;
	char folder[61];
	char friends[100][31];
	int friend_num;
} user_info;

typedef struct socket_information
{
	int port;
	char ip[20];
	int sockfd;
} socket_info;
//main functions
socket_info * client_init();
int client_run(socket_info *server);
void client_destroy(socket_info *server);

//utility functions
user_info *client_login(socket_info *server);
user_info *client_sign_up(socket_info *server);
struct socket_information *read_server_info(int mode);
int start_connect(socket_info *server);
user_info *client_main_menu(struct socket_information *server);
int client_user_menu(user_info *cur_user);
