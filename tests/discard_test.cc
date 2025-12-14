#include <cstdio>
#include <memory>

#include "buffer.h"
#include "event_loop.h"
#include "inet_addr.h"
#include "tcp_server.h"
void ReadCb(std::shared_ptr<TcpConnection> conn, Buffer* buf) {
  (void)conn;
  printf("readed %*s\n", (int)buf->size(), buf->begin());
  buf->Pop(static_cast<int>(buf->size()));
}
int main() {
  EventLoop loop{};
  InetAddr addr{9999};
  TcpServer srv{&loop, addr};
  srv.SetReadableCallback(ReadCb);
  loop.Loop();
  return 0;
}
