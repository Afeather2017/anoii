#include "connector.h"

#include <memory>

#include "event_loop.h"
#include "inet_addr.h"
#include "socket.h"
#include "tcp_connection.h"
EventLoop loop{};
void NewConn(std::shared_ptr<TcpConnection> conn,
             int sockfd,
             const InetAddr &peer) {
  auto addr = GetLocalAddr(sockfd);
  conn = std::make_shared<TcpConnection>(&loop, sockfd, 0, addr, peer);
  conn->SetConnectionCallback([](std::shared_ptr<TcpConnection>) {});
  conn->OnEstablished();
  conn->Send("exit", 4);
  conn->Shutdown();
  loop.QueueInLoop([] { loop.Quit(); });
}
int main(int argc, char **argv) {
  if (argc < 3) {
    return 0;
  }
  // 1. 连接测试
  InetAddr addr{argv[1], static_cast<uint16_t>(::atoi(argv[2]))};
  Connector ctr{&loop, addr};
  std::shared_ptr<TcpConnection> conn;
  ctr.SetNewConnCb(
      [&conn](int fd, const InetAddr &peer) { NewConn(conn, fd, peer); });
  ctr.Start();
  loop.Loop();
  // 2. Connector析构后，Timer也要取消
  return 0;
}
