#pragma once
#ifndef ACCEPTOR_H
#define ACCEPTOR_H
#include <sys/socket.h>

#include <functional>

#include "channel.h"
#include "inet_addr.h"
#include "macros.h"
class Acceptor final {
 public:
  DISALLOW_COPY(Acceptor);
  // backlog = 0时，安卓系统中进行网络连接的时候，accept事件无法触发。
  Acceptor(EventLoop *loop,
           const InetAddr &addr,
           int backlog = SOMAXCONN,
           bool reuse_addr = false,
           bool reuse_port = false);
  void SetNewConnectionCallback(std::function<void(int, InetAddr *)> cb) {
    new_conn_cb_ = std::move(cb);
  }
  void AcceptHandler();
  Channel *GetChannel() { return &channel_; }
  const InetAddr &GetAddr() { return addr_; }

 private:
  Channel channel_;
  InetAddr addr_;
  std::function<void(int peer_fd, InetAddr *perr_addr)> new_conn_cb_;
};
#endif  // ACCEPTOR_H
