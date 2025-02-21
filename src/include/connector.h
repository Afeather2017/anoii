#pragma once
#ifndef CONNECT_H
#define CONNECT_H
#include "channel.h"
#include "definitions.h"
#include "inet_addr.h"
#include "macros.h"
class EventLoop;
// 用于解决发起连接的重试问题
class Connector {
 public:
  DISALLOW_COPY(Connector);
  Connector(EventLoop *loop, const InetAddr &addr);
  ~Connector() {
    Stop();
  }
  void SetNewConnCb(const std::function<void(int fd, const InetAddr &)> &cb) {
    new_conn_cb_ = cb;
  }
  void Start();
  // Stop可以反复调用
  void Stop();
  InetAddr *GetPeerAddr() { return &addr_; }

 private:
  void ConnectOnce();
  void Retry();
  enum State { kConnecting, kDisconnected, kConnected };
  void Connecting();
  void HandleWrite();
  void HandleError(long int write_ret, int err);
  void RemoveAndResetChannel();
  void StopInLoop();
  int fd_ = -1;
  int retry_ms_ = 500;
  State state_ = kConnecting;
  EventLoop *loop_{};
  TimerId timer_id_{};
  InetAddr addr_{};
  Channel channel_;
  std::function<void(int fd, const InetAddr &)> new_conn_cb_;
};
#endif  // CONNECT_H
