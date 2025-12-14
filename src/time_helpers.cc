#include "time_helpers.h"

#include <sys/time.h>

#include <cassert>
#include <ctime>
mstime_t CurrentTimeMs() {
  struct timeval tv;
  int ret = gettimeofday(&tv, nullptr);
  assert(ret == 0);
  (void)ret;
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}
std::string GetNowTimeString() {
  struct timeval tv;
  gettimeofday(&tv, 0);
  struct tm* tm_time = localtime(&tv.tv_sec);
  char buffer[80];
  auto size = strftime(buffer, sizeof(buffer), "%Y%m%d %H:%M:%S", tm_time);
  sprintf(buffer + size, ".%03ld", (long)tv.tv_usec / 1000);
  return std::string(buffer);
}
