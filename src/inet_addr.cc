#include "inet_addr.h"

#include <arpa/inet.h>

#include <cassert>
#include <string>

InetAddr::InetAddr(uint16_t port, bool loopback_only) {
  addr_.sin_family = AF_INET;
  addr_.sin_addr.s_addr = loopback_only ? INADDR_LOOPBACK : INADDR_ANY;
  addr_.sin_port = htons(port);
}

InetAddr::InetAddr(std::string_view ip, uint16_t port) {
  addr_.sin_family = AF_INET;
  assert(inet_pton(addr_.sin_family, ip.data(), &addr_.sin_addr.s_addr) > 0);
  addr_.sin_port = htons(port);
}

uint16_t InetAddr::GetPort() const { return ntohs(addr_.sin_port); }
std::string InetAddr::GetIp() const {
  static_assert(INET6_ADDRSTRLEN > INET_ADDRSTRLEN);
  char buf[INET6_ADDRSTRLEN + 1]{};
  assert(buf ==
         inet_ntop(addr_.sin_family, &addr_.sin_addr, buf, sizeof(addr_)));
  return std::string{buf};
}
