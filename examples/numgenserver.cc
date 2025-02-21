/*
 * 一个简单的chargen服务，用以生成数字，供numgenclient校验
 */
#include <csignal>
#include <cassert>
#include <memory>

#include "event_loop.h"
#include "inet_addr.h"
#include "logger.h"
#include "tcp_connection.h"
#include "tcp_server.h"
long long written = 0;
void WriteCb(std::shared_ptr<TcpConnection> conn) {
  // static const int char_size = 127 - 33;
  // static char tmp[65536 / char_size * char_size];
  // static char charset[char_size];
  // for (int i = 0; i < char_size; i++) {
  //   charset[i] = i + 33;
  // }
  // for (int i = 0; i < sizeof(tmp); i++) {
  //   tmp[i] = charset[i % char_size];
  // }
  const int nums_could_place = 1024 * 100;  // 1000K
  const int size = nums_could_place * 20;
  static char tmp[size + 1];
  static long long val = 1e9 * 1e9;  // 长度是9 + 9 + 1 = 19
  for (int i = 0, j = 0; j < nums_could_place; j++) {
    int len = sprintf(tmp + i, " %19lld", val++);
    i += len;
  }
  Trace("About write {} bytes to {}", size, conn->GetPeer().GetAddr());
  written += size;
  for (int i = 0; i < size; i++)
    assert(tmp[i]);
  conn->Send(tmp, size);
}
void ConnCb(std::shared_ptr<TcpConnection> conn) {
  switch (conn->GetState()) {
    case TcpConnection::kConnecting: break;
    case TcpConnection::kEstablished:
      conn->Send("abcd", 4);
      written += 4;
      break;
    case TcpConnection::kHalfShutdown: break;
    case TcpConnection::kDisconnected: break;
    default: Fatal("Unknow tcp status");
  }
}
mstime_t TimerCb(mstime_t now) {
  Info("{:.3f}MB/s", written / 1024. / 1024);
  written = 0;
  return 1000;
}
int main() {
  signal(SIGPIPE, SIG_IGN);
  InetAddr addr{9997};
  EventLoop loop{};
  TcpServer srv{&loop, addr};
  srv.SetWriteCompleteCallback(WriteCb);
  srv.SetConnectionCallback(ConnCb);
  loop.AddTimer(TimerCb, 1000);
  loop.Loop();
}
