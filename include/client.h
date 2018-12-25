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

struct socket_information
{
	int port;
	char ip[20];
	int sockfd;
};

int client_init();
int client_run();
void client_destroy(struct socket_information *server);
