#include "common.h"
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

int recv_message(int conn_fd, void* message, size_t len) {
  if (len == 0) {
    return 0;
  }
  return recv(conn_fd, message, len, MSG_WAITALL);
}

//used to receive complete header
int complete_message_with_header(
  int conn_fd, datum_protocol_header* header, void* result) {
  memcpy(result, header->bytes, sizeof(datum_protocol_header));
  return recv(conn_fd,
              result + sizeof(datum_protocol_header),
              header->req.datalen,
              MSG_WAITALL) == header->req.datalen;
}

int send_message(int conn_fd, void* message, size_t len) {
  if (len == 0) {
    return 0;
  }
  return send(conn_fd, message, len, 0) == len;
}

