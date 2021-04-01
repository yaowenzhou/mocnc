/*
 * WIN32_LEAN_AND_MEAN
 * 宏编译 预处理头 ，经常可以在stdafx.h文件中出现宏定义#define
 * WIN32_LEAN_AND_MEAN，
 * 具体含义就是当项目中包含#include<windows.h>时去除一些头文件的包含。一个重要的演示
 * 例子就是winsock2.h和windows.h之间有关于_WINSOCKAPI_的重复定义，如果定义了上面的宏编译
 * 就会避免出现重定义，不同的链接的错误
 */
#define WIN32_LEAN_AND_MEAN
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>

#include <cstdio>

#include "../Common/Data.h"

// 加入WindowsSocket的静态库，此处采取第二种方法，为工程添加附加依赖项
// #pragma comment(lib, "ws2_32.lib")

int main() {
  // 启动WindowsSocket 2.x 的网络编程环境
  WORD ver = MAKEWORD(2, 2);
  WSADATA dat;
  if (WSAStartup(ver, &dat) != NO_ERROR) {
    printf("ERROR|%s:%d: WSAStartup error.\n", __FILE__, __LINE__);
    return 1;
  }
  // 1. 建立一个socket
  SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  // 设置端口重用
  int on = 1;
  setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
  // 2. 绑定接收客户端连接的端口 bind
  sockaddr_in _sin = {};
  _sin.sin_family = AF_INET;
  _sin.sin_port = htons(16642);
  // _sin.sin_addr.S_un.S_addr = INADDR_ANY;
  // VC++中使用了新的方法构造socket addr，这种写法会报错
  // _sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
  // VC++构造socket地址的新写法
  inet_pton(AF_INET, "127.0.0.1", &_sin.sin_addr);
  if (bind(_sock, (sockaddr *)&_sin, sizeof(sockaddr_in)) == SOCKET_ERROR) {
    printf("ERROR|%s:%d: bind fail, errno is: %d.\n", __FILE__, __LINE__,
           errno);
    WSACleanup();
    ExitProcess(errno);
  } else {
    printf("SUCCESS|%s:%d: bind success.\n", __FILE__, __LINE__);
  }
  // 3. 监听网络端口 listen
  if (listen(_sock, 128) == SOCKET_ERROR) {
    printf("ERROR|%s:%d: listen fail, errno is: %d.\n", __FILE__, __LINE__,
           errno);
    WSACleanup();
    ExitProcess(errno);
  } else {
    printf("SUCCESS|%s:%d: listen success.\n", __FILE__, __LINE__);
  }
  // 4. 等待接收客户端连接accept
  sockaddr_in client = {};
  int nAddrLen = sizeof(client);
  SOCKET _cSock = INVALID_SOCKET;
  char const *msg = nullptr;
  int recvLen = 0;
  char recvBuf[1024] = {};

  while (true) {
    _cSock = accept(_sock, (sockaddr *)&client, &nAddrLen);
    if (_cSock == INVALID_SOCKET) {
      printf(
          "ERROR|%s:%d: accept error, get a invalid client socket, errno is: "
          "%d.",
          __FILE__, __LINE__, errno);
      WSACleanup();
      ExitProcess(errno);
    }
    // 此处不可以使用 inet_ntoa()，需使用 inet_ntop 替代
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client.sin_addr, ipStr, INET_ADDRSTRLEN);
    printf("New client access. IP = %s\n", ipStr);
    // 循环处理当前已连接的客户端的请求
    while (true) {
      // 先清空接收缓冲区
      memset(recvBuf, 0, 1024);
      // 5. 接收客户端数据，如果接收到的数据长度为0，则退出循环
      recvLen = recv(_cSock, recvBuf, sizeof(DataHeader), 0);
      if (recvLen <= 0) {
        printf("Client has exit, task end.\n");
        break;
      }
      DataHeader *header = (DataHeader *)recvBuf;
      switch (header->cmd) {
        case CMD::CMD_LOGIN: {
          // 由于Login中包含了DataHeader，第二次接收时，需要进行内存偏移
          recv(_cSock, recvBuf + sizeof(DataHeader),
               header->dataLength - sizeof(DataHeader), 0);
          Login *login = (Login *)recvBuf;
          printf(
              "INFO|Receive command: login, data length: %d, userName: %s.\n",
              login->dataLength, login->userName);
          // 忽略判断用户名密码
          // 给客户端返回登陆成功的信息
          LoginResult result;
          send(_cSock, (const char *)&result, sizeof(LoginResult), 0);
        } break;
        case CMD::CMD_LOGOUT: {
          recv(_cSock, recvBuf + sizeof(DataHeader),
               header->dataLength - sizeof(DataHeader), 0);
          Logout *logout = (Logout *)recvBuf;
          printf(
              "INFO|Receive command: logout, data length: %d, userName: %s.\n",
              logout->dataLength, logout->userName);
          LogoutResult result;
          send(_cSock, (const char *)&result, sizeof(LogoutResult), 0);

        } break;
        default: {
          DataHeader header = {0, CMD_ERROR};
          printf("INFO|Receive unknow command.\n");
          send(_cSock, (const char *)&header, sizeof(DataHeader), 0);
        } break;
      }
    }
  }
  // 8. 关闭socket closesocket
  closesocket(_sock);
  // 清理WindowsSocket的网络变成环境
  WSACleanup();
  return 0;
}