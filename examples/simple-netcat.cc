#include <memory>

#include "buffer.h"
#include "event_loop.h"
#include "tcp_client.h"
EventLoop loop{};
void ReadCb(std::shared_ptr<TcpConnection> conn, Buffer *buf) {
  (void)conn;
  size_t size = static_cast<size_t>(buf->ReadableBytes());
  auto result = write(STDOUT_FILENO, buf->begin(), size);
  (void)result;
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
