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
  DATUM_PROTOCOL_MAGIC_REQ = 0x90,
  DATUM_PROTOCOL_MAGIC_RES = 0x91,
} DATUM_protocol_magic;

typedef enum {
  DATUM_PROTOCOL_OP_LOGIN = 0x00,
  DATUM_PROTOCOL_OP_SEND_MESSAGE = 0x01,
  DATUM_PROTOCOL_OP_SEND_FILE = 0x02,
  DATUM_PROTOCOL_OP_REQ_LOG = 0x03,
} DATUM_protocol_op;

typedef enum {
  DATUM_PROTOCOL_STATUS_OK = 0x00,
  DATUM_PROTOCOL_STATUS_FAIL = 0x01,
  DATUM_PROTOCOL_STATUS_MORE = 0x02,
} DATUM_protocol_status;

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
} datum_protocol_header;

//below are three types of complete header
//header used to login
typedef union {
  struct {
    datum_protocol_header header;
    struct {
      char user[USER_LEN_MAX];
      char passwd[PASSWD_LEN_MAX];
    } body;
  } message;
  uint8_t bytes[sizeof(datum_protocol_header) + USER_LEN_MAX * 2];
} datum_protocol_login;


//header used to send data of file
typedef union {
  struct {
    datum_protocol_header header;
    struct {
      //char path[400];
      uint64_t datalen;
    } body;
  } message;
  uint8_t bytes[sizeof(datum_protocol_header) + 8];
} datum_protocol_file;

//header used to send message
typedef union {
    struct {
      datum_protocol_header header;
      struct {
        uint64_t datalen;
      } body;
    } message;
    uint8_t bytes[sizeof(datum_protocol_header) + 8];
} datum_protocol_message;

/*
 * utility
 */

// recv message
int recv_message(int conn_fd, void* message, size_t len);

// copy header and recv remain part of message
int complete_message_with_header(
  int conn_fd, datum_protocol_header* header, void* result);

// send message
int send_message(int conn_fd, void* message, size_t len);
