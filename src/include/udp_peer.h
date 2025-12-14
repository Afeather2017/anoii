#pragma once
#ifndef UDP_SERVER_H
#define UDP_SERVER_H
#include <functional>
#include <vector>

#include "channel.h"
#include "inet_addr.h"
class InetAddr;
class EventLoop;
class UdpPeer final {
 public:
  DISALLOW_COPY(UdpPeer);
  DISALLOW_MOVE(UdpPeer);
  // buffer_size: 用以接收UDP包的缓冲区的大小。
  //              如果buffer_size > 0，那么如果某个UDP包的大小超过了设置的
  //              缓冲区大小，那么包将会被丢弃。
  //              如果buffer_size = 0，那么表示一个包必须被完整的接收下来。
  //              由于大多数包都小于1472字节（一个MTU大小-UDP头与IP头大小），
  //              所以默认设置为1472
  // addr       : 监听的IP地址
  UdpPeer(EventLoop* loop, const InetAddr& addr, unsigned buffer_size = 1472);
  explicit UdpPeer(EventLoop* loop, unsigned buffer_size = 1472);
  void SetReadableCallback(
      std::function<void(UdpPeer*, InetAddr&, char*, int)> cb) {
    readable_cb_ = cb;
  }
  void SendTo(const char* data, int size, InetAddr& addr);
  void SendTo(std::string_view str, InetAddr& addr);
  ~UdpPeer();

 private:
  void OnMessage();
  std::function<void(UdpPeer*, InetAddr&, char*, int)> readable_cb_{};
  std::vector<char> buffer_{};
  Channel channel_;
  InetAddr addr_;
  int fd_;
  bool auto_buffer_size_{false};
  bool binded_addr_{false};
};
#endif  // UDP_SERVER_H
