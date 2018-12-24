#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
/*
 * protocol
 */

#define USER_LEN_MAX 30
#define PASSWD_LEN_MAX 30
typedef enum {
  LINE_PROTOCOL_MAGIC_REQ = 0x90,
  LINE_PROTOCOL_MAGIC_RES = 0x91,
} line_protocol_magic;

typedef enum {
  LINE_PROTOCOL_OP_LOGIN = 0x00,
  LINE_PROTOCOL_OP_SEND_MESSAGE = 0x01,
  LINE_PROTOCOL_OP_SEND_FILE = 0x02,
  LINE_PROTOCOL_OP_REQ_LOG = 0x03,
} line_protocol_op;

typedef enum {
  LINE_PROTOCOL_STATUS_OK = 0x00,
  LINE_PROTOCOL_STATUS_FAIL = 0x01,
  LINE_PROTOCOL_STATUS_MORE = 0x02,
} line_protocol_status;

//common header
typedef union {
  struct {
    uint8_t magic;
    uint8_t op;
    uint8_t status;
    uint16_t sender_id;
    uint16_t reciever_id;
    //datalen = the length of complete header - the length of common header
    uint32_t datalen;
  } req;
  struct {
    uint8_t magic;
    uint8_t op;
    uint8_t status;
    uint16_t sender_id;
    uint16_t receiver_id;
    //datalen = the length of complete header - the length of common header
    uint32_t datalen;
  } res;
  uint8_t bytes[11];
} line_protocol_header;

//below are three types of complete header
//header used to login
typedef union {
  struct {
    line_protocol_header header;
    struct {
      uint8_t user[USER_LEN_MAX];
      uint8_t passwd[PASSWD_LEN_MAX];
    } body;
  } message;
  uint8_t bytes[sizeof(line_protocol_header) + USER_LEN_MAX * 2];
} line_protocol_login;


//header used to send data of file
typedef union {
  struct {
    line_protocol_header header;
    struct {
      //char path[400];
      uint64_t datalen;
    } body;
  } message;
  uint8_t bytes[sizeof(line_protocol_header) + 8];
} line_protocol_file;

//header used to send message
typedef union {
    struct {
      line_protocol_header header;
      struct {
        uint64_t datalen;
      } body;
    } message;
    uint8_t bytes[sizeof(line_protocol_header) + 8];
} line_protocol_message;

/*
 * utility
 */

// recv message
int recv_message(int conn_fd, void* message, size_t len);

// copy header and recv remain part of message
int complete_message_with_header(
  int conn_fd, csiebox_protocol_header* header, void* result);

// send message
int send_message(int conn_fd, void* message, size_t len);


