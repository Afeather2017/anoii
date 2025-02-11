#include "tcp_connection.h"

#include <sys/socket.h>
#include <unistd.h>

#include <cassert>
#include <functional>
#include <memory>

#include "buffer.h"
#include "event_loop.h"
#include "logger.h"
#include "socket.h"
static void DefaultReadCb(std::shared_ptr<TcpConnection> ptr, Buffer *buf) {
  buf->Pop(buf->size());
  Info("tcpid={} called tries call an invalid read cb", ptr->GetId());
}

static void DefaultWriteCb(std::shared_ptr<TcpConnection> ptr) {
  Info("tcpid={} called tries call an invalid write cb", ptr->GetId());
}

static void DefaultConnCb(std::shared_ptr<TcpConnection> ptr) {
  Info("tcpid={} called tries call an invalid conn cb", ptr->GetId());
}

static void DefaultCloseCb(std::shared_ptr<TcpConnection> ptr) {
  Info("tcpid={} called tries call an invalid close cb", ptr->GetId());
}

static void DefaultErrorCb(std::shared_ptr<TcpConnection> ptr) {
  Info("tcpid={} called tries call an invalid error cb", ptr->GetId());
}

TcpConnection::TcpConnection(EventLoop *loop,
                             int sockfd,
                             uint64_t id,
                             const InetAddr &local,
                             const InetAddr &peer)
    : loop_{loop}, local_{local}, peer_{peer}, channel_{loop, sockfd} {
  id_ = id;
  fd_ = sockfd;
  channel_.SetCloseCallback(std::bind(&TcpConnection::HandleClose, this));
  channel_.SetReadCallback(std::bind(&TcpConnection::HandleRead, this));
  channel_.SetWriteCallback(std::bind(&TcpConnection::HandleWrite, this));
  channel_.SetErrorCallback(std::bind(&TcpConnection::HandleError,
                                      this,
                                      std::placeholders::_1,
                                      std::placeholders::_2));
  // 设置默认回调，方便调试。
  SetReadableCallback(DefaultReadCb);
  SetWriteCompleteCallback(DefaultWriteCb);
  SetConnectionCallback(DefaultConnCb);
  SetCloseCallback(DefaultCloseCb);
  SetErrorCallback(DefaultErrorCb);
  SetState(kConnecting);
  input_buffer_ = std::make_unique<Buffer>(1024, 0);
  output_buffer_ = std::make_unique<Buffer>(1024, 8);
}

TcpConnection::~TcpConnection() {
  loop_->AssertIfOutLoopThread();
  close(fd_);
}

void TcpConnection::HandleRead() {
  loop_->AssertIfOutLoopThread();
  int err = 0;
  ssize_t n = input_buffer_->ReadFd(channel_.GetFd(), &err);
  if (n > 0) {
    readable_cb_(shared_from_this(), input_buffer_.get());
  } else if (n == 0) {
    HandleClose();
  } else {
    HandleError(n, err);
  }
}

void TcpConnection::HandleWrite() {
  loop_->AssertIfOutLoopThread();
  char *data = output_buffer_->begin();
  int size = output_buffer_->ReadableBytes();
  ssize_t ret = ::write(channel_.GetFd(), data, size);
  Trace("Write {} byte to {} got ret={}", size, channel_.GetFd(), ret);
  if (ret < 0) {
    if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
      Error("write socket: {}", strerror(errno));
    }
  } else if (ret <= size) {
    output_buffer_->Pop(ret);
    if (output_buffer_->Empty()) {
      Trace("tcpid={} is disabled cuz buffer is empty", id_);
      channel_.DisableWrite();
      loop_->QueueInLoop(std::bind(write_cb_, shared_from_this()));
      if (GetState() == kHalfShutdown) {
        ShutdownUnsafe();
      }
    }
  }
}

void TcpConnection::DestroyConnection() {
  loop_->AssertIfOutLoopThread();
  if (state_ == kEstablished) {
    state_ = kDisconnected;
    channel_.DisableAll();
  }
  loop_->RemoveChannel(&channel_);
}

void TcpConnection::HandleClose() {
  loop_->AssertIfOutLoopThread();
  assert(state_ == kEstablished || state_ == kHalfShutdown);
  // FIXME: 此处直接DisableAll，可能会导致可写入事件不再发生。
  // 在output_buffer中可能残留部分数据将不会再发送。
  channel_.DisableAll();
  close_cb_(shared_from_this());
}

void TcpConnection::HandleError(ssize_t read_ret, int err) {
  loop_->AssertIfOutLoopThread();
  Warn("tcpid={} error_code=({},{},{})",
       id_,
       read_ret,
       GetSockError(channel_.GetFd()),
       strerror(err));
}

void TcpConnection::Send(const char *data, size_t size) {
  // FIXME: 线程安全性不足
  // 调用这个函数后，data需要存在多久？
  // 在当前线程调用，那么就是函数结束后就不需要继续存在了。
  // 在别的线程调用，则需要存活久一些，至少要到SendUnsafe执行之后。
  assert(state_ == kEstablished);
  loop_->RunInLoop(std::bind(&TcpConnection::SendUnsafe, this, data, size));
}

void TcpConnection::SendUnsafe(const char *data, size_t size) {
  loop_->AssertIfOutLoopThread();
  ASSERT(write_cb_, "write complete cb called but not set");
  // 竞态条件：
  // 不能够仅仅探测 channel 是否正在写入。
  // HandleWrite处，当输出buffer为空时，会关闭关闭IsWriting。
  // 这样的问题是，如果此时Append了，那么最后可能有一些数据残留且不会发送。
  //
  // 这可能发生吗？
  // SendUnsafe和HandleWrite只可能同时在同一个线程上执行，所以不会
  if (channel_.IsWriting()) {
    output_buffer_->Append(data, size);
    return;
  }
  if (!output_buffer_->Empty()) {
    // 缓存区中依然还有数据待发送
    output_buffer_->Append(data, size);
    channel_.EnableWrite();
  } else {
    // 缓存区中没有数据待发送
    ssize_t ret = ::write(channel_.GetFd(), data, size);
    if (ret < 0) {
      if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
        Error("write socket: {}", strerror(errno));
      }
      ret = 0;
    }
    if (ret < size) {
      Trace("tcpid={} enabled write to write more data", id_);
      output_buffer_->Append(data + ret, size - ret);
      channel_.EnableWrite();
    } else {
      // 当前数据发送完了
      loop_->QueueInLoop(std::bind(write_cb_, shared_from_this()));
    }
  }
}

void TcpConnection::Shutdown() {
  if (GetState() == kEstablished) {
    SetState(kHalfShutdown);
    loop_->RunInLoop(std::bind(&TcpConnection::ShutdownUnsafe, this));
  }
}

void TcpConnection::ShutdownUnsafe() {
  loop_->AssertIfOutLoopThread();
  if (!channel_.IsWriting()) {
    ::shutdown(fd_, SHUT_WR);
  }
}

void TcpConnection::OnEstablished() {
  assert(GetState() == kConnecting);
  auto self = shared_from_this();
  SetState(kEstablished);
  if (conn_cb_) conn_cb_(self);
}
