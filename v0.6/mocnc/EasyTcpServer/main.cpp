/*
 * WIN32_LEAN_AND_MEAN
 * ����� Ԥ����ͷ ������������stdafx.h�ļ��г��ֺ궨��#define
 * WIN32_LEAN_AND_MEAN��
 * ���庬����ǵ���Ŀ�а���#include<windows.h>ʱȥ��һЩͷ�ļ��İ�����һ����Ҫ����ʾ
 * ���Ӿ���winsock2.h��windows.h֮���й���_WINSOCKAPI_���ظ����壬�������������ĺ����
 * �ͻ��������ض��壬��ͬ�����ӵĴ���
 */
#define WIN32_LEAN_AND_MEAN
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>

#include <cstdio>

#include "../Common/Data.h"

// ����WindowsSocket�ľ�̬�⣬�˴���ȡ�ڶ��ַ�����Ϊ������Ӹ���������
// #pragma comment(lib, "ws2_32.lib")

int main() {
  // ����WindowsSocket 2.x �������̻���
  WORD ver = MAKEWORD(2, 2);
  WSADATA dat;
  if (WSAStartup(ver, &dat) != NO_ERROR) {
    printf("ERROR|%s:%d: WSAStartup error.\n", __FILE__, __LINE__);
    return 1;
  }
  // 1. ����һ��socket
  SOCKET _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  // ���ö˿�����
  int on = 1;
  setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
  // 2. �󶨽��տͻ������ӵĶ˿� bind
  sockaddr_in _sin = {};
  _sin.sin_family = AF_INET;
  _sin.sin_port = htons(16642);
  // _sin.sin_addr.S_un.S_addr = INADDR_ANY;
  // VC++��ʹ�����µķ�������socket addr������д���ᱨ��
  // _sin.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
  // VC++����socket��ַ����д��
  inet_pton(AF_INET, "127.0.0.1", &_sin.sin_addr);
  if (bind(_sock, (sockaddr *)&_sin, sizeof(sockaddr_in)) == SOCKET_ERROR) {
    printf("ERROR|%s:%d: bind fail, errno is: %d.\n", __FILE__, __LINE__,
           errno);
    WSACleanup();
    ExitProcess(errno);
  } else {
    printf("SUCCESS|%s:%d: bind success.\n", __FILE__, __LINE__);
  }
  // 3. ��������˿� listen
  if (listen(_sock, 128) == SOCKET_ERROR) {
    printf("ERROR|%s:%d: listen fail, errno is: %d.\n", __FILE__, __LINE__,
           errno);
    WSACleanup();
    ExitProcess(errno);
  } else {
    printf("SUCCESS|%s:%d: listen success.\n", __FILE__, __LINE__);
  }
  // 4. �ȴ����տͻ�������accept
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
    // �˴�������ʹ�� inet_ntoa()����ʹ�� inet_ntop ���
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client.sin_addr, ipStr, INET_ADDRSTRLEN);
    printf("New client access. IP = %s\n", ipStr);
    // ѭ������ǰ�����ӵĿͻ��˵�����
    while (true) {
      // ����ս��ջ�����
      memset(recvBuf, 0, 1024);
      // 5. ���տͻ������ݣ�������յ������ݳ���Ϊ0�����˳�ѭ��
      recvLen = recv(_cSock, recvBuf, sizeof(DataHeader), 0);
      if (recvLen <= 0) {
        printf("Client has exit, task end.\n");
        break;
      }
      DataHeader *header = (DataHeader *)recvBuf;
      switch (header->cmd) {
        case CMD::CMD_LOGIN: {
          // ����Login�а�����DataHeader���ڶ��ν���ʱ����Ҫ�����ڴ�ƫ��
          recv(_cSock, recvBuf + sizeof(DataHeader),
               header->dataLength - sizeof(DataHeader), 0);
          Login *login = (Login *)recvBuf;
          printf(
              "INFO|Receive command: login, data length: %d, userName: %s.\n",
              login->dataLength, login->userName);
          // �����ж��û�������
          // ���ͻ��˷��ص�½�ɹ�����Ϣ
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
  // 8. �ر�socket closesocket
  closesocket(_sock);
  // ����WindowsSocket�������ɻ���
  WSACleanup();
  return 0;
}