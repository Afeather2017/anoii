/*
 * 一个简单的echo服务。不能够处理高速的数据传输
 */
#include <memory>

#include "buffer.h"
#include "event_loop.h"
#include "inet_addr.h"
#include "tcp_connection.h"
#include "tcp_server.h"
void ReadCb(std::shared_ptr<TcpConnection> conn, Buffer *buf) {
  conn->Send(buf->begin(), buf->size());
  buf->Pop(static_cast<int>(buf->size()));
}
void WriteCb(std::shared_ptr<TcpConnection>) {}
int main() {
  InetAddr addr{9998};
  EventLoop loop{};
  TcpServer srv{&loop, addr};
  srv.SetReadableCallback(ReadCb);
  srv.SetWriteCompleteCallback(WriteCb);
  loop.Loop();
}
