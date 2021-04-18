#include "pch.h"

#ifdef _WIN32
#include <Windows.h>
#endif

#include <cerrno>
#include <cstdio>

void logError(const char* file, int line, const char* funcName) {
#ifdef _WIN32
  int errNum = GetLastError();
#else
  int errNum = errno;
  errno = 0;
#endif
  printf("ERROR|%s:%d: %s FAIL, ERROR: %d.\n", file, line, funcName, errNum);
}
void logSuccess(const char* file, int line, const char* funcName) {
  printf("SUCCESS|%s:%d: %s success.\n", file, line, funcName);
}
