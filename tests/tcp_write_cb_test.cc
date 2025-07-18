#include <algorithm>
#include <memory>
#include <thread>

#include "buffer.h"
#include "event_loop.h"
#include "inet_addr.h"
#include "logger.h"
#include "tcp_client.h"
#include "tcp_connection.h"
#include "tcp_server.h"
#include "time_helpers.h"
EventLoop *loop_srv;
EventLoop *loop_cli;
mstime_t last_send_time = CurrentTimeMs();
const int kDataLen = 1024 * 1024 * 500;
char data[kDataLen];
bool send_large_data = false;
void WriteCb(std::shared_ptr<TcpConnection> conn) {
  // Send("a")的作用是发送一个大概率能在一次write中
  // 发送完成的数据检查可写入回调是否还能被调用。
  if (!send_large_data) conn->Send("a");
  // 随后开始发送大量的数据，检查HandleWrite的发送完成能否触发写回调
  else
    conn->Send(data, kDataLen);
  last_send_time = CurrentTimeMs();
}
void ConnCb(std::shared_ptr<TcpConnection> conn) {
  switch (conn->GetState()) {
    case TcpConnection::kConnecting:
    case TcpConnection::kEstablished: WriteCb(conn);
    case TcpConnection::kHalfShutdown:
    case TcpConnection::kDisconnected: break;
  }
}
void ReadCb(std::shared_ptr<TcpConnection> conn, Buffer *buf) {
  (void)conn;
  buf->Pop(static_cast<int>(buf->size()));
}
mstime_t TimerFunc(mstime_t) {
  if (CurrentTimeMs() - last_send_time > 1000) {
    Fatal("Send should triggered in 1s");
  } else {
    if (send_large_data == false) {
      Info("Test1 pass");
    } else {
      Info("Test2 pass");
      exit(0);
    }
    send_large_data = true;
  }
  return 1000;
}
void Thread() {
  loop_cli = new EventLoop{};
  InetAddr addr{"127.0.0.1", 9999};
  TcpClient cli{loop_cli, addr};
  cli.SetReadableCallback(ReadCb);
  loop_cli->Loop();
}
int main() {
  std::fill(data, data + kDataLen, 'b');
  loop_srv = new EventLoop{};
  InetAddr addr{9999};
  TcpServer srv{loop_srv, addr};
  srv.SetWriteCompleteCallback(WriteCb);
  srv.SetConnectionCallback(ConnCb);
  std::thread th{Thread};
  loop_srv->AddTimer(TimerFunc, 1000);
  loop_srv->Loop();
  th.join();
  return 0;
}
