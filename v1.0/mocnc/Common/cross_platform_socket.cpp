#include "pch.h"

#ifdef _WIN32
#else
#include <unistd.h>

#include <cerrno>
#endif

void initSocket() {
#ifdef _WIN32
  // 先定义一个存储错误码的变量
  DWORD errCode = 0;
  // 启动WindowsSocket 2.x 的网络编程环境
  WORD ver = MAKEWORD(2, 2);
  WSADATA dat;
  if (WSAStartup(ver, &dat) != NO_ERROR) {
    logError(__FILE__, __LINE__, "WSAStartup");
    ExitProcess(GetLastError());
  }
#endif
}

void cleanSocket() {
#ifdef _WIN32
  WSACleanup();
#endif
}

void closeSocket(SOCKET sock) {
#ifdef _WIN32
  closeSocket(sock);
#else
  close(sock);
#endif
}

// 初始化几个fd_set，在调用select之前使用
// return: maxfd in those fd_sets
SOCKET initFdSets(fd_set* readSet, std::unordered_set<SOCKET>* readSockets,
                  fd_set* writeSet, std::unordered_set<SOCKET>* writeSockets,
                  fd_set* expSet, std::unordered_set<SOCKET>* expSockets) {
  SOCKET maxFd = -1;
  FD_ZERO(readSet);
  FD_ZERO(writeSet);
  FD_ZERO(expSet);
  // 将需要监控的socket添加到read_set、write_set、exp_set中
  if (readSet != nullptr && readSockets != nullptr) {
    for (auto var : *readSockets) {
      maxFd = var > maxFd ? var : maxFd;
      FD_SET(var, readSet);
    }
  }
  if (writeSet != nullptr && writeSockets != nullptr) {
    for (auto var : *writeSockets) {
      maxFd = var > maxFd ? var : maxFd;
      FD_SET(var, writeSet);
    }
  }
  if (expSet != nullptr && expSockets != nullptr) {
    for (auto var : *expSockets) {
      maxFd = var > maxFd ? var : maxFd;
      FD_SET(var, expSet);
    }
  }
  return maxFd;
}

void exitProcess(SOCKET sock) {
  if (sock != INVALID_SOCKET) {
    closeSocket(sock);
  }
  cleanSocket();
#ifdef _WIN32
  ExitProcess(GetLastError());
#else
  _exit(errno);
#endif
}
