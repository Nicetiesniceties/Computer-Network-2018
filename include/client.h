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

typedef enum {
	USER_MAIN_OPT_LOG_IN_SUCCESS = 0x01,
	USER_MAIN_OPT_EXIT = 0x02,
	USER_MAIN_OPT_AGAIN = 0x03,
} user_main_option;

typedef struct user_information
{
	char name[31];
	int login_status;
	char folder[61];
	char friends[100][31];
	int friend_num;
	uint16_t user_id;
} user_info;

typedef struct socket_information
{
	int port;
	char ip[20];
	int sockfd;
	char receiver_name[30 + 1];
	char file_path[100]; 
	uint16_t user_id;
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
int client_user_menu(user_info *cur_user, socket_info *server);
