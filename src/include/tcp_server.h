#pragma once
#ifndef TCP_SERVER_H
#define TCP_SERVER_H
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>

#include "macros.h"
class EventLoop;
class InetAddr;
class Acceptor;
class TcpConnection;
class Buffer;
class TcpServer final {
 public:
  DISALLOW_COPY(TcpServer);
  TcpServer(EventLoop *loop, InetAddr &addr);
  ~TcpServer();
  void SetConnectionCallback(
      const std::function<void(std::shared_ptr<TcpConnection>)> &cb) {
    conn_cb_ = cb;
  }
  void SetReadableCallback(
      const std::function<void(std::shared_ptr<TcpConnection>, Buffer *)> &cb) {
    readable_cb_ = cb;
  }
  void SetWriteCompleteCallback(
      const std::function<void(std::shared_ptr<TcpConnection>)> &cb) {
    write_cb_ = cb;
  }

 private:
  void RemoveConnection(std::shared_ptr<TcpConnection> conn);
  void NewConnection(int fd, InetAddr *addr);
  std::function<void(std::shared_ptr<TcpConnection>)> conn_cb_;
  std::function<void(std::shared_ptr<TcpConnection>, Buffer *)> readable_cb_;
  std::function<void(std::shared_ptr<TcpConnection>)> write_cb_;
  // 如果没有这个映射，那么当用户不持有std::shared_ptr<TcpConnection>的时候，
  // 引用计数降为0，马上就会析构了，然而poller中的fd与std::unordered_map<int,
  // Channel*> fd_channels_没有删掉，所以依然可能调用回调，此时程序很可能崩溃。
  // 可以理解为给远端分配了一个shared_ptr。
  // 虽然确实可以调用loop->RemoveChannel()，然而由于持有此shared_ptr<TcpConnection>
  // 的线程不一定是其EventLoop中，所以调用在Poller中删除会有延迟，所以在
  // TcpConnection的析构中加入loop->RemoveChannel是不可行的……
  // 至于为什么非得用id:shared_ptr这种映射，是因为两次获得的shared_ptr的地址可能相同
  // 而单调递增的id则不可能相同
  std::unordered_map<uint64_t, std::shared_ptr<TcpConnection>> id_conn_;
  EventLoop *loop_{};
  Acceptor *acceptor_{};
  uint64_t next_id_{};
  bool started_{false};
};
#endif  // TCP_SERVER_H
