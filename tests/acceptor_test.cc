#include "acceptor.h"

#include <unistd.h>

#include <cstring>

#include "event_loop.h"
#include "inet_addr.h"
#include "logger.h"
int main() {
  EventLoop loop{};
  InetAddr addr{1234};
  Acceptor acceptor{&loop, addr};
  acceptor.SetNewConnectionCallback([](int fd, InetAddr *addr) {
    Info("new connection to {}:{}", addr->GetIp(), addr->GetPort());
    const char *str = "How dare you??";
    ::write(fd, str, strlen(str));
    close(fd);
  });
  loop.Loop();
  return 0;
}
