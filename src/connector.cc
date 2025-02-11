#include "connector.h"

#include <sys/socket.h>
#include <unistd.h>

#include <cstring>

#include "event_loop.h"
#include "logger.h"
#include "macros.h"
#include "socket.h"
#include "timer_queue.h"
Connector::Connector(EventLoop *loop, const InetAddr &addr)
    : loop_{loop}, addr_{addr}, channel_{loop_} {}

void Connector::Start() {
  loop_->RunInLoop(std::bind(&Connector::ConnectOnce, this));
}

void Connector::Connecting() {
  ASSERT(fd_ >= 0, "Connector::Connecting: fd shall >= 0");
  state_ = kConnecting;
  channel_ = Channel(loop_, fd_);
  channel_.DisableAll();
  channel_.SetWriteCallback(std::bind(&Connector::HandleWrite, this));
  channel_.SetErrorCallback(std::bind(&Connector::HandleError,
                                      this,
                                      std::placeholders::_1,
                                      std::placeholders::_2));
  channel_.EnableWrite();
}

// 进入到Retry的时候，要保证所有资源都释放了。
void Connector::Retry() {
  loop_->AssertIfOutLoopThread();
  if (retry_ms_ == 500) {
    timer_id_ = loop_->AddTimer(
        [this](mstime_t ms) {
          // Timer的生命周期可能比Connector更长，所以Timer调用的时候，Connector已经析构了。
          // 所以需要避免这种情况。
          // 方案是给Timer添加一个Cancel方法，详情见Timer的实现。
          this->ConnectOnce();
          return this->retry_ms_;
        },
        retry_ms_);
  }
  retry_ms_ = retry_ms_ * 2 > 30 * 1000 ? 30 * 1000 : retry_ms_ * 2;
}

void Connector::ConnectOnce() {
  fd_ = socket(AF_INET, SOCK_STREAM, 0);
  ErrorIf(fd_ < 0, "Cannot create socket: {}", strerror(errno));
  SetCloseExecNonBlock(fd_);
  int ret = ::connect(fd_, addr_.GetSockAddr(), sizeof(*addr_.GetSockAddr()));
  int err = ret == 0 ? 0 : errno;
  switch (err) {
    // 根据文档，有以下错误
    // 无法恢复的错误
    case EAFNOSUPPORT:  // AF_INET的值错误
    case EBADF:         // 文件描述符错误
    case ENOTSOCK:      // 不是套接字
    case EPROTOTYPE:    // 协议错误
    case ENOENT:        // 本地套接字的错误，文件名错误
    case ENOTDIR:       // 本地套接字错误，文件夹错误
    case EACCES:        // 权限错误
    case EINVAL:        // sockaddr参数错误
    case ELOOP:         // 软链接数量到达最大值
    case ENAMETOOLONG:  // 本地套接字路径过长
    case ENOBUFS:       // 无法开辟缓冲区，内存不足
    case EOPNOTSUPP:    // 地址正在被用于监听
    case EIO:           // IO错误
      Error("Cannot connect to {}: {}", addr_.GetAddr(), strerror(err));
      ::close(fd_);
      fd_ = -1;
      break;
    // 以下需要重试
    case ENETDOWN:       // 网络已经关闭
    case EADDRINUSE:     // 目标端口已经正在使用
    case EHOSTUNREACH:   // 该IP不可到达
    case EADDRNOTAVAIL:  // 该地址不可达
    case ECONNREFUSED:   // 远端拒绝
    case ENETUNREACH:    // 没有路由
    case ETIMEDOUT:      // 超时
    case ECONNRESET:     // 连接被重置，也就是收到了RST包
      Error("Temperary error, connect {}: {}", addr_.GetAddr(), strerror(err));
      ::close(fd_);
      fd_ = -1;
      Retry();
      break;
    // 以下表明正在连接
    case EISCONN:      // 已经连接
    case EALREADY:     // 正在连接中
    case EINPROGRESS:  // 异步请求，正在连接中
    case EINTR:        // 正在连接中，但是被信号打断
    case EAGAIN:       // 非阻塞情况下连接还没有建立
    case 0:
      Debug("Connecting {} async: {}", addr_.GetAddr(), strerror(err));
      Connecting();
      break;
    default:
      Error("Unexpect error to connect {}: {}", addr_.GetAddr(), strerror(err));
      ::close(fd_);
      fd_ = -1;
      break;
  }
}

void Connector::StopInLoop() {
  loop_->AssertIfOutLoopThread();
  // Stop要做的事情很多，其原因是，Connector操作的对象很多，有Channel,
  // Timer, sockfd。要保证在任意时刻调用它时，资源都需要被正确释放
  if (fd_ != -1) {
    ::close(fd_);
    fd_ = -1;
  }
  if (channel_.GetIndex() != -1) {
    channel_.DisableAll();
    loop_->RemoveChannel(&channel_);
  }
  stoped_ = true;
  if (timer_id_ != 0) loop_->CancelTimer(timer_id_);
}

void Connector::Stop() {
  loop_->RunInLoop(std::bind(&Connector::StopInLoop, this));
}

void Connector::HandleError(long int write_ret, int err) {
  loop_->AssertIfOutLoopThread();
  // 在异步连接中，如果连接没有马上建立，那么连接会处于正在连接中的状态。
  // 此时进入poll后会触发错误，继而调用此回调。
  // 所以处于连接中的状态，需要直接返回而不是关闭文件描述符。
  if (state_ != kConnecting) return;
  Error("Connect to {} failed: {}", addr_.GetAddr(), strerror(err));
  if (fd_ != -1) {
    ::close(fd_);
    fd_ = -1;
    RemoveChannel();
  }
}

static bool IsSelfConn(int sockfd) {
  struct sockaddr_in local, peer;
  socklen_t len = sizeof(struct sockaddr_in);
  int ret =
      ::getsockname(sockfd, reinterpret_cast<struct sockaddr *>(&local), &len);
  if (ret < 0) {
    Error("getsockname failed: {}", strerror(errno));
    return false;
  }
  ret = ::getpeername(sockfd, reinterpret_cast<struct sockaddr *>(&peer), &len);
  if (ret < 0) {
    Error("getpeername failed: {}", strerror(errno));
    return false;
  }
  ASSERT(local.sin_family == AF_INET && peer.sin_family == AF_INET,
         "IsSelfConn: Incorrect sin_family");
  return local.sin_addr.s_addr == peer.sin_addr.s_addr &&
         local.sin_port == peer.sin_port;
}

void Connector::HandleWrite() {
  loop_->AssertIfOutLoopThread();
  if (state_ == kConnecting) {
    // 当对端不可达的时候，触发写入事件，所以要检查一下错误
    int err = GetSockError(fd_);
    if (err != 0) {
      Warn("Connect failed: {}", strerror(err));
      RemoveChannel();
      ::close(fd_);
      fd_ = -1;
      Retry();
    } else if (IsSelfConn(fd_)) {
      Warn("Self connection detected.");
      RemoveChannel();
      ::close(fd_);
      fd_ = -1;
      Retry();
    } else {
      state_ = kConnected;
      RemoveChannel();
      new_conn_cb_(fd_, addr_);
    }
  } else {
    ASSERT(state_ == kDisconnected,
           "Connector::HandleWrite: This shall never be reached.");
  }
}

void Connector::RemoveChannel() {
  channel_.DisableAll();
  loop_->RemoveChannel(&channel_);
}
