#include <cstring>
#include <memory>
#include <string_view>

#include "buffer.h"
#include "event_loop.h"
#include "tcp_client.h"
#include "tcp_connection.h"
EventLoop loop{};
static const int char_size = 127 - 33;
static char tmp[1024 * 20 / char_size * char_size];
static char charset[char_size];
void ReadCb(std::shared_ptr<TcpConnection>, Buffer* buf) {
  while (buf->ReadableBytes() >= char_size) {
    assert(0 == memcmp(buf->begin(), charset, sizeof(charset)));
    buf->Pop(char_size);
  }
}
int main(int argc, char** argv) {
  if (argc < 3) {
    return 0;
  }
  for (int i = 0; i < char_size; i++) {
    charset[i] = static_cast<char>(i + 33);
  }
  int port = std::atoi(argv[2]);
  InetAddr addr{argv[1], static_cast<uint16_t>(port)};
  TcpClient cli = TcpClient{&loop, addr};
  cli.SetRetry(true);
  cli.SetReadableCallback(ReadCb);
  loop.Loop();
}
