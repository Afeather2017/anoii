#include <cstring>
#include <limits>
#include <memory>
#include <string_view>

#include "buffer.h"
#include "event_loop.h"
#include "tcp_client.h"
#include "tcp_connection.h"
EventLoop loop{};
bool first = true;
uint64_t last = std::numeric_limits<uint64_t>::max();
void ReadCb(std::shared_ptr<TcpConnection> conn, Buffer *buf) {
  if (first) {
    first = false;
    buf->StartWith("abcd", 4);
    buf->Pop(4);
    return;
  }
  long long readed;
  uint64_t v = 0;
  while (buf->ReadableBytes() >= 20) {
    sscanf(buf->begin(), "%lu", &v);
    if (last != std::numeric_limits<uint64_t>::max()) assert(v == last + 1);
    last = v;
    buf->Pop(20);
  }
}
void ConnCb(std::shared_ptr<TcpConnection> conn) {
  if (conn->GetState() == TcpConnection::kEstablished) {
    first = true;
    last = std::numeric_limits<uint64_t>::max();
  }
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
  cli.SetConnectionCallback(ConnCb);
  loop.Loop();
}
