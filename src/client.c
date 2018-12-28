#include "client.h"
#include "common.h"
int GLOBAL_CLIENT_LOGIN_FLAG = 0;
char GLOBAL_NOW_CHATTING_USERNAME[USER_LEN_MAX] = "administrator";
//main functions
socket_info * client_init()//{{{
{
	struct socket_information *server = read_server_info(0);
	fflush(stdout);
	server->sockfd = start_connect(server);
	return server;
}//}}}

int client_run(socket_info *server)//{{{
{
	while(1)
	{
		user_info *cur_user = client_main_menu(server);
		if(cur_user->login_status == USER_MAIN_OPT_EXIT)
			break;
		else if(cur_user->login_status == USER_MAIN_OPT_LOG_IN_SUCCESS)
		{
			client_user_menu(cur_user, server);
		}
	}
	
	return 0;
}//}}}

void client_destroy(struct socket_information *server)//{{{
{
	close(server -> sockfd);
	free(server);
}//}}}

//utility for client_init()
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
	//fprintf(stdout, "Got it, server is at %s:%d\n", server->ip, server->port);
	return server;
}//}}}

int start_connect(socket_info *server)//{{{
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

	/*//set non-blocking
	if (ioctl(sockfd, FIONBIO, &opt) < 0) 
	{
		close(sockfd);
		perror("Fail to set non-bloakcing socket.");
		exit(0);
	}*/
	
	//printf("%d", connect(sockfd, (struct sockaddr *)&info, sizeof(info)));
	int connect_status = connect(sockfd, (struct sockaddr *)&info, sizeof(info));
	# ifdef debug
	if(connect_status == 12345)//-1)
	{
		fprintf(stderr, "Connection Error!\n");
		exit(0);
	}
	# endif
	
	//set back to blocking
	/*opt = 0;
	if (ioctl(sockfd, FIONBIO, &opt) < 0) 
	{
		close(sockfd);
		perror("ioctl");
		exit(0);
	}*/
	
	return sockfd;
}//}}}

//utility for client_run()

//functions for little utility{{{
void strip_path(char filename[FILENAME_LEN_MAX])
{
	char temp[FILENAME_LEN_MAX];
	char *ch;
	ch = strtok(filename, "/");
	while (ch != NULL)
	{
		strcpy(temp, ch);
		//printf("%s\n", ch);
		ch = strtok(NULL, "/");
	}
	strcpy(filename, temp);
}//}}}

user_info *client_main_menu(struct socket_information *server)//{{{
{
	user_info *cur_user = (user_info *) malloc(sizeof(user_info));
	cur_user->login_status = -1;
	char login_opt[20];
	while(1)
	{
		puts("----------------------------------------");
		puts("Entrance Hall");
		puts("> How can I help you? login/signup/exit");
		fprintf(stderr, "> ");
		fgets(login_opt, 20, stdin);
		strtok(login_opt, "\n");
		if(strcmp(login_opt, "login") == 0)
			cur_user = client_login(server);
		else if(strcmp(login_opt, "signup") == 0)
			cur_user = client_sign_up(server);
		else if(strcmp(login_opt, "exit") == 0)
			cur_user->login_status = USER_MAIN_OPT_EXIT;
		else
			fprintf(stderr, "> Wrong instruction, please input again!\n");
		if(cur_user->login_status != USER_MAIN_OPT_AGAIN)
			break;
	}
	return cur_user;
}//}}}

user_info *client_login(socket_info *server)//{{{
{
	user_info *cur_user = (user_info *) malloc(sizeof(user_info));
	cur_user->login_status = -1;
	cur_user->login_status = USER_MAIN_OPT_EXIT;
	char acc[31], pwd[31];
	puts("----------------------------------------");
	puts("Login Hall");
	fprintf(stderr, ">> Please input your account name: ");
	fgets(acc, 31, stdin);
	strtok(acc, "\n");
	fprintf(stderr, ">> Please input your password: ");
	fgets(pwd, 31, stdin);
	strtok(pwd, "\n");
	datum_protocol_login login_msg;
	datum_protocol_header header;
	memset(&login_msg, 0, sizeof(login_msg));
	memset(&header, 0, sizeof(header));
	
	//char buf[4096];
	strcpy(login_msg.message.body.user, acc);
	strcpy(login_msg.message.body.passwd, pwd);
	
	login_msg.message.header.req.magic = DATUM_PROTOCOL_MAGIC_REQ;
	login_msg.message.header.req.op = DATUM_PROTOCOL_OP_LOGIN;
	login_msg.message.header.req.datalen = sizeof(login_msg) - sizeof(login_msg.message.header);
	
	send_message(server -> sockfd, &login_msg, sizeof(login_msg));
	//send_message(server -> sockfd, buf, login_msg.message.body.datalen);
	int recv_len = recv_message(server -> sockfd, &header, sizeof(header));
	//if(recv_len != -1)
	if(header.res.status == DATUM_PROTOCOL_STATUS_OK)
	{
		puts("----------------------------------------");
		fprintf(stderr,">> Login successfully!\n");
		fprintf(stderr,">> Whelcome back %s!\n", acc);
		cur_user->login_status = USER_MAIN_OPT_LOG_IN_SUCCESS;
		strcpy(cur_user->name, acc);
		cur_user->user_id = header.res.client_id;
	}
	else if(header.res.status == DATUM_PROTOCOL_STATUS_FAIL)
	{
		puts("----------------------------------------");
		fprintf(stderr,">> Fail to login.\n");
		fprintf(stderr,">> Please try again!\n");
		cur_user->login_status = USER_MAIN_OPT_AGAIN;
	}

	return cur_user;
}//}}}

user_info* client_sign_up(struct socket_information *server)//{{{
{
	char acc[61], pwd[61];
	puts("----------------------------------------");
	puts("Sign up Counter");
	puts(">> Hello newbie!");
	fprintf(stderr, ">> Please input your account name: ");
	fgets(acc, 31, stdin);
	strtok(acc, "\n");
	/*strlen error
	while(strlen(acc) > USER_LEN_MAX)
	{
		puts(">> Sorry, the length shall be at most 30 letters.");
		fprintf(stderr, "> Please input again: ");
	}*/
	
	fprintf(stderr, ">> Please input your password: ");
	fgets(pwd, 31, stdin);
	strtok(pwd, "\n");
	/*strlen error
	while(strlen(acc) > USER_LEN_MAX)
	{
		puts(">> Sorry, the length shall be at most 30 letters.");
		fprintf(stderr, "> Please input again: ");
	}*/
	user_info *cur_user = (user_info *) malloc(sizeof(user_info));
	cur_user->login_status = USER_MAIN_OPT_EXIT;

	datum_protocol_sign_up sign_up_msg;
	datum_protocol_header header;
	memset(&sign_up_msg, 0, sizeof(sign_up_msg));
	memset(&header, 0, sizeof(header));
	
	//char buf[4096];
	strcpy(sign_up_msg.message.body.user, acc);
	strcpy(sign_up_msg.message.body.passwd, pwd);
	
	sign_up_msg.message.header.req.magic = DATUM_PROTOCOL_MAGIC_REQ;
	sign_up_msg.message.header.req.op = DATUM_PROTOCOL_OP_SIGN_UP;
	sign_up_msg.message.header.req.datalen = sizeof(sign_up_msg) - sizeof(sign_up_msg.message.header);
	
	send_message(server -> sockfd, &sign_up_msg, sizeof(sign_up_msg));
	//send_message(server -> sockfd, buf, sign_up_msg.message.body.datalen);
	int recv_len = recv_message(server -> sockfd, &header, sizeof(header));
	//if(recv_len != -1)
	if(header.res.status == DATUM_PROTOCOL_STATUS_OK)
	{
		puts("----------------------------------------");
		fprintf(stderr,"> Sign up successfully!\n");
		fprintf(stderr,"> Whelcome to Datum %s!\n", acc);
		strcpy(cur_user->name, acc);
		//make some directory for new_user
		struct stat st = {0};
		char path_name[40] = "../cdir/user/";
		strcat(path_name, cur_user->name);
		if (stat(path_name, &st) == -1) {
			mkdir(path_name, 0700);
		}
	}
	else if(header.res.status == DATUM_PROTOCOL_STATUS_FAIL)
	{
		puts("----------------------------------------");
		fprintf(stderr,"> Sorry, the name is already taken!\n");
		fprintf(stderr,"> Please retry!\n");
	}
	cur_user->login_status = USER_MAIN_OPT_AGAIN;
	return cur_user;
}//}}}

//send file{{{

void *thread_function_for_send_file(void *vargp)
{
	socket_info *message = (socket_info *)vargp;
	puts("----------------------------------------");
	fprintf(stderr, ">>> File '%s' sent to: %s\n", message->file_path, message->receiver_name);
	
	message->sockfd = start_connect(message);
	FILE * sent_file = fopen(message->file_path, "rb");
	char *buffer = (char*) malloc(sizeof(char) * 1024);

	//init header
	datum_protocol_send_file send_file_msg;
	datum_protocol_header header;
	memset(&send_file_msg, 0, sizeof(send_file_msg));
	memset(&header, 0, sizeof(header));
	//char buf[4096];
	strcpy(send_file_msg.message.body.receiver, message->receiver_name);
	fseek(sent_file, 0, SEEK_END);
	send_file_msg.message.body.datalen = ftell(sent_file);
	fprintf(stderr, "datalen: %d\n", send_file_msg.message.body.datalen);
	fseek(sent_file, 0, SEEK_SET);
	send_file_msg.message.header.req.magic = DATUM_PROTOCOL_MAGIC_REQ;
	send_file_msg.message.header.req.op = DATUM_PROTOCOL_OP_SEND_FILE;
	send_file_msg.message.header.req.status = DATUM_PROTOCOL_STATUS_MORE;
	send_file_msg.message.header.req.datalen = sizeof(send_file_msg) - sizeof(send_file_msg.message.header);
	send_file_msg.message.header.req.client_id = message->user_id;

	send_message(message -> sockfd, &send_file_msg, sizeof(send_file_msg));
	//send_message(server -> sockfd, buf, login_msg.message.body.datalen);
	int recv_len = recv_message(message -> sockfd, &header, sizeof(header));
	free(&send_file_msg);
	free(&header);
	uint64_t size = 0;
	//FILE *fp = fopen("./yoyoyo", "a");//for debug
	if(header.res.status == DATUM_PROTOCOL_STATUS_OK)
	{
		char filename[FILENAME_LEN_MAX];
		strcpy(filename, message->file_path);
		strip_path(filename);
		while((size = fread (buffer, 1, sizeof(buffer), sent_file)) != 0)
		{
			//init header
			datum_protocol_send_file send_file_ack;
			memset(&send_file_ack, 0, sizeof(send_file_ack));
			strcpy(send_file_ack.message.body.receiver, message->receiver_name);
			send_file_ack.message.header.req.magic = DATUM_PROTOCOL_MAGIC_REQ;
			send_file_ack.message.header.req.op = DATUM_PROTOCOL_OP_SEND_FILE;
			send_file_ack.message.header.req.status = DATUM_PROTOCOL_STATUS_MORE;
			send_file_ack.message.header.req.datalen = sizeof(send_file_ack) - sizeof(send_file_ack.message.header);
			send_file_ack.message.header.req.client_id = message->user_id;
			send_file_ack.message.body.datalen = size;
			strcpy(send_file_ack.message.body.filename, filename);
			memcpy(send_file_ack.message.body.file_buff, buffer, size);
			send_message(message->sockfd, &send_file_ack, sizeof(send_file_ack));
			free(&send_file_ack);
			//fwrite(buffer, 1, size, fp);//
		}
	}
	else
		fprintf(stderr, ">>> Fail to sent file\n");
	close(message->sockfd);
	//fclose(fp);//
	pthread_exit(NULL);
}

void send_file_to_server(user_info *cur_user)
{ 
	char file_path[40];
	char receiver_name[31];
	puts(">>> Who do you want to send to?");
	fprintf(stderr, ">>> ");
	fgets(receiver_name, 31, stdin);
	puts(">>> What file  do you want to do?");
	fprintf(stderr, ">>> ");
	fflush(stdout);
	fgets(file_path, 40, stdin);
	socket_info *message = (socket_info *)malloc(sizeof(socket_info));
	message = read_server_info(0);
	strcpy(message->file_path, file_path);
	strcpy(message->receiver_name, receiver_name);
	message->user_id = cur_user->user_id;
	//fprintf(stderr, "To: %s, %s\n", message->receiver_name, message->file_path);
	fflush(stdout);
	pthread_t tid;
	pthread_create(&tid, NULL, thread_function_for_send_file, message); 
	pthread_detach(tid);
	//pthread_join(tid, NULL);
	return;
}
//}}} 

//sycning and listening{{{
void *thread_function_for_sycning_and_listening(void *vargp)
{
	//this thread is for listen request only
	//socket_info *server = (socket_info *)vargp;
	/*give init message to the server*/
	int client_id = *((int *) vargp);
	socket_info *server = (socket_info *)malloc(sizeof(socket_info));
	server = read_server_info(0);
	server->sockfd = start_connect(server);
	datum_protocol_header header;
	memset(&header, 0, sizeof(header));
	header.req.magic = DATUM_PROTOCOL_MAGIC_REQ;
	header.req.op = DATUM_PROTOCOL_OP_LISTEN;
	header.req.client_id = client_id;
	send_message(server->sockfd, &header, sizeof(header));
	while(1)
	{
		datum_protocol_header header;
		int recv_len = recv_message(server -> sockfd, &header, sizeof(header));
		if(header.req.magic == DATUM_PROTOCOL_MAGIC_REQ)
		{
			switch(header.req.op)
			{
				case DATUM_PROTOCOL_OP_SEND_MESSAGE:
				{
					datum_protocol_send_message recv_msg;
					memset(&recv_msg, 0, sizeof(recv_msg));
					complete_message_with_header(server->sockfd, &header, &recv_msg);
					if(strcpy(recv_msg.message.body.sender, GLOBAL_NOW_CHATTING_USERNAME))
					{
						fprintf(stderr, ">>> %s\n", recv_msg.message.body.msg);
					}
					free(&recv_msg);
				}
				case DATUM_PROTOCOL_OP_SEND_FILE:
				{
					datum_protocol_send_file recv_file;
					memset(&recv_file, 0, sizeof(recv_file));
					complete_message_with_header(server->sockfd, &header, &recv_file);
					if(recv_file.message.header.req.status == DATUM_PROTOCOL_STATUS_MORE)
					{
						char filename[FILENAME_LEN_MAX] = "~/Downloads/";
						strcat(filename, recv_file.message.body.filename);
						FILE *fp = fopen(filename, "a");//for debug
						fwrite(recv_file.message.body.file_buff, 1, recv_file.message.body.datalen, fp);
					}
					free(&recv_file);
				}
			}
		}
		free(&header);
		if(!GLOBAL_CLIENT_LOGIN_FLAG)
			break;
		/*
		else if(header.res.magic == DATUM_PROTOCOL_MAGIC_RES)
		{
			switch(header.res.op)
			{
				case DATUM_PROTOCOL_OP_SEND_MESSAGE:
				{
				
				}
				case DATUM_PROTOCOL_OP_SEND_FILE:
				{
				
				}
				case DATUM_PROTOCOL_OP_REQ_LOG:
				{
				
				}
			}
		}
		*/
		/*
		receive message: req, op_send_message
		receive file: req, op_send_file, ok
		append file: req, op_send_file, more
		in ui: request and then log: res, op_send_req_log, ok
		*/
	}
	pthread_exit(NULL);
}
//}}}

int chat(socket_info *server)//{{{
{
	fprintf(stderr, ">>> Who do you want to chat with? \n");
	char homie[USER_LEN_MAX];
	fgets(homie, USER_LEN_MAX, stdin);
	strtok(homie, "\n");
	
	datum_protocol_req_log req_log;
	datum_protocol_header header;
	memset(&req_log, 0, sizeof(req_log));
	memset(&header, 0, sizeof(header));
	
	strcpy(req_log.message.body.homie, homie);
	req_log.message.header.req.magic = DATUM_PROTOCOL_MAGIC_REQ;
	req_log.message.header.req.op = DATUM_PROTOCOL_OP_REQ_LOG;
	req_log.message.header.req.client_id = server->user_id;
	req_log.message.header.req.datalen = sizeof(req_log) - sizeof(req_log.message.header);
	
	send_message(server -> sockfd, &req_log, sizeof(req_log));
	//send_message(server -> sockfd, buf, req_log.message.body.datalen);
	int recv_len = recv_message(server -> sockfd, &header, sizeof(header));
	//if(recv_len != -1)
	if(header.res.status == DATUM_PROTOCOL_STATUS_OK)
	{
		strcpy(GLOBAL_NOW_CHATTING_USERNAME, homie);
		datum_protocol_send_message req_log;
		memset(&req_log, 0, sizeof(req_log));
		complete_message_with_header(server->sockfd, &header, &req_log);
		puts("----------------------------------------");
		fprintf(stderr, "%s", req_log.message.body.msg);
		fprintf(stderr,">>> Say hi to %s!\n", homie);
		while(1)//keep chatting with your homie
		{
			char some_words[MSG_LEN_MAX];
			fprintf(stderr,">>> ");
			fgets(some_words, MSG_LEN_MAX, stdin);
			strtok(some_words, "\n");
			if(strcmp(some_words, "exit()") == 0)
			{
				strcpy(GLOBAL_NOW_CHATTING_USERNAME, "administrator");
				return 0;
			}
			datum_protocol_send_message sent_msg;
			memset(&sent_msg, 0, sizeof(sent_msg));
			memset(&header, 0, sizeof(header));
			
			strcpy(sent_msg.message.body.msg, some_words);
			strcpy(sent_msg.message.body.receiver, homie);
			sent_msg.message.header.req.magic = DATUM_PROTOCOL_MAGIC_REQ;
			sent_msg.message.header.req.op = DATUM_PROTOCOL_OP_SEND_MESSAGE;
			sent_msg.message.header.req.client_id = server->user_id;
			sent_msg.message.header.req.datalen = sizeof(sent_msg) - sizeof(sent_msg.message.header);
			
			send_message(server -> sockfd, &sent_msg, sizeof(sent_msg));
			//send_message(server -> sockfd, buf, sent_msg.message.body.datalen);
			int recv_len = recv_message(server -> sockfd, &header, sizeof(header));
			//if(recv_len != -1)
			if(header.res.status == DATUM_PROTOCOL_STATUS_OK)
			{
				fprintf(stderr, ">>> --------\n");
			}
			else if(header.res.status == DATUM_PROTOCOL_STATUS_FAIL)
			{
				fprintf(stderr, ">>> Fail to send message\n");
				return 0;
			}
		}
	}
	else if(header.res.status == DATUM_PROTOCOL_STATUS_FAIL)
	{
		puts("----------------------------------------");
		fprintf(stderr,">>> Who the fuck is %s? U friends?\n", homie);
		fprintf(stderr,">>> Please try again!\n");
		return 0;
	}
	return 0;
}//}}}

int add_friend(socket_info *server)//{{{
{
	fprintf(stderr, ">>> Who do you want to add? \n");
	char homie[USER_LEN_MAX];
	fgets(homie, USER_LEN_MAX, stdin);
	strtok(homie, "\n");
	
	datum_protocol_add_friend add_friend_req;
	datum_protocol_header header;
	memset(&add_friend_req, 0, sizeof(add_friend_req));
	memset(&header, 0, sizeof(header));
	
	strcpy(add_friend_req.message.body.homie, homie);
	add_friend_req.message.header.req.magic = DATUM_PROTOCOL_MAGIC_REQ;
	add_friend_req.message.header.req.op = DATUM_PROTOCOL_OP_ADD_FRIEND;
	add_friend_req.message.header.req.client_id = server->user_id;
	add_friend_req.message.header.req.datalen = sizeof(add_friend_req) - sizeof(add_friend_req.message.header);
	
	send_message(server -> sockfd, &add_friend_req, sizeof(add_friend_req));
	//send_message(server -> sockfd, buf, req_log.message.body.datalen);
	int recv_len = recv_message(server -> sockfd, &header, sizeof(header));
	//if(recv_len != -1)
	if(header.res.status == DATUM_PROTOCOL_STATUS_OK)
	{
		puts("----------------------------------------");
		fprintf(stderr,">>> Sucessfully add %s!\n", homie);
	}
	else if(header.res.status == DATUM_PROTOCOL_STATUS_FAIL)
	{
		puts("----------------------------------------");
		fprintf(stderr,">>> Who the fuck is %s?\n", homie);
		fprintf(stderr,">>> Please try again!\n");
		return 0;
	}
	return 0;
}//}}}

int client_user_menu(user_info *cur_user, socket_info *server)//{{{
{
	GLOBAL_CLIENT_LOGIN_FLAG = 1;
	pthread_t tid;
	pthread_create(&tid, NULL, thread_function_for_sycning_and_listening, server); 
	pthread_detach(tid);
	char user_opt[20];
	int opt = -1;
	while(1)
	{
		puts("----------------------------------------");
		puts("User Menu");
		puts(">>> What do you want to do? add_friend/chat/logout/send_file");
		fprintf(stderr, ">>> ");
		fflush(stdout);
		fgets(user_opt, 20, stdin);
		strtok(user_opt, "\n");
		if(strcmp(user_opt, "add_friend") == 0)
			add_friend(server);
		else if(strcmp(user_opt, "chat") == 0)
			chat(server);
		else if(strcmp(user_opt, "send_file") == 0)
			send_file_to_server(cur_user);
		else if(strcmp(user_opt, "logout") == 0)
			opt = 0, GLOBAL_CLIENT_LOGIN_FLAG = 0;
		else
			fprintf(stderr, ">>> Wrong instruction, please input again!\n");
		if(opt == 0)
			break;
	}
	return 0;
}//}}}
