#include "common.h"

#include <limits.h>


typedef struct {
  char user[USER_LEN_MAX];
  char passwd[PASSWD_LEN_MAX];
} account_info;

typedef struct {
  account_info account;
  int conn_fd;
  int req_fd;
} client_info;

typedef struct {
  struct {
    char path[PATH_MAX];
    char port[6];
  } arg;
  int listen_fd;
  client_info** client;
} server_info;

void server_init(
  server_info** server, int argc, char** argv);
int server_run(server_info* server);
void server_destroy(server_info** server);