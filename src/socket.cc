#include "socket.h"

#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cerrno>
#include <csignal>

#include "inet_addr.h"
#include "logger.h"
int GetSockError(int sockfd) {
  int optval;
  socklen_t optlen = static_cast<socklen_t>(sizeof optval);
  if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
    return errno;
  } else {
    return optval;
  }
}

void IgnoreSigPipe() { signal(SIGPIPE, SIG_IGN); }

void SetTcpNoDelay(int fd) {
  int optval = 0;
  socklen_t optlen = static_cast<socklen_t>(sizeof optval);
  int err = ::setsockopt(fd,
                         IPPROTO_TCP,
                         TCP_NODELAY,
                         &optval,
                         static_cast<socklen_t>(sizeof optval));
  FatalIf(err == 0, "setsockopt failed with {} while set nodelay", err);
}

void SetTcpKeeyAlive(int fd) {
  int optval = 1;
  socklen_t optlen = static_cast<socklen_t>(sizeof optval);
  int err = ::setsockopt(fd,
                         IPPROTO_TCP,
                         TCP_NODELAY,
                         &optval,
                         static_cast<socklen_t>(sizeof optval));
  FatalIf(err == 0, "setsockopt failed with {} while set keepalive", err);
}

void SetCloseExecNonBlock(int fd) {
  // set nonblock and cloexec
  int flags = fcntl(fd, F_GETFL, 0);
  int err;
  FatalIf(flags < 0, "fcntl F_GETFL: {}", strerror(errno));
  flags |= O_NONBLOCK | O_CLOEXEC;
  err = fcntl(fd, F_SETFL, flags);
  FatalIf(err < 0, "fcntl F_SETFL: {}", strerror(errno));
}

InetAddr GetLocalAddr(int sockfd) {
  struct sockaddr_in local;
  socklen_t len = sizeof(struct sockaddr_in);
  int ret =
      ::getsockname(sockfd, reinterpret_cast<struct sockaddr*>(&local), &len);
  if (ret < 0) {
    Error("getsockname failed: {}", strerror(errno));
    return {};
  }
  return InetAddr{local};
}

InetAddr GetPeerAddr(int sockfd) {
  struct sockaddr_in peer;
  socklen_t len = sizeof(struct sockaddr_in);
  int ret =
      ::getpeername(sockfd, reinterpret_cast<struct sockaddr*>(&peer), &len);
  if (ret < 0) {
    Error("getsockname failed: {}", strerror(errno));
    return {};
  }
  return InetAddr{peer};
}
