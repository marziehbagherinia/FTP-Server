#ifndef ZEROFTP_CONST_H
#define ZEROFTP_CONST_H

const int TRANSFERING = 125;
const int OPENING_DATAPORT = 150;
const int HELP_COMMAND = 214;
const int SERVER_READY = 220;
const int GOODBYE = 221;
const int CLOSE_DATA_CONNECTION = 226;
const int ENTER_PASSIVE = 227;
const int LOGIN_SUCCESS = 230;
const int ACTION_DONE = 250;
const int PATHNAME_CREATED = 257;
const int RIGHT_USERNAME = 331;
const int NOT_LOGINED = 332;
const int CANNOT_OPEN_DATA_CONNECTION = 425;
const int INVALID_USER_PASS = 430;
const int NOT_IMPLEMENTED = 501;
const int BAD_SEQ = 503;
const int ACTION_FAILED = 550;
const int REQUESTED_ACTION_NOT_TAKEN = 553;

#define char_to_code(a, b, c) ((a - '0') * 100 + (b - '0') * 10 + (c - '0'))

#endif