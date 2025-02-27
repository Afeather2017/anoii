#include <memory>

#include "buffer.h"
#include "event_loop.h"
#include "tcp_client.h"
EventLoop loop{};
void ReadCb(std::shared_ptr<TcpConnection> conn, Buffer *buf) {
  write(STDOUT_FILENO, buf->begin(), buf->ReadableBytes());
  buf->Pop(buf->ReadableBytes());
}
int main(int argc, char **argv) {
  if (argc < 3) {
    return 0;
  }
  int port = std::atoi(argv[2]);
  InetAddr addr{argv[1], static_cast<uint16_t>(port)};
  TcpClient cli = TcpClient{&loop, addr};
  cli.SetRetry(true);
  cli.SetReadableCallback(ReadCb);
  loop.Loop();
}
