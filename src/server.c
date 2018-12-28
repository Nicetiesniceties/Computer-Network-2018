#include "server.h"
#include "utime.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>

static int parse_arg(server_info* server, int argc, char** argv);
static int server_start(char* port);

static void datum_login(server_info* server, int conn_fd, datum_protocol_login* login);
static int get_account_info(server_info* server, const char* user, account_info* info);
static void datum_sign_up(server_info* server, int conn_fd, datum_protocol_sign_up* sign_up);
static void datum_send_message(server_info* server, int conn_fd, datum_protocol_send_message* message);
static int check_recv_online(server_info* server, char* reciever);
static void datum_send_file(server_info* server, int conn_fd, datum_protocol_send_file* file);
static void datum_req_log(server_info* server, int conn_fd, datum_protocol_send_req send_req);
static void datum_logout(server_info* server, int conn_fd);

static char filebuf[4000];
static char serverPath[400];
#define MAX_CLIENT_SIZE getdtablesize()
#define BACKLOG 1024

void server_init(server_info** server, int argc, char** argv) {
  server_info* tmp;
  tmp = (server_info*) malloc(sizeof(server));
  if (!tmp) {
    fprintf(stderr, "server malloc fail\n");
    return;
  }
  memset(tmp, 0, sizeof(server));
  if (!parse_arg(tmp, argc, argv)) {
    fprintf(stderr, "Usage: %s [config file]\n", argv[0]);
    free(tmp);
    return;
  }
  int fd = server_start(tmp->arg.port);
  if (fd < 0) {
    fprintf(stderr, "server fail\n");
    free(tmp);
    return;
  }
  tmp->client = (client_info**)
      malloc(sizeof(client_info*) * MAX_CLIENT_SIZE);
  if (!tmp->client) {
    fprintf(stderr, "client list malloc fail\n");
    close(fd);
    free(tmp);
    return;
  }
  memset(tmp->client, 0, sizeof(client_info*) * MAX_CLIENT_SIZE);
  tmp->listen_fd = fd;
  *server = tmp;
}

int server_run(server_info* server){
	fd_set master;
	fd_set read_fds;
	int fdmax;

	int listener = server->listen_fd;
	int newfd;
	struct sockaddr_storage remoteaddr;
	socklen_t addrlen;

	FD_ZERO(&master);
	FD_ZERO(&read_fds);

	FD_SET(listener, &master);
	fdmax = listener;

	int conn_fd;
  fprintf(stderr, "server run\n");
  while(1){
      read_fds = master;
      select(fdmax + 1, &read_fds, NULL, NULL, NULL);
      for(int i = 0; i <= fdmax; i++){
      	if(FD_ISSET(i, &read_fds)){
      		conn_fd = i;
      		if(conn_fd == listener){
      			//new coming connection
      			addrlen = sizeof(remoteaddr);
      			newfd = accept(listener, (struct sockaddr*)&remoteaddr, &addrlen);
            if(newfd < 0){
              continue;
            }
      			FD_SET(newfd, &master);
      			if(newfd > fdmax)
      				fdmax = newfd;
      		}
      		else{
      			datum_protocol_header header;
      			memset(&header, 0, sizeof(header));
            int sz;
            sz = recv_message(conn_fd, &header, sizeof(header));
            fprintf(stderr, "%d\n", sz);
      			if(sz == 0){
      				fprintf(stderr, "conn_fd %d has left the connection abruptly", conn_fd);
              free(server->client[conn_fd]);
      				close(conn_fd);
      				FD_CLR(conn_fd, &master);
      				continue;
      			}
            if(sz == -1){
              fprintf(stderr, "%s\n", strerror(errno));
              continue;
            }
            fprintf(stderr, "fd is set %d\n", sz);
      			if(header.req.magic != DATUM_PROTOCOL_MAGIC_REQ){
      				continue;
      			}
      			switch (header.req.op){
      				case DATUM_PROTOCOL_OP_SIGN_UP:
                fprintf(stderr, "\nsign up\n");
                datum_protocol_sign_up signup;
                if(complete_message_with_header(conn_fd, &header, &signup)){
                  datum_sign_up(server, conn_fd, &signup);
                }
                break;
      				case DATUM_PROTOCOL_OP_LOGIN:
                fprintf(stderr, "\nlogin\n");
                datum_protocol_login login;
                if(complete_message_with_header(conn_fd, &header, &login)){
                  datum_login(server, conn_fd, &login);
                }
                break;
      				case DATUM_PROTOCOL_OP_SEND_MESSAGE:
                fprintf(stderr, "\nsend message\n");
                datum_protocol_send_message message;
                if(complete_message_with_header(conn_fd, &header, &message)){
                  datum_send_message(server, conn_fd, &message);
                }
                break;
      				/*case DATUM_PROTOCOL_OP_SEND_FILE:
                fprintf(stderr, "\nsend file\n");
                datum_protocol_send_file req;
                if(complete_message_with_header(conn_fd, &header, &req)){
                  send_file(server, conn_fd, &req);
                }
                break;*/
      				case DATUM_PROTOCOL_OP_REQ_LOG:
                fprintf(stderr, "\nrequest log\n", );
                datum_protocol_req_log req_log;
                if(complete_message_with_header(conn_fd, &header, &req_log)){
                  req_log(server, conn_fd, &req_log);  
                }
                break;
      				//case DATUM_PROTOCOL_OP_ADD_FRIEND:
      				case DATUM_PROTOCOL_OP_LOGOUT:
                fprintf(stderr, "\nlog out\n");
                if(complete_message_with_header(conn_fd, &header, &req)){
                  logout(server, conn_fd, &req);
                }
                break;
      				default:
                continue;
      				fprintf(stderr, "hahaha\n");
      			}

        	}
        }
      }
    }

}



static int server_start(char* port){
	int status;
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if((status = getaddrinfo(NULL, port, &hints, &res)) != 0){
        fprintf(stderr, "%s\n", gai_strerror(status));
        exit(1);
    }
    
    int sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if(sock_fd < 0){
        fprintf(stderr, "socket open error\n");
        exit(1);
    }

    if(bind(sock_fd, res->ai_addr, res->ai_addrlen) < 0){
        close(sock_fd);
        fprintf(stderr, "port bind error\n");
        exit(1);
    }

    if(listen(sock_fd, BACKLOG) < 0){
        close(sock_fd);
        fprintf(stderr, "port listen error\n");
        exit(1);
    }

    //sometime fail to retrieve host name by unknown reason
    char hostbuffer[256];
    char *IPbuffer;
    struct hostent *host_entry;
    int hostname;
    hostname = gethostname(hostbuffer, sizeof(hostbuffer));
    host_entry = gethostbyname(hostbuffer);
    IPbuffer = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0]));
    printf("Hostname: %s\n", hostbuffer);
    printf("Host IP: %s\n", IPbuffer);
    return sock_fd;
}

static int parse_arg(server_info *server, int argc, char ** argv){
	if (argc != 2) {
    return 0;
  }
  FILE* file = fopen(argv[1], "r");
  if (!file) {
    return 0;
  }
  fprintf(stderr, "reading config...\n");
  size_t keysize = 20, valsize = 20;
  char* key = (char*)malloc(sizeof(char) * keysize);
  char* val = (char*)malloc(sizeof(char) * valsize);
  int keylen, vallen;
  int accept_config_total = 2;
  int accept_config[2] = {0, 0};
  while ((keylen = getdelim(&key, &keysize, '=', file) - 1) > 0) {
    key[keylen] = '\0';
    vallen = getline(&val, &valsize, file) - 1;
    val[vallen] = '\0';
    fprintf(stderr, "config (%d, %s)=(%d, %s)\n", keylen, key, vallen, val);
    if (strcmp("path", key) == 0) {
      if (vallen <= sizeof(server->arg.path)) {
        strncpy(server->arg.path, val, vallen);
        accept_config[0] = 1;
      }
    } else if (strcmp("port", key) == 0) {
	    strncpy(server->arg.port, val, vallen);
	    accept_config[1] = 1;
    }
  }
  free(key);
  free(val);
  fclose(file);
  int i, test = 1;
  for (i = 0; i < accept_config_total; ++i) {
    test = test & accept_config[i];
  }
  if (!test) {
    fprintf(stderr, "config error\n");
    return 0;
  }
  return 1;
}

void server_destroy(server_info** server){
	server_info* tmp = *server;
	*server = 0;
	close(tmp->listen_fd);
	free(tmp->client);
	free(tmp);
}

// operations

static void datum_login(server_info* server, int conn_fd, datum_protocol_login* login){
  int succ = 1;
  client_info *info = (client_info*) malloc(sizeof(client_info));
  memset(info, 0, sizeof(client_info));
  if (!get_account_info(server, login->message.body.user, &(info->account))) {
    fprintf(stderr, "cannot find account\n");
    succ = 0;
  }
  if(succ && strcmp(login->message.body.passwd, info->account.passwd) != 0){
    fprintf(stderr, "passwd miss match\n");
    succ = 0;
  }

  datum_protocol_header header;
  memset(&header, 0, sizeof(header));
  header.res.magic = DATUM_PROTOCOL_MAGIC_RES;
  header.res.op = DATUM_PROTOCOL_OP_LOGIN;
  header.res.datalen = 0;

  if(succ){
    if(server->client[conn_fd]){
      free(server->client[conn_fd]);
    }
    info->conn_fd = conn_fd;
    server->client[conn_fd] = info;
    header.res.status = DATUM_PROTOCOL_STATUS_OK;
    header.res.client_id = info->conn_fd;
    fprintf(stderr, "user %s login success\n", info->account.user);
  }
  else{
    header.res.status = DATUM_PROTOCOL_STATUS_FAIL;
    free(info);
  }
  send_message(conn_fd, &header, sizeof(header));
  return;
}

static int get_account_info(server_info* server, const char* user, account_info* info){
  int ret = 0;
  char account_path[100] = {0};
  strcpy(account_path, server->arg.path);
  strcat(account_path, "/accountlist");
  FILE* file = fopen(account_path, "r");
  if(!file){
    return 0;
  }
  char buffer[50] = {0};
  while(fgets(buffer, 100, file) != NULL){
    char *s = strtok(buffer, ",");
    if(strcmp(user, s) == 0){
      strcpy(info->user, user);
      char *passwd = strtok(NULL, ",");
      passwd = strtok(passwd, "\n");
      strcpy(info->passwd, passwd);
      fprintf(stderr, "%s,%s\n", info->passwd, passwd);
      ret = 1;
    }
  }
  fclose(file);
  return ret;
}

static void datum_sign_up(server_info* server, int conn_fd, datum_protocol_sign_up* signup){
  int succ = 1;
  char *user, *passwd;
  user = signup->message.body.user;
  passwd = signup->message.body.passwd;
  char account_path[100] = {0};
  strcpy(account_path, server->arg.path);
  strcat(account_path, "/accountlist");
  FILE* file = fopen(account_path, "a+");
  char buffer[50] = {0};
  while(fgets(buffer, 100, file) != NULL){
    char *s = strtok(buffer, ",");
    if(strcmp(user, s) == 0){
      fprintf(stderr, "the user name %s has already registered\n", s);
      succ = 0;
      break;
    } 
  }

  datum_protocol_header header;
  memset(&header, 0, sizeof(header));
  header.res.magic = DATUM_PROTOCOL_MAGIC_RES;
  header.res.op = DATUM_PROTOCOL_OP_SIGN_UP;
  header.res.datalen = 0;

  if(succ){
    fprintf(stderr, "user:%s signup success\n", user);
    fprintf(file, "%s,%s\n", user, passwd);
    char path[100] = {0};
    strcpy(path, server->arg.path);
    strcat(path, "/user/");
    strcat(path, user);
    struct stat st;
    if(stat(path, &st) == -1){
      mkdir(path, 0700);
    }
    header.res.status = DATUM_PROTOCOL_STATUS_OK;
  }
  else{
    header.res.status = DATUM_PROTOCOL_STATUS_FAIL;
  }
  send_message(conn_fd, &header, sizeof(header));
  fclose(file);
  return;
}
static void datum_send_message(server_info* server, int conn_fd, datum_protocol_send_message* message){
  char *sender = server->client[conn_fd]->account.user;
  char *reciever = message->message.body.reciever;
  char sender_path[100] = {0};
  char reciever_path[100] = {0};
  strcpy(sender_path, server->arg.path);
  strcat(sender_path, "/user/");
  strcpy(reciever_path, sender_path);
  strcat(sender_path, sender);
  strcat(reciever_path, reciever);

  strcat(sender_path, "/");
  strcat(reciever_path, "/");

  strcat(sender_path, reciever);
  strcat(reciever_path, sender);

  struct stat st;
  if(stat(sender_path, &st) == -1){
    mkdir(sender_path, 0700);
  }
  if(stat(reciever_path, &st) == -1){
    mkdir(reciever_path, 0700);
  }

  char sender_log_path[100] = {0};
  char reciever_log_path[100] = {0};

  strcpy(sender_log_path, sender_path);
  strcpy(reciever_log_path, reciever_path);
  strcat(sender_log_path, "/.log");
  strcat(reciever_log_path, "/.log");

  FILE* sender_log = fopen(sender_path, "a+");
  FILE* reciever_log = fopen(reciever_path, "a+");

  int datalen;
  datalen = message->message.body.datalen;

  recv_message(conn_fd, filebuf, datalen);

  fprintf(sender_log, "\nfrom: %s\nto: %s\n", sender, reciever);
  fwrite(filebuf, sizeof(char), datalen, sender_log);
  fprintf(sender_log, "\n");

  fprintf(reciever_log, "\nfrom: %s\nto: %s\n", sender, reciever);
  fwrite(filebuf, sizeof(char), datalen, reciever_log);
  fprintf(reciever_log, "\n");

  fclose(sender_log);
  fclose(reciever_log);

  int recv_fd;
  recv_fd = check_recv_online(server, reciever);

  if(recv_fd == -1){
    char unsend_reciever_log_path[100];
    strcpy(unsend_reciever_log_path, reciever_path);
    strcat(unsend_reciever_log_path, "/.unsend");

    FILE* unsend_log = fopen(unsend_reciever_log_path, "a+");
    
    fwrite(filebuf, sizeof(char), datalen, unsend_log);
    fprintf(unsend_log, "\n");

    fclose(unsend_log);
  }
  else{
    send_message(recv_fd, message, sizeof(message));
  }

  datum_protocol_header header;
  memset(&header, 0, sizeof(header));
  header.res.magic = DATUM_PROTOCOL_MAGIC_RES;
  header.res.op = DATUM_PROTOCOL_OP_SEND_MESSAGE;
  header.res.datalen = 0;
  header.res.status = DATUM_PROTOCOL_STATUS_OK;

  send_message(conn_fd, &header, sizeof(header));

  return;
}

static int check_recv_online(server_info* server, char* reciever){
  int recv_fd = -1;
  for(int i = 0; i < MAX_CLIENT_SIZE; i++){
    if(server->client[i] != NULL){
      if(strcmp(server->client[i]->account.user, reciever) == 0){
        recv_fd = i;
      }
    }
  }
  return recv_fd;
}

static void datum_req_log(server_info* server, int conn_fd, datum_protocol_send_req send_req){
  char log_path[100] = {0};
  strcpy(log_path, server->arg.path);
  strcat(log_path, "/");
  strcat(log_path, server->client[conn_fd]->account.user);
  strcat(log_path, "/");
  strcat(log_path, send_req->message.body.receiver);
  strcat(log_path, "/.log");
  fprintf(stderr, "log file path:%s\n", log_path)
}