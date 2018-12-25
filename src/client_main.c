#include "client.h"
//testing branch
int main()
{
	struct socket_information *server;
	server = client_init();
	client_run(server);
	//client_destroy();
	return 0;
}
