#ifndef CROSS_PLATFORM_SOCK
#define CROSS_PLATFORM_SOCK
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>
#define ADDR_LEN int
#else
#include <arpa/inet.h>
#include <unistd.h>
// #include <cerrno>
// #include <cstring>
#define SOCKET int
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR (-1)
#define ADDR_LEN socklen_t
#endif
#include <unordered_set>
void initSocket();
void cleanSocket();
void closeSocket(SOCKET sock);
SOCKET initFdSets(fd_set* readSet,
                  std::unordered_set<SOCKET>* readSockets,
                  fd_set* writeSet,
                  std::unordered_set<SOCKET>* writeSockets,
                  fd_set* expSet,
                  std::unordered_set<SOCKET>* expSockets);
void exitProcess(SOCKET sock);
#endif