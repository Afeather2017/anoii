#include "acceptor.h"

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cassert>
#include <cstring>

#include "event_loop.h"
#include "inet_addr.h"
#include "logger.h"
#include "socket.h"
Acceptor::Acceptor(EventLoop *loop,
                   const InetAddr &addr,
                   int backlog,
                   bool reuse_addr,
                   bool reuse_port)
    : channel_{loop}, addr_{addr} {
  Trace("loop={} {}:{} backlog={} reuse_addr={} reuse_port={}",
        (void *)loop,
        addr.GetIp(),
        addr.GetPort(),
        backlog,
        reuse_addr,
        reuse_port);
  int fd = socket(addr_.GetSockAddr()->sa_family, SOCK_STREAM, 0);
  FatalIf(fd < 0, "socket: {}", strerror(errno));
  int val = reuse_port ? 1 : 0;
  int err = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val));
  FatalIf(err < 0, "listen: {}", strerror(errno));
  val = reuse_addr ? 1 : 0;
  err = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
  FatalIf(err < 0, "listen: {}", strerror(errno));
  err = bind(fd, addr_.GetSockAddr(), sizeof(*addr_.GetSockAddr()));
  FatalIf(err < 0, "bind: {}", strerror(errno));
  err = listen(fd, backlog);
  FatalIf(err < 0, "listen: {}", strerror(errno));
  SetCloseExecNonBlock(fd);

  channel_.SetFd(fd);
  channel_.SetReadCallback(std::bind(&Acceptor::AcceptHandler, this));
  channel_.EnableRead();
}

void Acceptor::AcceptHandler() {
  channel_.GetLoop()->AssertIfOutLoopThread();
  InetAddr addr;
  socklen_t socklen = sizeof(*addr.GetSockAddr());
  int fd = accept(channel_.GetFd(), addr.GetSockAddr(), &socklen);
  SetCloseExecNonBlock(fd);
  if (fd >= 0) {
    assert(new_conn_cb_);
    new_conn_cb_(fd, &addr);
  } else {
    // TODO handle error
    Error("cannot accept: {}", strerror(errno));
  }
}
