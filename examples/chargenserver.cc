/*
 * 一个简单的chargen服务，用以生成数字，供numgenclient校验
 */
#include <cassert>
#include <csignal>
#include <memory>

#include "event_loop.h"
#include "inet_addr.h"
#include "logger.h"
#include "tcp_connection.h"
#include "tcp_server.h"
long long written = 0;
static const int char_size = 127 - 33;
static char tmp[20000 / char_size * char_size];
static char charset[char_size];
void WriteCb(std::shared_ptr<TcpConnection> conn) {
  for (unsigned i = 0; i < sizeof(tmp); i++) {
    tmp[i] = charset[i % char_size];
  }
  written += sizeof(tmp);
  conn->Send(tmp, sizeof(tmp));
}
void ConnCb(std::shared_ptr<TcpConnection> conn) {
  switch (conn->GetState()) {
    case TcpConnection::kConnecting: break;
    case TcpConnection::kEstablished:
      conn->Send(charset, sizeof(charset));
      written += sizeof(charset);
      break;
    case TcpConnection::kHalfShutdown: break;
    case TcpConnection::kDisconnected: break;
    default: Fatal("Unknow tcp status");
  }
}
mstime_t TimerCb(mstime_t) {
  fmt::println("{:.3f}MB/s", static_cast<double>(written) / 1024. / 1024);
  written = 0;
  return 1000;
}
int main() {
  for (int i = 0; i < char_size; i++) {
    charset[i] = static_cast<char>(i + 33);
  }
  signal(SIGPIPE, SIG_IGN);
  InetAddr addr{9997};
  EventLoop loop{};
  TcpServer srv{&loop, addr};
  srv.SetWriteCompleteCallback(WriteCb);
  srv.SetConnectionCallback(ConnCb);
  loop.AddTimer(TimerCb, 1000);
  loop.Loop();
}
