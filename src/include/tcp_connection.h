#pragma once
#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H
#include <functional>
#include <memory>

#include "channel.h"
#include "inet_addr.h"
#include "macros.h"
class InetAddr;
class EventLoop;
class Buffer;
class TcpConnection final : public std::enable_shared_from_this<TcpConnection> {
 public:
  DISALLOW_COPY(TcpConnection);
  TcpConnection(EventLoop *loop,
                int sockfd,
                uint64_t id,
                const InetAddr &local,
                const InetAddr &peer);
  ~TcpConnection();
  enum ConnState {
    kConnecting,
    kEstablished,
    kHalfShutdown,
    kDisconnected,
  };
  void SetState(ConnState state) { state_ = state; }
  void SetConnectionCallback(
      const std::function<void(std::shared_ptr<TcpConnection>)> &cb) {
    conn_cb_ = cb;
  }
  void SetReadableCallback(
      const std::function<void(std::shared_ptr<TcpConnection>, Buffer *)> &cb) {
    readable_cb_ = cb;
    channel_.EnableRead();
  }
  void SetWriteCompleteCallback(
      const std::function<void(std::shared_ptr<TcpConnection>)> &cb) {
    write_cb_ = cb;
  }
  void SetCloseCallback(
      const std::function<void(std::shared_ptr<TcpConnection>)> &cb) {
    close_cb_ = cb;
  }
  auto GetPeer() { return peer_; }
  void OnEstablished();
  void DestroyConnection();
  void ForceClose();
  void HandleRead();
  void HandleWrite();
  void HandleClose();
  void HandleError(ssize_t read_ret, int err);
  void Send(const char *data, size_t size);
  void Shutdown();
  EventLoop *GetLoop() { return loop_; }
  uint64_t GetId() { return id_; }
  ConnState GetState() { return state_; };

 private:
  void ShutdownUnsafe();
  void SendUnsafe(const char *data, size_t size);
  int fd_;
  ConnState state_;
  uint64_t id_;
  EventLoop *loop_;
  Channel channel_;
  InetAddr local_;
  InetAddr peer_;
  std::unique_ptr<Buffer> input_buffer_;
  std::unique_ptr<Buffer> output_buffer_;
  std::function<void(std::shared_ptr<TcpConnection>)> conn_cb_;
  // TcpConnection中带有缓冲区，所以这两回调函数实际上是处理缓冲区的。
  std::function<void(std::shared_ptr<TcpConnection>, Buffer *)> readable_cb_;
  std::function<void(std::shared_ptr<TcpConnection>)> write_cb_;
  std::function<void(std::shared_ptr<TcpConnection>)> close_cb_;
};
#endif  // TCP_CONNECTION_H
