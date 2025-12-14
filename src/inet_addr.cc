#include "inet_addr.h"

#include <arpa/inet.h>

#include <cassert>
#include <cstring>
#include <string>

InetAddr::InetAddr(uint16_t port, bool loopback_only) {
  addr_.sin_family = AF_INET;
  addr_.sin_addr.s_addr = loopback_only ? INADDR_LOOPBACK : INADDR_ANY;
  addr_.sin_port = htons(port);
}

InetAddr::InetAddr() {
  addr_.sin_family = AF_INET;
  addr_.sin_addr.s_addr = INADDR_ANY;
  addr_.sin_port = htons(0);
}

InetAddr::InetAddr(std::string_view ip, uint16_t port) {
  addr_.sin_family = AF_INET;
  int ret = inet_pton(addr_.sin_family, ip.data(), &addr_.sin_addr.s_addr);
  assert(ret > 0);
  addr_.sin_port = htons(port);
}

uint16_t InetAddr::GetPort() const { return ntohs(addr_.sin_port); }
std::string InetAddr::GetIp() const {
  static_assert(INET6_ADDRSTRLEN > INET_ADDRSTRLEN);
  char buf[INET6_ADDRSTRLEN + 1]{};
  auto* ptr = inet_ntop(addr_.sin_family, &addr_.sin_addr, buf, sizeof(addr_));
  assert(ptr == buf);
  return std::string{buf};
}

char* InetAddr::GetAddrBytes() {
  assert(addr_.sin_family == AF_INET);
  return reinterpret_cast<char*>(&addr_.sin_addr.s_addr);
}

const char* InetAddr::GetAddrBytes() const {
  assert(addr_.sin_family == AF_INET);
  return reinterpret_cast<const char*>(&addr_.sin_addr.s_addr);
}

bool InetAddr::operator==(const InetAddr& rhs) {
  return 0 == memcmp(&rhs.addr_, &addr_, sizeof(struct sockaddr_in));
}
