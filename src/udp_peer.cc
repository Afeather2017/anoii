#include "udp_peer.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cassert>
#include <cstring>

#include "logger.h"
#include "socket.h"
UdpPeer::UdpPeer(EventLoop *loop, const InetAddr &addr, unsigned buffer_size)
    // 由于需要多一个字节才可以确保一个包被完整读取了，所以是buffer_size+1
    : buffer_(buffer_size == 0 ? 1473 : buffer_size + 1)
    , channel_{loop}
    , addr_{addr}
    , binded_addr_{true} {
  Trace("Socket bind to {}", addr.GetAddr());
  auto_buffer_size_ = buffer_size == 0;
  fd_ = socket(AF_INET, SOCK_DGRAM, 0);
  FatalIf(fd_ < 0, "socket: {}", strerror(errno));
  int err = bind(fd_, addr_.GetSockAddr(), sizeof(*addr_.GetSockAddr()));
  FatalIf(err < 0, "bind to {} failed: {}", addr_.GetAddr(), strerror(errno));
  err = fcntl(fd_, F_SETFL, O_NONBLOCK);
  FatalIf(err < 0, "fcntl set nonblock failed: {}", strerror(errno));
  channel_.SetFd(fd_);
  channel_.SetReadCallback(std::bind(&UdpPeer::OnMessage, this));
  channel_.EnableRead();
}

UdpPeer::UdpPeer(EventLoop *loop, unsigned buffer_size)
    // 由于需要多一个字节才可以确保一个包被完整读取了，所以是buffer_size+1
    : buffer_(buffer_size == 0 ? 1473 : buffer_size + 1)
    , channel_{loop}
    , binded_addr_{false} {
  auto_buffer_size_ = buffer_size == 0;
  fd_ = socket(AF_INET, SOCK_DGRAM, 0);
  FatalIf(fd_ < 0, "socket: {}", strerror(errno));
  int err = fcntl(fd_, F_SETFL, O_NONBLOCK);
  FatalIf(err < 0, "fcntl set nonblock failed: {}", strerror(errno));
  channel_.SetFd(fd_);
  channel_.SetReadCallback(std::bind(&UdpPeer::OnMessage, this));
  channel_.EnableRead();
}

void UdpPeer::OnMessage() {
  // 由于公司有强依赖于localhost的UDP socket总是能够发送到对端，
  // 且总是能够收到对端发送的包的代码，所以就写成了尽可能接收数据的形式。
  InetAddr peer{};
  auto *peer_addr = peer.GetSockAddr();
  socklen_t len = sizeof(*peer.GetSockAddr());
  int size;
  if (!binded_addr_) {
    Fatal("Tries recvfrom an unbinded UDP socket");
  }
  for (;;) {
    size = ::recvfrom(fd_,
                      buffer_.data(),
                      buffer_.size(),
                      auto_buffer_size_ ? MSG_PEEK : 0,
                      peer_addr,
                      &len);
    if (size > 0) {
      if (!auto_buffer_size_) {
        if (size < buffer_.size()) break;
        Error("Package corrupted, ignore it.");
        return;
      }
      if (size >= buffer_.size()) {
        // UDP的包的长度字段包括了首部的长度，所以不是65535
        if (buffer_.size() * 2 >= 65527 + 1) {
          buffer_.resize(65527 + 1);
        } else {
          buffer_.resize(buffer_.size() * 2);
        }
        continue;
      }
      ::recvfrom(fd_, nullptr, 0, 0, nullptr, nullptr);
      break;
    }
    switch (size) {
      case EBADF:
      case ENOBUFS:
      case ENOMEM:
        // 致命错误，无法恢复
        Fatal("recvfrom: {}", strerror(errno));
        return;
      case ECONNRESET:
      case EAGAIN:
      case EINVAL:
      case ENOTCONN:
      case ENOTSOCK:
      case ETIMEDOUT:
      case EIO:
      case EOPNOTSUPP:
        // WTF???
        Error("recvfrom failed: {}", strerror(errno));
        return;
      case EINTR:
        if (!auto_buffer_size_) return;
        // 中断，由于使用的是PEEK参数，所以中断之后这个数据还有救
        continue;
      default: Error("recvfrom failed: {}", strerror(errno)); return;
    }
  }
  // 啧，坑真多……只有缓冲区比size大才可能表明接收的是整个包而不是半个。
  // 如果没有保证尽量接收，即auto_buffer_size_=false，那么就有可能出现这种情况
  assert(size >= 0);
  assert(size < buffer_.size());  // Package corrupted
  readable_cb_(this, peer, buffer_.data(), size);
}

void UdpPeer::SendTo(const char *data, int size, InetAddr &addr) {
  assert(size > 0);
  auto *sock_addr = addr.GetSockAddr();
  for (;;) {
    int sent = ::sendto(fd_, data, size, 0, sock_addr, sizeof(*sock_addr));
    if (sent == size) {
      if (!binded_addr_) {
        binded_addr_ = true;
        addr_ = GetLocalAddr(fd_);
      }
      return;
    }
    if (sent > 0) {
      Error("Wants to send {} but sent {} actually", size, sent);
      return;
    }
    Error(strerror(errno));
    // 根据man文档可以得知以下错误。但实际上似乎没有必要关心这么多的问题。
    switch (errno) {
      case EAFNOSUPPORT:
      case ENOTSOCK:
      case EBADF:
      case ENOBUFS:
      case ENOMEM:
        // 编程错误或致命错误，无法恢复
        Fatal(strerror(errno));
        return;

      case EINTR:
        // 中断。再试试
        continue;
      case EAGAIN:
      case ECONNRESET:  // 本地发送可能会触发
      case EMSGSIZE:    // 包过大
      case ENOTCONN:    // 未连接
      case EOPNOTSUPP:  // 操作不支持
      case EPIPE:       // sigpipe
      case ENOENT:      // 本地套接字不是个文件，或者文件名为空
      case ENOTDIR:     // ???
      case EACCES:      // 无访问权限
      case EDESTADDRREQ:
      case EHOSTUNREACH:
      case EINVAL:
      case EIO:
      case EISCONN:
      case ENETDOWN:
      case ENETUNREACH:
      case ELOOP:
      case ENAMETOOLONG:
      default:
        // 直接放弃发送
        Error(strerror(errno));
        return;
    }
  }
}

void UdpPeer::SendTo(std::string_view str, InetAddr &addr) {
  SendTo(str.data(), str.size(), addr);
}

UdpPeer::~UdpPeer() {
  channel_.DisableAll();
  close(fd_);
}
