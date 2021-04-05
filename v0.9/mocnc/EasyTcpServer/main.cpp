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
#include <unordered_set>

#include "../Common/Data.h"

// ����WindowsSocket�ľ�̬�⣬�˴���ȡ�ڶ��ַ�����Ϊ������Ӹ���������
// #pragma comment(lib, "ws2_32.lib")

// ����һ��ȫ�ֵĶ�̬���飬����
std::unordered_set<SOCKET> readSockets;
std::unordered_set<SOCKET> writeSockets;
std::unordered_set<SOCKET> expSockets;

// �涨���ͻ����˳�ʱ������
// -1����ʱ��Ҫ�������������н�ͨ��socket����Ӧ�ļ������Ƴ�
int doRead(SOCKET _cSock) {
  char recvBuf[1024] = {};
  // 5. ���տͻ������ݣ�������յ������ݳ���Ϊ0�����˳�����
  if (recv(_cSock, recvBuf, sizeof(DataHeader), 0) <= 0) {
    printf("Client(%llu) has exit, task end.\n", _cSock);
    return -1;
  }
  DataHeader *header = (DataHeader *)recvBuf;
  switch (header->cmd) {
    case CMD::CMD_LOGIN: {
      // ����Login�а�����DataHeader���ڶ��ν���ʱ����Ҫ�����ڴ�ƫ��
      recv(_cSock, recvBuf + sizeof(DataHeader),
           header->dataLength - sizeof(DataHeader), 0);
      Login *login = (Login *)recvBuf;
      printf(
          "INFO|Receive command login from socket(%llu), data length: %d, "
          "userName: %s.\n",
          _cSock, login->dataLength, login->userName);
      // �����ж��û�������
      // ���ͻ��˷��ص�½�ɹ�����Ϣ
      LoginResult result;
      send(_cSock, (const char *const) & result, sizeof(LoginResult), 0);
    } break;
    case CMD::CMD_LOGOUT: {
      recv(_cSock, recvBuf + sizeof(DataHeader),
           header->dataLength - sizeof(DataHeader), 0);
      Logout *logout = (Logout *)recvBuf;
      printf(
          "INFO|Receive command logout from socket(%llu), data length: %d, "
          "userName: %s.\n",
          _cSock, logout->dataLength, logout->userName);
      LogoutResult result;
      send(_cSock, (const char *const) & result, sizeof(LogoutResult), 0);
    } break;
    default: {
      DataHeader header = {0, CMD_ERROR};
      printf("INFO|Receive unknow command.\n");
      send(_cSock, (const char *const) & header, sizeof(DataHeader), 0);
    } break;
  }
  return 0;
}

int main() {
  // �ȶ���һ���洢������ı���
  DWORD errCode = 0;
  // ����WindowsSocket 2.x �������̻���
  WORD ver = MAKEWORD(2, 2);
  WSADATA dat;
  if (WSAStartup(ver, &dat) != NO_ERROR) {
    printf("ERROR|%s:%d: WSAStartup error.\n", __FILE__, __LINE__);
    return 1;
  }
  // 1. ����һ��socket(����socket)
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
    errCode = GetLastError();
    printf("ERROR|%s:%d: bind fail, error: %u.\n", __FILE__, __LINE__, errCode);
    WSACleanup();
    ExitProcess(errCode);
  } else {
    printf("SUCCESS|%s:%d: bind success.\n", __FILE__, __LINE__);
  }
  // 3. ��������˿� listen
  if (listen(_sock, 128) == SOCKET_ERROR) {
    errCode = GetLastError();
    printf("ERROR|%s:%d: listen fail, error: %u.\n", __FILE__, __LINE__,
           errCode);
    WSACleanup();
    ExitProcess(errCode);
  } else {
    printf("SUCCESS|%s:%d: listen success.\n", __FILE__, __LINE__);
  }
  // ��ɼ����󣬽�_sock��ӵ� read_sockets, write_sockets, exp_sockets;
  readSockets.insert(_sock);
  writeSockets.insert(_sock);
  expSockets.insert(_sock);

  // 4. �ȴ����տͻ�������accept
  sockaddr_in client = {};
  int nAddrLen = sizeof(client);
  // �˱������ڽ����µ�ͨ��socket
  SOCKET _cSock = INVALID_SOCKET;
  char const *msg = nullptr;
  int recvLen = 0;
  char recvBuf[1024] = {};
  // ������socket
  fd_set readSet = {};
  fd_set writeSet = {};
  fd_set expSet = {};

  // v0.8: ��select����һ����ʱ����(���һ������)
  timeval t = {5, 0};

  while (true) {
    // ����ʵ�⣬�Լ����Ĵ���
    // ����FD_ZERO�����޸�fd_set.fd_count���������޸�fd_set.fd_array
    // ���Դ˴�����ȡFD_ZERO�ķ�ʽ��ʹ��memset����ָ�����ʼ״̬
    FD_ZERO(&readSet);
    FD_ZERO(&writeSet);
    FD_ZERO(&expSet);
    // memset(&readSet, 0, sizeof(fd_set));
    // memset(&writeSet, 0, sizeof(fd_set));
    // memset(&expSet, 0, sizeof(fd_set));
    // ����Ҫ��ص�socket��ӵ�read_set��write_set��exp_set��
    for (auto var : readSockets) {
      FD_SET(var, &readSet);
    }
    for (auto var : writeSockets) {
      FD_SET(var, &writeSet);
    }
    for (auto var : expSockets) {
      FD_SET(var, &expSet);
    }

    // nfds��һ������ֵ������fd_set���������������ֵ+1����Windows�������������д0
    // select ִ�г������˳�ѭ����
    //if (select(_sock + 1, &readSet, &writeSet, &expSet, &t) < 0) {
    if (select(_sock + 1, &readSet, nullptr, &expSet, &t) < 0) {
      printf("ERROR|The select task ends.\n");
      printf("ERROR|select error: %d.\n", GetLastError());
      break;
    }
    // ��⵽_sock�ɶ���˵�����µĿͻ�������
    if (FD_ISSET(_sock, &readSet)) {
      _cSock = accept(_sock, (sockaddr *)&client, &nAddrLen);
      if (_cSock == INVALID_SOCKET) {
        printf(
            "ERROR|%s:%d: accept error, get a invalid client socket, error: "
            "%u.\n",
            __FILE__, __LINE__, GetLastError());
      } else {
        // �˴�������ʹ�� inet_ntoa()����ʹ�� inet_ntop ���
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client.sin_addr, ipStr, INET_ADDRSTRLEN);
        printf("New client access. socket = %llu, IP = %s, Port = %u.\n",
               _cSock, ipStr, client.sin_port);
        // v0.8: �������ӵĿͻ���Ⱥ����Ŀǰ���е����пͻ���
        NewUserJoin newUser;
        newUser.sock = _cSock;
        for (SOCKET sock : readSockets) {
          send(sock, (const char *const) & newUser, sizeof(NewUserJoin), 0);
        }
        // �������ӵ�ͨ��socket��ӵ� read_sockets, write_sockets, exp_sockets;
        readSockets.insert(_cSock);
        writeSockets.insert(_cSock);
        expSockets.insert(_cSock);
      }
    }
    // ����ɶ���ͨ��socket
    for (int i = 0; i < readSet.fd_count; i++) {
      if (readSet.fd_array[i] == _sock) continue;
      // �����ǰ�ͻ����˳���doRet == -1
      // ��Ҫ����Ӧ��socket�����Ӧ�ļ������Ƴ�
      int doRet = doRead(readSet.fd_array[i]);
      if (doRet == -1) {
        // ͨ��socketҲ��Ҫ�رգ���֮ǰ�İ汾��
        // ��û�йرգ����ǲ��Ե�
        // һ��̳̻����鼮�н����֮Ϊ�ͻ���socket
        // �����Ҿ��ý���ͨ��socket���Ӻ���
        closesocket(readSet.fd_array[i]);
        readSockets.erase(readSet.fd_array[i]);
        writeSockets.erase(readSet.fd_array[i]);
        expSockets.erase(readSet.fd_array[i]);
      }
    }
    printf("INFO|Handle other business...\n");
  }
  // �ر����е�socket
  // һ����˵���߲�����һ���ģ�����Ϊ���ݴ����������
  for (auto sock : readSockets) {
    closesocket(sock);
  }
  // _sockҲ������һ���д����ˣ��˴�ע�͵���ش���
  // closesocket(_sock);
  // ����WindowsSocket�������ɻ���
  WSACleanup();
  return 0;
}