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
#include <cstdlib>

// �ͻ��˺ͷ���˶�������ͬ�Ľṹ��
// ��������ͬһ̨������ͬһ�����߱���ģ�������ݴ�Сһģһ��
struct DataPackage {
  int age;
  char name[32];
};

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
    // �����Ҫ��ȡ���пո���ַ������˴�����ʹ��scanf��scanf_s�����Ƕ�ȡ���ո�ͻ�ֹͣ
    // scanf_s("%s", cmdBuf, 1023);
    scanf_s("%[^\n]", cmdBuf, 1023);
    scanf_s("%c", &a, 1);
    // gets_s(cmdBuf, 1023); // ���һ���ַ�λ������'\0'
    if (0 == strcmp(cmdBuf, "exit")) {
      printf("Exiting...\n");
      break;
    }
    // 4. ��ָ��͸�������
    send(_sock, cmdBuf, strlen(cmdBuf) + 1, 0);
    // 5. ���շ�������Ϣrecv
    char recvBuf[256] = {};
    if (recv(_sock, recvBuf, 256, 0) == 0) {
      printf("SUCCESS|%s:%d: recv failed with error: %u\n", __FILE__, __LINE__,
             WSAGetLastError());
      WSACleanup();
      return -1;
    }
    // ��������ֱ��ǿת�������ǳ�low���Ҳ���ȫ����ǰ����Ϊ������İ汾
    DataPackage* dp = (DataPackage*)recvBuf;
    printf("Get msg from server:\n%d, %s\n", dp->age, dp->name);
  }

  // 4. �ر��׽���closesocket
  closesocket(_sock);
  // ����WindowsSocket�������ɻ���
  WSACleanup();
  system("pause");
  return 0;
}