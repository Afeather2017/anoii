#pragma once
#include <string>
#ifndef INET_ADDR_H
#define INET_ADDR_H
#include <netinet/in.h>

#include <cstdint>
#include <string_view>
class InetAddr final {
 public:
  // For accept socket only
  explicit InetAddr(uint16_t port, bool loopback_only = false);
  explicit InetAddr(struct sockaddr_in addr) : addr_{addr} {}
  InetAddr();
  InetAddr(std::string_view ip, uint16_t port);
  ~InetAddr() = default;
  struct sockaddr* GetSockAddr() {
    return reinterpret_cast<struct sockaddr*>(&addr_);
  }
  int GetDomain() const { return addr_.sin_family; }
  uint16_t GetPort() const;
  std::string GetIp() const;
  std::string GetAddr() const {
    return GetIp() + ':' + std::to_string(GetPort());
  }
  char* GetAddrBytes();
  const char* GetAddrBytes() const;
  bool operator==(const InetAddr& rhs);

 private:
  struct sockaddr_in addr_{};
};
namespace std {
template <>
struct hash<InetAddr> {
  auto operator()(const InetAddr& addr) {
    long long val = *(int*)(addr.GetAddrBytes()) * 0x10000;
    val += addr.GetPort();
    return hash<long long>{}(val);
  }
};
};  // namespace std
#endif  // INET_ADDR_H
