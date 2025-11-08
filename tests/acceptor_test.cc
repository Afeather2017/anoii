#include "acceptor.h"

#include <unistd.h>

#include <cstring>

#include "event_loop.h"
#include "inet_addr.h"
#include "logger.h"
int main() {
  EventLoop loop{};
  InetAddr bind_addr{1234};
  Acceptor acceptor{&loop, bind_addr};
  acceptor.SetNewConnectionCallback([](int fd, InetAddr *addr) {
    Info("new connection to {}:{}", addr->GetIp(), addr->GetPort());
    const char *str = "How dare you??";
    auto result = ::write(fd, str, strlen(str));
    (void)result;
    close(fd);
  });
  loop.Loop();
  return 0;
}
