#pragma once

enum CMD { CMD_LOGIN, CMD_LOGOUT, CMD_ERROR };

struct DataHeader {
  short dataLength;
  short cmd;
};
struct Login {
  char userName[32];
  char password[32];
};
struct LoginResult {
  int result;
};
struct Logout {
  char userName[32];
};
struct LogoutResult {
  int result;
};
