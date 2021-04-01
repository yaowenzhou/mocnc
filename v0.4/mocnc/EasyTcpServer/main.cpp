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
#include <stdio.h>

#include "Data.h"

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
    DataHeader header = {};
    char const *cmds[2] = {"login", "logout"};
    char const *cmd = nullptr;
    // ѭ������ǰ�����ӵĿͻ��˵�����
    while (true) {
      // ��ս������ݻ�����������
      memset(&header, 0, 1024);
      // 5. ���տͻ������ݣ�������յ������ݳ���Ϊ0�����˳�ѭ��
      if (recv(_cSock, (char *)&header, sizeof(DataHeader), 0) <= 0) {
        printf("Client has exit, task end.\n");
        break;
      }

      switch (header.cmd) {
        case CMD::CMD_LOGIN: {
          cmd = "login";
          Login login = {};
          recv(_cSock, (char *)&login, sizeof(Login), 0);
          // �����ж��û�������
          // ���ͻ��˷��ص�½�ɹ�����Ϣ
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
  // 8. �ر�socket closesocket
  closesocket(_sock);
  // ����WindowsSocket�������ɻ���
  WSACleanup();
  return 0;
}