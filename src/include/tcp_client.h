#pragma once
#include <functional>

#include "inet_addr.h"
#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H
#include <memory>

#include "macros.h"
class EventLoop;
class Connector;
class TcpConnection;
class Buffer;
class TcpClient final {
 public:
  DISALLOW_COPY(TcpClient);
  TcpClient(EventLoop* loop, const InetAddr& addr);
  void SetRetry(bool retry) { retry_ = retry; }
  void SetConnectionCallback(
      std::function<void(std::shared_ptr<TcpConnection>)> cb) {
    conn_cb_ = cb;
  }
  void SetReadableCallback(
      std::function<void(std::shared_ptr<TcpConnection>, Buffer*)> cb) {
    readable_cb_ = cb;
  }
  void SetWriteCompleteCallback(
      std::function<void(std::shared_ptr<TcpConnection>)> cb) {
    write_cb_ = cb;
  }
  void SetHighWatermarkCallback(
      const std::function<void(std::shared_ptr<TcpConnection>)>& cb) {
    watermark_cb_ = cb;
  }
  ~TcpClient();

 private:
  void NewConnection(int fd, const InetAddr& addr);
  void HandleClose();
  EventLoop* loop_;
  std::unique_ptr<Connector> connector_;
  bool retry_ = false;
  InetAddr peer_;
  std::shared_ptr<TcpConnection> conn_;
  std::function<void(std::shared_ptr<TcpConnection>)> conn_cb_;
  // TcpConnection中带有缓冲区，所以这两回调函数实际上是处理缓冲区的。
  std::function<void(std::shared_ptr<TcpConnection>, Buffer*)> readable_cb_;
  std::function<void(std::shared_ptr<TcpConnection>)> write_cb_;
  std::function<void(std::shared_ptr<TcpConnection>)> watermark_cb_;
  static long long id_;
};
#endif  // TCP_CLIENT_H
