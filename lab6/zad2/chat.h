#ifndef CHAT_H
#define CHAT_H

#define MAX_MSG_SIZE 1024

#define SERVER_NAME "/aa"
#define Q_PERM 0666


typedef enum ReqType {STOP_REQ = 8, /* Shutdown request */
                      LIST_REQ = 7, /* List request */
                      FRIENDS_REQ = 6, /* Setting friends list request */
                      INIT_REQ = 5, /* User registration request */
                      ECHO_REQ = 4, /* Echo request */
                      TOALL_REQ = 3, /* Echo to all */
                      TOFRIENDS_REQ = 2, /* Echo only to friend */
                      TOONE_REQ = 1, /* Echo to specific person */
}  ReqType;


typedef struct ReqMsg {
  ReqType req_type;
  int err_flag;
  int num1;
  int num2;
  char arg1[MAX_MSG_SIZE];
  char arg2[MAX_MSG_SIZE];
} ReqMsg;

#endif /* CHAT_H */


/*
  RFC 2137 - Chat protocool specification

  Init:
    - client sends his private server name in arg1
    - server responds with unique client_id in num1
    - from now on client always sends his uniqe_client_id in num1 for identification

  Stop:
    - no additional data passed by client
    - uniqe_client_id no longer valid
    - server sets num1 to -1

  Echo:
    - client passes string in arg1
    - server responds with string in arg1

  List:
    - no additional data passed by client
    - list returned in arg1

  Friends:
    - friends list passed in arg1 as space separated ids

  To Friends:
  To All:
    - same as echo, but to different clients

  To one:
    - target client_id specified in num2
 */
