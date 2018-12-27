#include "server.h"

//where the server starts
int main(int argc, char** argv) {
  server_info* box = 0;
  server_init(&box, argc, argv);
  if (box) {
    server_run(box);
  }
  server_destroy(&box);
  return 0;
}