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
#include <cstdlib>

// 客户端和服务端都定义相同的结构体
// 由于是在同一台机器由同一个工具编译的，因此数据大小一模一样
struct DataPackage {
  int age;
  char name[32];
};

// 加入WindowsSocket的静态库，此处采取第二种方法，为工程添加附加依赖项
// #pragma comment(lib, "ws2_32.lib")

int main() {
  // 启动WindowsSocket 2.x 的网络编程环境
  WORD ver = MAKEWORD(2, 2);
  WSADATA dat;
  if (WSAStartup(ver, &dat) != NO_ERROR) {
    printf("ERROR|%s:%d: WSAStartup failed with error: %u.\n", __FILE__,
           __LINE__, WSAGetLastError());
    return -1;
  } else {
    printf("SUCCESS|%s:%d: WSAStartup success.\n", __FILE__, __LINE__);
  }
  // 1. 建立一个socket
  SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (_sock == INVALID_SOCKET) {
    printf("ERROR|%s:%d: create socket failed with error: %u.\n", __FILE__,
           __LINE__, WSAGetLastError());
    WSACleanup();
    return -1;
  } else {
    printf("SUCCESS|%s:%d: create socket success.\n", __FILE__, __LINE__);
  }
  // 2. 连接服务器 connect
  sockaddr_in _sin = {};
  _sin.sin_family = AF_INET;
  _sin.sin_port = htons(16642);
  inet_pton(AF_INET, "127.0.0.1", &_sin.sin_addr);
  if (connect(_sock, (sockaddr*)&_sin, sizeof(sockaddr_in)) == SOCKET_ERROR) {
    printf("ERROR|%s:%d: connect failed with error: %u\n", __FILE__, __LINE__,
           WSAGetLastError());
    WSACleanup();
    return -1;
  } else {
    printf("SUCCESS|%s:%d: connect success.\n", __FILE__, __LINE__);
  }

  char cmdBuf[1024] = {};
  char a;
  int count = 0;
  while (true) {
    printf("Please enter a command%d:\n", count++);
    memset(cmdBuf, 0, 1024);
    // 3. 读取指令
    // 如果想要读取带有空格的字符串，此处不能使用scanf和scanf_s，它们读取到空格就会停止
    // scanf_s("%s", cmdBuf, 1023);
    scanf_s("%[^\n]", cmdBuf, 1023);
    scanf_s("%c", &a, 1);
    // gets_s(cmdBuf, 1023); // 最后一个字符位置留给'\0'
    if (0 == strcmp(cmdBuf, "exit")) {
      printf("Exiting...\n");
      break;
    }
    // 4. 将指令发送给服务器
    send(_sock, cmdBuf, strlen(cmdBuf) + 1, 0);
    // 5. 接收服务器信息recv
    char recvBuf[256] = {};
    if (recv(_sock, recvBuf, 256, 0) == 0) {
      printf("SUCCESS|%s:%d: recv failed with error: %u\n", __FILE__, __LINE__,
             WSAGetLastError());
      WSACleanup();
      return -1;
    }
    // 下面这种直接强转的做法非常low，且不安全，当前仅作为最基本的版本
    DataPackage* dp = (DataPackage*)recvBuf;
    printf("Get msg from server:\n%d, %s\n", dp->age, dp->name);
  }

  // 4. 关闭套接字closesocket
  closesocket(_sock);
  // 清理WindowsSocket的网络变成环境
  WSACleanup();
  system("pause");
  return 0;
}