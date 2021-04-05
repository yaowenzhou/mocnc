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

#include "../Common/Data.h"

// ����WindowsSocket�ľ�̬�⣬�˴���ȡ�ڶ��ַ�����Ϊ������Ӹ���������
// #pragma comment(lib, "ws2_32.lib")

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
    // 3. ��ȡָ��
    // �����Ҫ��ȡ���пո���ַ������˴�����ֱ��ʹ��scanf��scanf_s�����Ƕ�ȡ���ո�ͻ�ֹͣ
    // scanf_s("%s", cmdBuf, 1023);
    scanf_s("%[^\n]", cmdBuf, 1023);
    scanf_s("%c", &a, 1);
    // gets_s(cmdBuf, 1023); // ���һ���ַ�λ������'\0'
    DataHeader retHeader = {};
    if (strcmp(cmdBuf, "exit") == 0) {
      printf("Exiting...\n");
      break;
    } else if (strcmp(cmdBuf, "login") == 0) {
      Login login;
      strcpy_s(login.userName, "xiaoyao");
      strcpy_s(login.password, "123456");
      send(_sock, (const char*)&login, sizeof(Login), 0);
      LoginResult loginResult;
      recv(_sock, (char*)&loginResult, sizeof(LoginResult), 0);
      printf("INFO|Login return: %d.\n", loginResult.result);
    } else if (strcmp(cmdBuf, "logout") == 0) {
      Logout logout;
      strcpy_s(logout.userName, "xiaoyao");
      DataHeader dataHeader = {sizeof(Logout), CMD_LOGOUT};
      send(_sock, (const char*)&logout, sizeof(Logout), 0);
      LoginResult logoutResult;
      recv(_sock, (char*)&logoutResult, sizeof(LogoutResult), 0);
      printf("INFO|Logout return: %d.\n", logoutResult.result);
    } else {
      printf("Unsupported command, please retry.\n");
      continue;
    }
  }

  // 4. �ر��׽���closesocket
  closesocket(_sock);
  // ����WindowsSocket�������ɻ���
  WSACleanup();
  system("pause");
  return 0;
}