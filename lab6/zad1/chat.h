#ifndef CHAT_H
#define CHAT_H

#define MAX_MSG_SIZE 2048

#define SERVER_KEY_PATH "/tmp"
#define SERVER_KEY_SEED 112
#define QUEUE_PERMISSIONS 0666


typedef enum ReqType {STOP_REQ = 1, /* Shutdown request */
                      LIST_REQ = 2, /* List request */
                      FRIENDS_REQ = 3, /* Setting friends list request */
                      INIT_REQ = 4, /* User registration request */
                      ECHO_REQ = 5, /* Echo request */
                      TOALL_REQ = 6, /* Echo to all */
                      TOFRIENDS_REQ = 7, /* Echo only to friend */
                      TOONE_REQ = 8, /* Echo to specific person */
}  ReqType;


typedef struct ReqMsg {
  long type;
  ReqType req_type;
  char arg1[MAX_MSG_SIZE];
  char arg2[MAX_MSG_SIZE];
} ReqMsg;


#endif /* CHAT_H */
