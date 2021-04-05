/*
 * WIN32_LEAN_AND_MEAN
 * ����� Ԥ����ͷ ������������stdafx.h�ļ��г��ֺ궨��#define
 * WIN32_LEAN_AND_MEAN��
 * ���庬����ǵ���Ŀ�а���#include<windows.h>ʱȥ��һЩͷ�ļ��İ�����һ����Ҫ����ʾ
 * ���Ӿ���winsock2.h��windows.h֮���й���_WINSOCKAPI_���ظ����壬�������������ĺ����
 * �ͻ��������ض��壬��ͬ�����ӵĴ���
 */
// #pragma warning(disable : 4996)

#define WIN32_LEAN_AND_MEAN
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>

#include <cstdio>
#include <cstdlib>
#include <thread>

#include "../Common/Data.h"

// ����WindowsSocket�ľ�̬�⣬�˴���ȡ�ڶ��ַ�����Ϊ������Ӹ���������
// #pragma comment(lib, "ws2_32.lib")

// ͨ��socket�Ķ�ȡ������ר��������ȡ�ӷ���˷���������Ϣ
int doRead(SOCKET _cSock) {
  char recvBuf[1024] = {};
  // 5. ���շ��������ݣ�������յ������ݳ���Ϊ0�����˳�����
  if (recv(_cSock, recvBuf, sizeof(DataHeader), 0) <= 0) {
    printf("ERROR|recv error: %u.\n", GetLastError());
    return -1;
  }
  DataHeader *header = (DataHeader *)recvBuf;
  switch (header->cmd) {
    case CMD::CMD_LOGIN_RESULT: {
      // ����Login�а�����DataHeader���ڶ��ν���ʱ����Ҫ�����ڴ�ƫ��
      recv(_cSock, recvBuf + sizeof(DataHeader),
           header->dataLength - sizeof(DataHeader), 0);
      LoginResult *result = (LoginResult *)recvBuf;
      printf(
          "INFO|Receive login result from server, data length: %d, result "
          "code: %d.\n",
          result->dataLength, result->result);
    } break;
    case CMD::CMD_LOGOUT_RESULT: {
      recv(_cSock, recvBuf + sizeof(DataHeader),
           header->dataLength - sizeof(DataHeader), 0);
      LogoutResult *result = (LogoutResult *)recvBuf;
      printf(
          "INFO|Receive logout result from server, data length: %d, result "
          "code: "
          "%d\n",
          result->dataLength, result->result);
    } break;
    case CMD::CMD_NEW_USER_JOIN: {
      recv(_cSock, recvBuf + sizeof(DataHeader),
           header->dataLength - sizeof(DataHeader), 0);
      NewUserJoin *newJoin = (NewUserJoin *)recvBuf;
      printf(
          "INFO|Receive new user join from server, data length: %d, new user: "
          "%d.\n",
          newJoin->dataLength, newJoin->sock);
    } break;
    default: {
      DataHeader header = {0, CMD_ERROR};
      printf("INFO|Receive unknow command.\n");
    } break;
  }
  return 0;
}

bool g_Run = true;
void readCmd(SOCKET _sock) {
  char cmdBuf[1024] = {};
  int count = 0;
  while (true) {
    printf("Please enter a command%d:\n", count++);
    memset(cmdBuf, 0, 1024);
    // 3. ��ȡָ��
    // �����Ҫ��ȡ���пո���ַ������˴�����ֱ��ʹ��scanf��scanf_s�����Ƕ�ȡ���ո�ͻ�ֹͣ
    // scanf_s("%s", cmdBuf, 1023);
    scanf_s("%[^\n]%*c", cmdBuf, 1023);
    // gets_s(cmdBuf, 1023); // ���һ���ַ�λ������'\0'
    DataHeader retHeader = {};
    if (strcmp(cmdBuf, "exit") == 0) {
      printf("Exiting...\n");
      g_Run = false;
      return;
    } else if (strcmp(cmdBuf, "login") == 0) {
      Login login;
      strcpy_s(login.userName, "xiaoyao");
      strcpy_s(login.password, "123456");
      send(_sock, (const char *)&login, sizeof(Login), 0);
    } else if (strcmp(cmdBuf, "logout") == 0) {
      Logout logout;
      strcpy_s(logout.userName, "xiaoyao");
      DataHeader dataHeader = {sizeof(Logout), CMD_LOGOUT};
      send(_sock, (const char *)&logout, sizeof(Logout), 0);
    } else {
      printf("Unsupported command, please retry.\n");
      continue;
    }
  }
}

int main() {
  // ����WindowsSocket 2.x �������̻���
  WORD ver = MAKEWORD(2, 2);
  WSADATA dat;
  if (WSAStartup(ver, &dat) != NO_ERROR) {
    printf("ERROR|%s:%d: WSAStartup failed with error: %u.\n", __FILE__,
           __LINE__, WSAGetLastError());
    return -1;
  } else {
    printf("SUCCESS|%s:%d: WSAStartup success.\n", __FILE__, __LINE__);
  }
  // 1. ����һ��socket
  SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (_sock == INVALID_SOCKET) {
    printf("ERROR|%s:%d: create socket failed with error: %u.\n", __FILE__,
           __LINE__, WSAGetLastError());
    WSACleanup();
    return -1;
  } else {
    printf("SUCCESS|%s:%d: create socket success.\n", __FILE__, __LINE__);
  }
  // 2. ���ӷ����� connect
  sockaddr_in _sin = {};
  _sin.sin_family = AF_INET;
  _sin.sin_port = htons(16642);
  inet_pton(AF_INET, "127.0.0.1", &_sin.sin_addr);
  if (connect(_sock, (sockaddr *)&_sin, sizeof(sockaddr_in)) == SOCKET_ERROR) {
    printf("ERROR|%s:%d: connect failed with error: %u\n", __FILE__, __LINE__,
           WSAGetLastError());
    WSACleanup();
    return -1;
  } else {
    printf("SUCCESS|%s:%d: connect success.\n", __FILE__, __LINE__);
  }

  // �����߳�
  std::thread cmdThread(readCmd, _sock);
  // �����߳������̷߳���
  cmdThread.detach();

  fd_set readSet = {};
  timeval t = {1, 0};
  while (g_Run) {
    FD_ZERO(&readSet);
    FD_SET(_sock, &readSet);
    // ���ǵ�ֻ��һ����Ҫ�������׽��֣��˴��Ͳ�����Լ���������readSet��
    int ret = select(_sock, &readSet, nullptr, nullptr, &t);
    if (ret < 0) {
      // ���ǵ������¿ͻ������ӵ���Ϣ�ǷǱ��蹦�ܣ���˴˴������жϳ��򣬶��Ǽ�������ִ�С�
      printf("ERROR|%s:%d: select error: %u.\n", __FILE__, __LINE__,
             GetLastError());
    } else if (ret > 0) {
      if (FD_ISSET(_sock, &readSet)) {
        // ��ȡ��Ϣ�ŵ�����
        // doRead ����-1˵���������ӳ���������߷������ر��ˣ���ʱֱ���˳�
        if (doRead(_sock) == -1) {
          break;
        }
      }
    }
    printf("INFO|Handle other business...\n");
  }

  // 4. �ر��׽���closesocket
  closesocket(_sock);
  // ����WindowsSocket�������ɻ���
  WSACleanup();
  system("pause");
  return 0;
}