/*
 * WIN32_LEAN_AND_MEAN
 * 宏编译 预处理头 ，经常可以在stdafx.h文件中出现宏定义#define
 * WIN32_LEAN_AND_MEAN，
 * 具体含义就是当项目中包含#include<windows.h>时去除一些头文件的包含。一个重要的演示
 * 例子就是winsock2.h和windows.h之间有关于_WINSOCKAPI_的重复定义，如果定义了上面的宏编译
 * 就会避免出现重定义，不同的链接的错误
 */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>
#define ADDR_LEN int
#else
#include <arpa/inet.h>
#include <unistd.h>

#include <cstring>
#define SOCKET int
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR (-1)
#define ADDR_LEN socklen_t
#endif

#include <cstdio>
#include <unordered_set>
// #include <cstdlib>

#include "../Common/cross_platform_socket.h"
#include "../Common/data.h"
#include "../Common/log.h"

// 加入WindowsSocket的静态库，此处采取第二种方法，为工程添加附加依赖项
// #pragma comment(lib, "ws2_32.lib")

// 定义一个全局的动态数组，用于
std::unordered_set<SOCKET> readSockets;
std::unordered_set<SOCKET> writeSockets;
std::unordered_set<SOCKET> expSockets;

void doAccept(SOCKET listenSock) {
  sockaddr_in client = {};
  ADDR_LEN addrLen = sizeof(client);

  auto _cSock = accept(listenSock, (sockaddr*)&client, &addrLen);
  if (_cSock == INVALID_SOCKET) {
    logError(__FILE__, __LINE__, "accept");
  } else {
    logSuccess(__FILE__, __LINE__, "accept");
    // 此处不可以使用 inet_ntoa()，需使用 inet_ntop 替代
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client.sin_addr, ipStr, INET_ADDRSTRLEN);
    printf("New client access. socket = %llu, IP = %s, Port = %u.\n", _cSock,
           ipStr, client.sin_port);
    // v0.8: 将新增加的客户端群发给目前已有的所有客户端
    NewUserJoin newUser;
    newUser.sock = _cSock;
    for (SOCKET sock : readSockets) {
      // v1.0 linux下服务端不允许send函数给监听socket发送数据
      if (sock == listenSock) continue;
      send(sock, (const char* const) & newUser, sizeof(NewUserJoin), 0);
    }
    // 将新连接的通信socket添加到 read_sockets, write_sockets,
    // exp_sockets;
    readSockets.insert(_cSock);
    writeSockets.insert(_cSock);
    expSockets.insert(_cSock);
  }
}

// 规定当客户端退出时，返回 -1
// 此时需要在主处理流程中将通信socket从相应的集合中移除
int doRead(SOCKET _cSock) {
  char recvBuf[1024] = {};
  // 5. 接收客户端数据，如果接收到的数据长度为0，则退出函数
  if (recv(_cSock, recvBuf, sizeof(DataHeader), 0) <= 0) {
    printf("Client(%llu) has exit, task end.\n", _cSock);
    return -1;
  }
  DataHeader* header = (DataHeader*)recvBuf;
  switch (header->cmd) {
    case CMD::CMD_LOGIN: {
      // 由于Login中包含了DataHeader，第二次接收时，需要进行内存偏移
      recv(_cSock, recvBuf + sizeof(DataHeader),
           header->dataLength - sizeof(DataHeader), 0);
      Login* login = (Login*)recvBuf;
      printf(
          "INFO|Receive command login from socket(%llu), data length: "
          "%d, userName: %s.\n",
          _cSock, login->dataLength, login->userName);
      // 忽略判断用户名密码
      // 给客户端返回登陆成功的信息
      LoginResult result;
      send(_cSock, (const char* const) & result, sizeof(LoginResult), 0);
    } break;
    case CMD::CMD_LOGOUT: {
      recv(_cSock, recvBuf + sizeof(DataHeader),
           header->dataLength - sizeof(DataHeader), 0);
      Logout* logout = (Logout*)recvBuf;
      printf(
          "INFO|Receive command logout from socket(%llu), data length: "
          "%d, "
          "userName: %s.\n",
          _cSock, logout->dataLength, logout->userName);
      LogoutResult result;
      send(_cSock, (const char* const) & result, sizeof(LogoutResult), 0);
    } break;
    default: {
      DataHeader header = {0, CMD_ERROR};
      printf("INFO|Receive unknow command.\n");
      send(_cSock, (const char* const) & header, sizeof(DataHeader), 0);
    } break;
  }
  return 0;
}

int main() {
  // 封装成了一个函数，如果是 Windows 环境，则此函数的函数体非空
  initSocket();
  // 1. 建立一个socket(监听socket)
  SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (listenSock == INVALID_SOCKET) {
    logError(__FILE__, __LINE__, "socket");
    exitProcess(listenSock);
  } else {
    logSuccess(__FILE__, __LINE__, "socket");
  }
  // 设置端口重用
  int on = 1;
  setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on));
  // 2. 绑定接收客户端连接的端口 bind
  sockaddr_in _sin = {};
  _sin.sin_family = AF_INET;
  _sin.sin_port = htons(16642);
  // _sin.sin_addr.S_un.S_addr = INADDR_ANY;
  // VC++中使用了新的方法构造socket addr，这种写法会报错
  // _sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
  // VC++构造socket地址的新写法
  inet_pton(AF_INET, "127.0.0.1", &_sin.sin_addr);
  if (bind(listenSock, (sockaddr*)&_sin, sizeof(sockaddr_in)) == SOCKET_ERROR) {
    logError(__FILE__, __LINE__, "bind");
    exitProcess(listenSock);
  } else {
    logSuccess(__FILE__, __LINE__, "bind");
  }
  // 3. 监听网络端口 listen
  if (listen(listenSock, 128) == SOCKET_ERROR) {
    logError(__FILE__, __LINE__, "listen");
    exitProcess(listenSock);
  } else {
    logSuccess(__FILE__, __LINE__, "listen");
  }
  // 完成监听后，将_sock添加到 read_sockets, write_sockets, exp_sockets;
  readSockets.insert(listenSock);
  writeSockets.insert(listenSock);
  expSockets.insert(listenSock);

  char const* msg = nullptr;
  int recvLen = 0;
  char recvBuf[1024] = {};
  // 伯克利socket
  fd_set readSet = {};
  fd_set writeSet = {};
  fd_set expSet = {};
  SOCKET maxFd = 0;

  while (true) {
    // v0.8: 给select传递一个超时参数(最后一个参数)
    timeval t = {1, 0};
    SOCKET maxFd = initFdSets(&readSet, &readSockets, &writeSet, &writeSockets,
                              &expSet, &expSockets);
    // nfds是一个整数值，三个fd_set中所有描述符最大值+1，在Windows中这个参数可以写0
    // select 执行出错，即退出循环。
    auto selectRet = select(maxFd + 1, &readSet, nullptr, nullptr, &t);
    if (selectRet < 0) {
      logError(__FILE__, __LINE__, "select");
      break;
    }
    // selectRet == 0 时直接处理其它业务
    if (selectRet > 0) {
      // 检测到_sock可读，说明有新的客户端连接
      if (FD_ISSET(listenSock, &readSet)) {
        doAccept(listenSock);
      }
      // 处理可读的通信socket
      for (auto var : readSockets) {
        if (var == listenSock) continue;
        // 如果当前客户端退出，doRet == -1
        // 需要将对应的socket其从相应的集合中移除
        int doRet = doRead(var);
        if (doRet == -1) {
          // 通信socket也需要关闭，在之前的版本中
          // 都没有关闭，这是不对的
          // 一般教程或者书籍中将其称之为客户端socket
          // 不过我觉得叫作通信socket更加合适
          closeSocket(var);
          readSockets.erase(var);
          writeSockets.erase(var);
          expSockets.erase(var);
        }
      }
    }
    printf("INFO|Handle other business...\n");
  }
  // 关闭所有的socket
  // 一般来说是走不到这一步的，但是为了容错，这里添加上
  for (auto sock : readSockets) {
    closeSocket(sock);
  }
  // listenSock也在上面一步中处理了，此处注释掉相关代码
  // closesocket(listenSock);
  // 清理WindowsSocket的网络变成环境
  cleanSocket();
  return 0;
}
