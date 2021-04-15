/*
 * WIN32_LEAN_AND_MEAN
 * 宏编译 预处理头 ，经常可以在stdafx.h文件中出现宏定义#define
 * WIN32_LEAN_AND_MEAN，
 * 具体含义就是当项目中包含#include<windows.h>时去除一些头文件的包含。一个重要的演示
 * 例子就是winsock2.h和windows.h之间有关于_WINSOCKAPI_的重复定义，如果定义了上面的宏编译
 * 就会避免出现重定义，不同的链接的错误
 */
// #pragma warning(disable : 4996)
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#define SOCKET int
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR (-1)
#endif

#include <cstdio>
#include <cstdlib>
#include <thread>

#include "../Common/Data.h"

#define SERVER_IP "192.168.1.2"

// 加入WindowsSocket的静态库，此处采取第二种方法，为工程添加附加依赖项
// #pragma comment(lib, "ws2_32.lib")

// 通信socket的读取函数，专门用来读取从服务端发回来的消息
int doRead(SOCKET _cSock)
{
  char recvBuf[1024] = {};
  // 5. 接收服务器数据，如果接收到的数据长度为0，则退出函数
  if (recv(_cSock, recvBuf, sizeof(DataHeader), 0) <= 0)
  {
#ifdef WIN32
    printf("ERROR|recv error: %u.\n", GetLastError());
#else
    printf("ERROR|recv error: %d.\n", errno);
#endif
    return -1;
  }
  DataHeader *header = (DataHeader *)recvBuf;
  switch (header->cmd)
  {
  case CMD::CMD_LOGIN_RESULT:
  {
    // 由于Login中包含了DataHeader，第二次接收时，需要进行内存偏移
    recv(_cSock, recvBuf + sizeof(DataHeader),
         header->dataLength - sizeof(DataHeader), 0);
    LoginResult *result = (LoginResult *)recvBuf;
    printf(
        "INFO|Receive login result from server, data length: %d, result "
        "code: %d.\n",
        result->dataLength, result->result);
  }
  break;
  case CMD::CMD_LOGOUT_RESULT:
  {
    recv(_cSock, recvBuf + sizeof(DataHeader),
         header->dataLength - sizeof(DataHeader), 0);
    LogoutResult *result = (LogoutResult *)recvBuf;
    printf(
        "INFO|Receive logout result from server, data length: %d, result "
        "code: "
        "%d\n",
        result->dataLength, result->result);
  }
  break;
  case CMD::CMD_NEW_USER_JOIN:
  {
    recv(_cSock, recvBuf + sizeof(DataHeader),
         header->dataLength - sizeof(DataHeader), 0);
    NewUserJoin *newJoin = (NewUserJoin *)recvBuf;
    printf(
        "INFO|Receive new user join from server, data length: %d, new user: "
        "%d.\n",
        newJoin->dataLength, newJoin->sock);
  }
  break;
  default:
  {
    DataHeader header = {0, CMD_ERROR};
    printf("INFO|Receive unknow command.\n");
  }
  break;
  }
  return 0;
}

bool g_Run = true;
void readCmd(SOCKET _sock)
{
  char cmdBuf[1024] = {};
  int count = 0;
  while (true)
  {
    printf("Please enter a command%d:\n", count++);
    memset(cmdBuf, 0, 1024);
// 3. 读取指令
// 如果想要读取带有空格的字符串，此处不能直接使用scanf和scanf_s，它们读取到空格就会停止
#ifdef WIN32
    // scanf_s("%s", cmdBuf, 1023);
    scanf_s("%[^\n]%*c", cmdBuf, 1023);
    // gets_s(cmdBuf, 1023); // 最后一个字符位置留给'\0'
#else
    scanf("%[^\n]%*c", cmdBuf);
#endif
    DataHeader retHeader = {};
    if (strcmp(cmdBuf, "exit") == 0)
    {
      printf("Exiting...\n");
      g_Run = false;
      return;
    }
    else if (strcmp(cmdBuf, "login") == 0)
    {
      Login login;
#ifdef WIN32
      strcpy_s(login.userName, "xiaoyao");
      strcpy_s(login.password, "123456");
#else
      strcpy(login.userName, "xiaoyao");
      strcpy(login.password, "123456");
#endif
      send(_sock, (const char *)&login, sizeof(Login), 0);
    }
    else if (strcmp(cmdBuf, "logout") == 0)
    {
      Logout logout;
#ifdef WIN32
      strcpy_s(logout.userName, "xiaoyao");
#else
      strcpy(logout.userName, "xiaoyao");
#endif
      DataHeader dataHeader = {sizeof(Logout), CMD_LOGOUT};
      send(_sock, (const char *)&logout, sizeof(Logout), 0);
    }
    else
    {
      printf("Unsupported command, please retry.\n");
      continue;
    }
  }
}

int main()
{
#ifdef WIN32
  // 启动WindowsSocket 2.x 的网络编程环境
  WORD ver = MAKEWORD(2, 2);
  WSADATA dat;
  if (WSAStartup(ver, &dat) != NO_ERROR)
  {
    printf("ERROR|%s:%d: WSAStartup failed with error: %u.\n", __FILE__,
           __LINE__, WSAGetLastError());
    return -1;
  }
  else
  {
    printf("SUCCESS|%s:%d: WSAStartup success.\n", __FILE__, __LINE__);
  }
#endif
  // 1. 建立一个socket
  SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (_sock == INVALID_SOCKET)
  {
#ifdef WIN32
    printf("ERROR|%s:%d: create socket failed with error: %u.\n", __FILE__,
           __LINE__, WSAGetLastError());
    WSACleanup();
#else
    printf("ERROR|%s:%d: create socket failed with error: %d.\n", __FILE__, __LINE__, errno);
#endif
    return -1;
  }
  else
  {
    printf("SUCCESS|%s:%d: create socket success.\n", __FILE__, __LINE__);
  }
  // 2. 连接服务器 connect
  sockaddr_in _sin = {};
  _sin.sin_family = AF_INET;
  _sin.sin_port = htons(16642);
  inet_pton(AF_INET, SERVER_IP, &_sin.sin_addr);
  if (connect(_sock, (sockaddr *)&_sin, sizeof(sockaddr_in)) == SOCKET_ERROR)
  {
#ifdef WIN32
    printf("ERROR|%s:%d: connect failed with error: %u\n", __FILE__, __LINE__,
           WSAGetLastError());
    WSACleanup();
#else
    printf("ERROR|%s:%d: connect failed with error: %d\n", __FILE__, __LINE__, errno);
#endif
    return -1;
  }
  else
  {
    printf("SUCCESS|%s:%d: connect success.\n", __FILE__, __LINE__);
  }

  // 启动线程
  std::thread cmdThread(readCmd, _sock);
  // 将子线程与主线程分离
  cmdThread.detach();

  fd_set readSet = {};
  while (g_Run)
  {
    FD_ZERO(&readSet);
    FD_SET(_sock, &readSet);
    // linux下的select返回后会将超时参数置空，因此需要在每次select之前都需要重新设置超时参数
    timeval t = {1, 0};
    // 考虑到只有一个需要监听的套接字，此处就不清空以及重新设置readSet了
    // linux 下的select第一个参数必须设置为三个描述符集中的 fdmax + 1
    int ret = select(_sock + 1, &readSet, nullptr, nullptr, &t);
    if (ret < 0)
    {
      // 考虑到接收新客户端连接的消息是非必需功能，因此此处出错不中断程序，而是继续向下执行。
#ifdef WIN32
      printf("ERROR|%s:%d: select error: %u.\n", __FILE__, __LINE__,
             GetLastError());
#else
      printf("ERROR|%s:%d: select error: %d.\n", __FILE__, __LINE__, errno);
#endif
    }
    else if (ret > 0)
    {
      printf("select returned a value greater than zero.\n");
      if (FD_ISSET(_sock, &readSet))
      {
        // 读取消息放到这里
        // doRead 返回-1说明网络连接出了问题或者服务器关闭了，此时直接退出
        if (doRead(_sock) == -1)
        {
          break;
        }
      }
    }
    else
    {
      printf("select returned 0.\n");
    }
    // printf("INFO|Handle other business...\n");
  }

// 4. 关闭套接字closesocket
#ifdef WIN32
  closesocket(_sock);
  // 清理WindowsSocket的网络变成环境
  WSACleanup();
#else
  close(_sock);
#endif
  system("pause");
  return 0;
}
