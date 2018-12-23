#include "client.h"
struct socket_information *read_server_info(int mode)
{
	struct socket_information *server =
	  (struct socket_information *) malloc(sizeof(struct socket_information));
	//mode 0: automatically reading infos in ../config/client.cfg
	//mode 1: reading standard input
	switch(mode)
	{
		case 1:
		{
			scanf("Please input the server ip: %s", server->ip);
			scanf("Please input the server port: %d", &(server->port));
			break;
		}
		case 0:
		{
			FILE * cfg_file = fopen("../config/client.cfg", "r");
			char c;
			if (cfg_file) 
			{
				while ((c = getc(cfg_file)) != EOF)
					putchar(c);
				fclose(cfg_file);
			}
			else
				fprintf(stderr, "Fail to read read client.cfg.\n");
			break;
		}
		default:
		{
			fprintf(stderr, "Wrong read_server_info() mode number\n");
			break;
		}
	}
	return server;
}
int client_init()
{
	int mode = 0;
	scanf("%d", &mode);
	struct socket_information *server = read_server_info(mode);
	return 0;
}
