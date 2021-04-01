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
#include <stdio.h>

#include "Data.h"

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
    DataHeader header = {};
    char const *cmds[2] = {"login", "logout"};
    char const *cmd = nullptr;
    // 循环处理当前已连接的客户端的请求
    while (true) {
      // 清空接收数据缓冲区的数据
      memset(&header, 0, 1024);
      // 5. 接收客户端数据，如果接收到的数据长度为0，则退出循环
      if (recv(_cSock, (char *)&header, sizeof(DataHeader), 0) <= 0) {
        printf("Client has exit, task end.\n");
        break;
      }

      switch (header.cmd) {
        case CMD::CMD_LOGIN: {
          cmd = "login";
          Login login = {};
          recv(_cSock, (char *)&login, sizeof(Login), 0);
          // 忽略判断用户名密码
          // 给客户端返回登陆成功的信息
          LoginResult result = {1};
          send(_cSock, (const char *)&header, sizeof(DataHeader), 0);
          send(_cSock, (const char *)&result, sizeof(LoginResult), 0);
        } break;
        case CMD::CMD_LOGOUT: {
          cmd = "logout";
          Logout logout = {};
          recv(_cSock, (char *)&logout, sizeof(Logout), 0);
          LogoutResult result = {0};
          send(_cSock, (const char *)&header, sizeof(DataHeader), 0);
          send(_cSock, (const char *)&result, sizeof(LogoutResult), 0);

        } break;
        default: {
          cmd = "Unknow comand";
          header.cmd = CMD::CMD_ERROR;
          header.dataLength = 0;
          send(_cSock, (const char *)&header, sizeof(DataHeader), 0);
        } break;
      }
      printf("INFO|Get cmd: %s, data length: %d.\n", cmd, header.dataLength);
    }
  }
  // 8. 关闭socket closesocket
  closesocket(_sock);
  // 清理WindowsSocket的网络变成环境
  WSACleanup();
  return 0;
}