#pragma once
#include <string_view>
#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H
#include <any>
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
  TcpConnection(EventLoop* loop,
                int sockfd,
                uint64_t id,
                const InetAddr& local,
                const InetAddr& peer);
  ~TcpConnection();
  enum ConnState {
    kConnecting,
    kEstablished,
    kHalfShutdown,
    kDisconnected,
  };
  void SetState(ConnState state) { state_ = state; }
  void SetConnectionCallback(
      const std::function<void(std::shared_ptr<TcpConnection>)>& cb) {
    conn_cb_ = cb;
  }
  void SetReadableCallback(
      const std::function<void(std::shared_ptr<TcpConnection>, Buffer*)>& cb) {
    readable_cb_ = cb;
    channel_.EnableRead();
  }
  void SetWriteCompleteCallback(
      const std::function<void(std::shared_ptr<TcpConnection>)>& cb) {
    write_cb_ = cb;
  }
  void SetCloseCallback(
      const std::function<void(std::shared_ptr<TcpConnection>)>& cb) {
    close_cb_ = cb;
  }
  void SetHighWatermarkCallback(
      const std::function<void(std::shared_ptr<TcpConnection>)>& cb) {
    watermark_cb_ = cb;
  }
  void SetHighWatermark(long long watermark) { watermark_ = watermark; }
  auto GetPeer() { return peer_; }
  void OnEstablished();
  void DestroyConnection();
  void ForceClose();
  void HandleRead();
  void HandleWrite();
  void HandleClose();
  void HandleError(ssize_t read_ret, int err);
  void Send(const char* data, size_t size);
  void Send(std::string_view sv);
  void Shutdown();
  EventLoop* GetLoop() { return loop_; }
  uint64_t GetId() { return id_; }
  ConnState GetState() { return state_; };
  void SetContext(std::any context) { context_ = std::move(context); }
  template <typename Type>
  Type& GetContext() {
    return std::any_cast<Type&>(context_);
  }

 private:
  void ShutdownUnsafe();
  void SendUnsafe(const char* data, size_t size);
  int fd_;
  ConnState state_;
  // TODO: 将ID更改为字符串形式以分辨清楚该连接的作用
  uint64_t id_;
  // 1GB大小限制，大概够应付多数场景了吧？
  long long watermark_{1024 * 1024 * 1024};
  EventLoop* loop_{};
  Channel channel_;
  InetAddr local_;
  InetAddr peer_;
  std::unique_ptr<Buffer> input_buffer_;
  std::unique_ptr<Buffer> output_buffer_;
  std::function<void(std::shared_ptr<TcpConnection>)> conn_cb_;
  // TcpConnection中带有缓冲区，所以这两回调函数实际上是处理缓冲区的。
  std::function<void(std::shared_ptr<TcpConnection>, Buffer*)> readable_cb_;
  std::function<void(std::shared_ptr<TcpConnection>)> write_cb_;
  std::function<void(std::shared_ptr<TcpConnection>)> close_cb_;
  std::function<void(std::shared_ptr<TcpConnection>)> watermark_cb_;
  std::any context_;
};

void DefaultReadCb(std::shared_ptr<TcpConnection> ptr, Buffer* buf);
void DefaultWriteCb(std::shared_ptr<TcpConnection> ptr);
void DefaultConnCb(std::shared_ptr<TcpConnection> ptr);
void DefaultCloseCb(std::shared_ptr<TcpConnection> ptr);
void DefaultHighWatermarkCb(std::shared_ptr<TcpConnection> ptr);
#endif  // TCP_CONNECTION_H
