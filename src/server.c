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
#include <pthread.h>
#include <errno.h>

static int parse_arg(server_info* server, int argc, char** argv);
static int server_start(char* port);

static void datum_login(server_info* server, int conn_fd, datum_protocol_login* login);
static void datum_listen(server_info* server, int conn_fd, datum_protocol_header* listen);
static int get_account_info(server_info* server, const char* user, account_info* info);
static void datum_sign_up(server_info* server, int conn_fd, datum_protocol_sign_up* sign_up);
static void datum_add_friend(server_info* server, int conn_fd, datum_protocol_add_friend* add_friend);
static void datum_send_message(server_info* server, int conn_fd, datum_protocol_send_message* message);
static int check_recv_online(server_info* server, char* receiver);
static void datum_send_file(server_info* server, int conn_fd, datum_protocol_send_file* file);
static void datum_req_log(server_info* server, int conn_fd, datum_protocol_req_log* req_log);
static void datum_req_list(server_info* server, int conn_fd, datum_protocol_req_list* req_list);
static int check_friend(server_info* server, char *s1, char *s2);
static void datum_block(server_info* server, int conn_fd, datum_protocol_add_friend* block);
static int check_if_block(server_info* server, char* sender, char* receiver);
static void datum_unblock(server_info* server, int conn_fd, datum_protocol_add_friend* unblock);

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
            fprintf(stderr, "new come%d\n", newfd);
      		}
      		else{
      			datum_protocol_header header;
      			memset(&header, 0, sizeof(header));
            int sz;
            sz = recv_message(conn_fd, &header, sizeof(header));
      			if(sz == 0){
      				fprintf(stderr, "conn_fd %d has left the connection abruptly\n", conn_fd);
              if(server->client[conn_fd] != NULL){
                free(server->client[conn_fd]);
                server->client[conn_fd] = NULL;
              }
      				close(conn_fd);
      				FD_CLR(conn_fd, &master);
      				continue;
      			}
            if(sz == -1){
              fprintf(stderr, "%s\n", strerror(errno));
              continue;
            }
      			if(header.req.magic != DATUM_PROTOCOL_MAGIC_REQ){
      				continue;
      			}
      			switch (header.req.op){
      				case DATUM_PROTOCOL_OP_SIGN_UP:
                fprintf(stderr, "\nsign up\n");
                datum_protocol_sign_up signup;
                memset(&signup, 0, sizeof(signup));
                if(complete_message_with_header(conn_fd, &header, &signup)){
                  datum_sign_up(server, conn_fd, &signup);
                }
                break;
      				case DATUM_PROTOCOL_OP_LOGIN:
                fprintf(stderr, "\nlogin\n");
                datum_protocol_login login;
                memset(&login, 0, sizeof(login));
                if(complete_message_with_header(conn_fd, &header, &login)){
                  datum_login(server, conn_fd, &login);
                }
                break;
      				case DATUM_PROTOCOL_OP_SEND_MESSAGE:
                fprintf(stderr, "\nsend message\n");
                datum_protocol_send_message message;
                memset(&message, 0, sizeof(message));
                if(complete_message_with_header(conn_fd, &header, &message)){
                  datum_send_message(server, conn_fd, &message);
                }
                break;
      				case DATUM_PROTOCOL_OP_SEND_FILE:{
                //printf(stderr, "\nsend file\n");
                datum_protocol_send_file send_file;
                memset(&send_file, 0, sizeof(send_file));
                if(complete_message_with_header(conn_fd, &header, &send_file)){
                  datum_send_file(server, conn_fd, &send_file);
                }
                break;
              }
      				case DATUM_PROTOCOL_OP_REQ_LOG:
                fprintf(stderr, "\nrequest log\n");
                datum_protocol_req_log req_log;
                memset(&req_log, 0, sizeof(req_log));
                if(complete_message_with_header(conn_fd, &header, &req_log)){
                  datum_req_log(server, conn_fd, &req_log);  
                }
                break;
      				case DATUM_PROTOCOL_OP_ADD_FRIEND:
                fprintf(stderr, "\nadd friend\n");
                datum_protocol_add_friend add_friend;
                memset(&add_friend, 0, sizeof(add_friend));
                if(complete_message_with_header(conn_fd, &header, &add_friend)){
                  datum_add_friend(server, conn_fd, &add_friend);
                }
                break;
      				case DATUM_PROTOCOL_OP_LOGOUT:
                fprintf(stderr, "\nlog out\n");
                if(server->client[conn_fd] != NULL){
                  FD_CLR(server->client[conn_fd]->req_fd, &master);
                  close(server->client[conn_fd]->req_fd);
                  free(server->client[conn_fd]);
                  server->client[conn_fd] = NULL;
                }
                break;
              case DATUM_PROTOCOL_OP_LISTEN:
                fprintf(stderr, "\nlisten\n");
                datum_listen(server, conn_fd, &header);
                break;
              case DATUM_PROTOCOL_OP_REQ_LIST:
                fprintf(stderr, "\nreq list\n");
                datum_protocol_req_list req_list;
                memset(&req_list, 0, sizeof(req_list));
                if(complete_message_with_header(conn_fd, &header, &req_list)){
                  datum_req_list(server, conn_fd, &req_list);
                }
                break;
              case DATUM_PROTOCOL_OP_BLOCK:
                fprintf(stderr, "\nblock\n");
                datum_protocol_add_friend block;
                memset(&block, 0, sizeof(block));
                if(complete_message_with_header(conn_fd, &header, &block)){
                  datum_block(server, conn_fd, &block);
                }
                break;
              case DATUM_PROTOCOL_OP_UNBLOCK:
                fprintf(stderr, "\nunblock");
                datum_protocol_add_friend unblock;
                memset(&unblock, 0, sizeof(unblock));
                if(complete_message_with_header(conn_fd, &header, &unblock)){
                  datum_unblock(server, conn_fd, &unblock);
                }
                break;
      				default:
              continue;
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
    fprintf(stderr, "account fail\n");
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
    if(server->client[conn_fd] != NULL){
      free(server->client[conn_fd]);
    }
    info->conn_fd = conn_fd;
    server->client[conn_fd] = info;
    header.res.status = DATUM_PROTOCOL_STATUS_OK;
    header.res.client_id = info->conn_fd;
    fprintf(stderr, "client id is %d\n", info->conn_fd);
    fprintf(stderr, "user %s login success\n", login->message.body.user);
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
  while(fgets(buffer, 50, file) != NULL){
    char *s = strtok(buffer, ",");
    if(strcmp(user, s) == 0){
      strcpy(info->user, user);
      char *passwd = strtok(NULL, ",");
      passwd = strtok(passwd, "\n");
      strcpy(info->passwd, passwd);
      fprintf(stderr, "%s,%s\n", user, passwd);
      fprintf(stderr, "%s,%s\n", info->user, info->passwd);
      ret = 1;
    }
    memset(buffer, 0, sizeof(buffer));
  }
  fclose(file);
  return ret;
}

static void datum_listen(server_info* server, int conn_fd, datum_protocol_header* listen){
  int client_id = listen->req.client_id;
  fprintf(stderr, "%d is listen to %d", client_id, conn_fd);
  server->client[client_id]->req_fd = conn_fd;
  return;
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
    strcat(path, "/friendlist");
    FILE* fp = fopen(path, "a");
    fclose(fp);
    header.res.status = DATUM_PROTOCOL_STATUS_OK;
  }
  else{
    header.res.status = DATUM_PROTOCOL_STATUS_FAIL;
  }
  send_message(conn_fd, &header, sizeof(header));
  fclose(file);
  return;
}

static void datum_add_friend(server_info* server, int conn_fd, datum_protocol_add_friend* add_friend){
  int succ = 0;
  char *sender = server->client[conn_fd]->account.user;
  char *homie = add_friend->message.body.homie;
  
  char account_path[100] = {0};
  strcpy(account_path, server->arg.path);
  strcat(account_path, "/accountlist");
  FILE* file = fopen(account_path, "r");
  if(!file){
    return;
  }
  char buffer[50] = {0};
  while(fgets(buffer, 50, file) != NULL){
    char *s = strtok(buffer, ",");
    if(strcmp(homie, s) == 0){
      succ = 1;
    }
  }
  fclose(file);

  if(succ)
    succ = !check_if_block(server, sender, homie);
  if(succ)
    succ = !check_friend(server, sender, homie);
  datum_protocol_add_friend res;
  res.message.header.res.magic = DATUM_PROTOCOL_MAGIC_RES;
  res.message.header.res.op = DATUM_PROTOCOL_OP_ADD_FRIEND;
  res.message.header.res.datalen = 0;

  if(succ){
    char sender_path[100] = {0};
    char homie_path[100] = {0};
    char sender_list_path[100] = {0};
    char homie_list_path[100] = {0};
    strcpy(sender_path, server->arg.path);
    strcat(sender_path, "/user/");
    strcpy(homie_path, sender_path);
    strcat(sender_path, sender);
    strcpy(sender_list_path, sender_path);
    strcat(sender_list_path, "/friendlist");
    strcat(sender_path, "/");
    strcat(sender_path, homie);

    strcat(homie_path, homie);
    strcpy(homie_list_path, homie_path);
    strcat(homie_list_path, "/friendlist");
    strcat(homie_path, "/");
    strcat(homie_path, sender);

    struct stat st;
    fprintf(stderr, "%s add friend %s\n", sender, homie);
    if(stat(sender_path, &st) == -1){
      mkdir(sender_path, 0700);
    }
    if(stat(homie_path, &st) == -1){
      mkdir(homie_path, 0700);
    }

    FILE* fp;
    fp = fopen(sender_list_path, "a");
    fprintf(fp, "%s\n", homie);
    fclose(fp);
    fp = fopen(homie_list_path, "a");
    fprintf(fp, "%s\n", sender);
    fclose(fp);

    res.message.header.res.status = DATUM_PROTOCOL_STATUS_OK;
  }
  else{
    res.message.header.res.status = DATUM_PROTOCOL_STATUS_FAIL;
  }
  send_message(conn_fd, &res, sizeof(res));
  return;
}

static void datum_send_message(server_info* server, int conn_fd, datum_protocol_send_message* message){
  int succ = 1; 
  char *sender = server->client[conn_fd]->account.user;
  char *receiver = message->message.body.receiver;
  fprintf(stderr, "from %s to %s\n", sender, receiver);


  char sender_path[100] = {0};
  char receiver_path[100] = {0};
  strcpy(sender_path, server->arg.path);
  strcat(sender_path, "/user/");
  strcpy(receiver_path, sender_path);
  strcat(sender_path, sender);
  strcat(receiver_path, receiver);
  strcat(sender_path, "/");
  strcat(receiver_path, "/");
  strcat(sender_path, receiver);
  strcat(receiver_path, sender);

  succ = check_friend(server, sender, receiver);
  fprintf(stderr, "%s to %s\n", sender_path, receiver_path);
  if(succ)
    succ = !check_if_block(server, sender, receiver);

  datum_protocol_header header;
  memset(&header, 0, sizeof(header));
  header.res.magic = DATUM_PROTOCOL_MAGIC_RES;
  header.res.op = DATUM_PROTOCOL_OP_SEND_MESSAGE;
  header.res.datalen = 0;

  if(succ){
    header.res.status = DATUM_PROTOCOL_STATUS_OK;
    char sender_log_path[100] = {0};
    char receiver_log_path[100] = {0};

    strcpy(sender_log_path, sender_path);
    strcpy(receiver_log_path, receiver_path);
    strcat(sender_log_path, "/.log");
    strcat(receiver_log_path, "/.log");

    FILE* sender_log = fopen(sender_log_path, "a+");
    FILE* receiver_log = fopen(receiver_log_path, "a+");

    fprintf(sender_log, "from: %s\n", sender);
    fwrite(message->message.body.msg, 1, message->message.body.datalen, sender_log);
    fprintf(sender_log, "\n");

    fprintf(receiver_log, "from: %s\n", sender);
    fwrite(message->message.body.msg, 1, message->message.body.datalen, receiver_log);
    fprintf(receiver_log, "\n");

    fclose(sender_log);
    fclose(receiver_log);

    int recv_fd;
    recv_fd = check_recv_online(server, receiver);

    if(recv_fd == -1){
      fprintf(stderr, "%s isn't online\n", receiver);
    }
    else{
      send_message(recv_fd, message, sizeof(datum_protocol_send_message));
    }
  }
  else{
    fprintf(stderr, "%s and %s is not friend\n",sender, receiver);
    header.res.status = DATUM_PROTOCOL_STATUS_FAIL;
  }

  send_message(conn_fd, &header, sizeof(header));

  return;
}

static int check_recv_online(server_info* server, char* receiver){
  int recv_fd = -1;
  for(int i = 0; i < MAX_CLIENT_SIZE; i++){
    if(server->client[i] != NULL){
      if(strcmp(server->client[i]->account.user, receiver) == 0){
        recv_fd = server->client[i]->req_fd;
      }
    }
  }
  return recv_fd;
}

static void datum_req_log(server_info* server, int conn_fd, datum_protocol_req_log* req_log){
  int succ = 1;
  char log_path[100] = {0};
  strcpy(log_path, server->arg.path);
  strcat(log_path, "/user/");
  strcat(log_path, server->client[conn_fd]->account.user);
  fprintf(stderr, "req user:%s\n", server->client[conn_fd]->account.user);
  strcat(log_path, "/");
  strcat(log_path, req_log->message.body.homie);

  succ = check_friend(server, server->client[conn_fd]->account.user, req_log->message.body.homie);
  if(!succ){
    succ = 0;
    fprintf(stderr, "%s is not %s's friend\n", req_log->message.body.homie, server->client[conn_fd]->account.user);
  }

  datum_protocol_req_log res;
  memset(&res, 0, sizeof(res));
  res.message.header.res.magic = DATUM_PROTOCOL_MAGIC_RES;
  res.message.header.res.op = DATUM_PROTOCOL_OP_REQ_LOG;
  res.message.header.res.datalen = sizeof(res) - sizeof(res.message.header);
  strcpy(res.message.body.homie, req_log->message.body.homie);

  if(succ){
    strcat(log_path, "/.log");
    fprintf(stderr, "log file path:%s\n", log_path);

    res.message.header.res.status = DATUM_PROTOCOL_STATUS_OK;
    FILE* fp = fopen(log_path, "a+");
    size_t size = 0;
    size = fread(res.message.body.msg, 1, 10240, fp);
    fprintf(stderr, "size:%d\n", size);
    res.message.body.datalen = size;
    fprintf(stderr, "size:%d\n", res.message.body.datalen);
    fclose(fp);
  }
  else{
    res.message.header.res.status = DATUM_PROTOCOL_STATUS_FAIL;
  }
  send_message(conn_fd, &res, sizeof(res));
  return;
}


void datum_send_file(server_info* server, int conn_fd, datum_protocol_send_file* file){
  int recv_fd = -1;
  if(file->message.header.req.status == DATUM_PROTOCOL_STATUS_OK){
    char *receiver = file->message.body.receiver;
    recv_fd = check_recv_online(server, receiver);
    datum_protocol_header header;
    memset(&header, 0, sizeof(header));
    header.res.magic = DATUM_PROTOCOL_MAGIC_RES;
    header.res.op = DATUM_PROTOCOL_OP_SEND_FILE;
    header.res.datalen = 0;

    if(recv_fd == -1){
      fprintf(stderr, "receiver %s isn't online\n", receiver);
      header.res.status = DATUM_PROTOCOL_STATUS_FAIL;
      send_message(conn_fd, &header, sizeof(header));
    }
    else{

      header.res.status = DATUM_PROTOCOL_STATUS_OK;
      send_message(conn_fd, &header, sizeof(header));

    }
  }
  else{
    char *receiver = file->message.body.receiver;
    recv_fd = check_recv_online(server, receiver);
    if(recv_fd == -1){
      return;
    }
    send_message(recv_fd, file, sizeof(datum_protocol_send_file));
    //fprintf(stderr, "send file to %d", recv_fd);
  }
  return;
}

static void datum_req_list(server_info* server, int conn_fd, datum_protocol_req_list* req_list){
  int succ = 1;
  char log_path[100] = {0};
  strcpy(log_path, server->arg.path);
  strcat(log_path, "/user/");
  strcat(log_path, server->client[conn_fd]->account.user);
  fprintf(stderr, "req user:%s\n", server->client[conn_fd]->account.user);
  strcat(log_path, "/friendlist");


  datum_protocol_req_list res;
  memset(&res, 0, sizeof(res));
  res.message.header.res.magic = DATUM_PROTOCOL_MAGIC_RES;
  res.message.header.res.op = DATUM_PROTOCOL_OP_REQ_LIST;
  res.message.header.res.datalen = sizeof(res) - sizeof(res.message.header);

  if(succ){

    fprintf(stderr, "list file path:%s\n", log_path);

    res.message.header.res.status = DATUM_PROTOCOL_STATUS_OK;
    char online_list_path[100] = {0};
    strcpy(online_list_path, log_path);
    strcat(online_list_path, "_online");
    FILE *fp1, *fp2;
    fp1 = fopen(log_path, "r");
    fp2 = fopen(online_list_path, "w");
    char buffer[50] = {0};
    while(fgets(buffer, 50, fp1) != NULL){
      char *s = strtok(buffer, "\n");
      if(check_recv_online(server, s) != -1){
        fprintf(fp2, "%s, online\n", s);
      }
      else{
        fprintf(fp2, "%s, offline\n", s);
      }
    }

    fclose(fp1);
    fclose(fp2);
    FILE* fp = fopen(online_list_path, "a+");
    size_t size = 0;
    size = fread(res.message.body.msg, 1, 10240, fp);
    res.message.body.datalen = size;
    fprintf(stderr, "%s\n", res.message.body.msg);
    fclose(fp);
  }
  else{
    res.message.header.res.status = DATUM_PROTOCOL_STATUS_FAIL;
  }
  send_message(conn_fd, &res, sizeof(res));
  return;
}


static int check_friend(server_info* server, char *s1, char *s2){
  int succ1 = 0, succ2 = 0;
  char sender_path[100] = {0};
  char receiver_path[100] = {0};
  strcpy(sender_path, server->arg.path);
  strcat(sender_path, "/user/");
  strcpy(receiver_path, sender_path);
  strcat(sender_path, s1);
  strcat(receiver_path, s2);

  strcat(sender_path, "/friendlist");
  strcat(receiver_path, "/friendlist");

  fprintf(stderr, "%s %s\n", sender_path, receiver_path);
  FILE* file = fopen(sender_path, "r");
  if(!file){
    return 0;
  }
  char buffer[50] = {0};
  while(fgets(buffer, 50, file) != NULL){
    char *s = strtok(buffer, "\n");
    if(strcmp(s2, s) == 0){
      succ1 = 1;
    }
    memset(buffer, 0, sizeof(buffer));
  }
  fclose(file);

  file = fopen(receiver_path, "r");
  if(!file){
    return 0;
  }
  while(fgets(buffer, 50, file) != NULL){
    char *s = strtok(buffer, "\n");
    if(strcmp(s1, s) == 0){
      succ2 = 1;
    }
    memset(buffer, 0, sizeof(buffer));
  }
  fclose(file);

  if(succ1 && succ2)
    return 1;
  else
    return 0;
}

static void datum_block(server_info* server, int conn_fd, datum_protocol_add_friend* block){
  int succ = 0;
  char *sender = server->client[conn_fd]->account.user;
  char *homie = block->message.body.homie;
  
  char account_path[100] = {0};
  strcpy(account_path, server->arg.path);
  strcat(account_path, "/accountlist");
  FILE* file = fopen(account_path, "r");
  if(!file){
    return;
  }
  char buffer[50] = {0};
  while(fgets(buffer, 50, file) != NULL){
    char *s = strtok(buffer, ",");
    if(strcmp(homie, s) == 0){
      succ = 1;
    }
  }
  fclose(file);

  datum_protocol_add_friend res;
  res.message.header.res.magic = DATUM_PROTOCOL_MAGIC_RES;
  res.message.header.res.op = DATUM_PROTOCOL_OP_BLOCK;
  res.message.header.res.datalen = 0;

  if(succ){
    char sender_path[100] = {0};
    char sender_list_path[100] = {0};
    strcpy(sender_path, server->arg.path);
    strcat(sender_path, "/user/");
    strcat(sender_path, sender);
    strcpy(sender_list_path, sender_path);
    strcat(sender_list_path, "/blocklist");


    fprintf(stderr, "%s block friend %s\n", sender, homie);

    FILE* fp;
    fp = fopen(sender_list_path, "a");
    fprintf(fp, "%s\n", homie);
    fclose(fp);

    res.message.header.res.status = DATUM_PROTOCOL_STATUS_OK;
  }
  else{
    res.message.header.res.status = DATUM_PROTOCOL_STATUS_FAIL;
  }
  send_message(conn_fd, &res, sizeof(res));
  return;
}

static int check_if_block(server_info* server, char* sender, char* receiver){
  char receiver_path[100] = {0};
  strcpy(receiver_path, server->arg.path);
  strcat(receiver_path, "/user/");
  strcat(receiver_path, receiver);
  strcat(receiver_path, "/blocklist");

  int succ = 0;
  FILE* file = fopen(receiver_path, "r");
  if(!file){
    return 0;
  }
  char buffer[50] = {0};
  while(fgets(buffer, 50, file) != NULL){
    char *s = strtok(buffer, "\n");
    if(strcmp(sender, s) == 0){
      succ = 1;
    }
  }
  fclose(file);
  return succ;
}

static void datum_unblock(server_info* server, int conn_fd, datum_protocol_add_friend* unblock){
  int succ = 0;
  char *sender = server->client[conn_fd]->account.user;
  char *homie = unblock->message.body.homie;
  
  char account_path[100] = {0};
  strcpy(account_path, server->arg.path);
  strcat(account_path, "/accountlist");
  FILE* file = fopen(account_path, "r");
  if(!file){
    return;
  }
  char buffer[50] = {0};
  while(fgets(buffer, 50, file) != NULL){
    char *s = strtok(buffer, ",");
    if(strcmp(homie, s) == 0){
      succ = 1;
    }
  }
  fclose(file);

  datum_protocol_add_friend res;
  res.message.header.res.magic = DATUM_PROTOCOL_MAGIC_RES;
  res.message.header.res.op = DATUM_PROTOCOL_OP_UNBLOCK;
  res.message.header.res.datalen = 0;

  if(succ){
    char sender_path[100] = {0};
    char sender_list_path[100] = {0};
    strcpy(sender_path, server->arg.path);
    strcat(sender_path, "/user/");
    strcat(sender_path, sender);
    strcpy(sender_list_path, sender_path);
    strcat(sender_list_path, "/blocklist");


    fprintf(stderr, "%s unblock friend %s\n", sender, homie);

    FILE* fp;
    FILE* fp2;
    char sender_new_list_path[100] = {0};
    strcpy(sender_new_list_path, sender_list_path);
    strcat(sender_new_list_path, "_new");
    fp = fopen(sender_list_path, "w+");
    fp2 = fopen(sender_new_list_path, "w");
    while(fgets(buffer, 50, fp) != NULL){
      char *s = strtok(buffer, "\n");
      if(strcmp(homie, s) != 0){
        fprintf(fp2, "%s\n", s); 
      }
    }
    fclose(fp);
    fclose(fp2);

    fp = fopen(sender_list_path, "w+");
    fp2 = fopen(sender_new_list_path, "r");

    while(fgets(buffer, 50, fp2) != NULL){
      char *s = strtok(buffer, "\n");
      fprintf(fp, "%s\n", s);
    }

    fclose(fp);
    fclose(fp2);

    res.message.header.res.status = DATUM_PROTOCOL_STATUS_OK;
  }
  else{
    res.message.header.res.status = DATUM_PROTOCOL_STATUS_FAIL;
  }
  send_message(conn_fd, &res, sizeof(res));
  return;
}