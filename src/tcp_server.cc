#include "tcp_server.h"

#include <cassert>
#include <memory>

#include "acceptor.h"
#include "event_loop.h"
#include "logger.h"
#include "tcp_connection.h"
TcpServer::TcpServer(EventLoop *loop, InetAddr &addr) : loop_{loop} {
  acceptor_ = new Acceptor{loop, addr, 4, true, true};
  // 据说某些C++编译器会让new返回nullptr而不是throw exception.
  assert(!acceptor_);
  acceptor_->SetNewConnectionCallback(std::bind(&TcpServer::NewConnection,
                                                this,
                                                std::placeholders::_1,
                                                std::placeholders::_2));
}

void TcpServer::NewConnection(int peer_fd, InetAddr *peer_addr) {
  loop_->AssertIfOutLoopThread();
  auto id = next_id_++;
  auto conn = std::make_shared<TcpConnection>(
      loop_, peer_fd, id, acceptor_->GetAddr(), *peer_addr);
  conn->SetReadableCallback(readable_cb_);
  conn->SetWriteCompleteCallback(write_cb_);
  conn->SetConnectionCallback(conn_cb_);
  conn->SetCloseCallback(
      std::bind(&TcpServer::RemoveConnection, this, std::placeholders::_1));
  conn->OnEstablished();
  id_conn_[id] = std::move(conn);
}

void TcpServer::RemoveConnection(std::shared_ptr<TcpConnection> conn) {
  Trace("tcpid={} use_count={}", conn->GetId(), conn.use_count());
  id_conn_.erase(conn->GetId());
  loop_->QueueInLoop(std::bind(&TcpConnection::DestroyConnection, conn));
}

TcpServer::~TcpServer() {
  for (auto &[id, conn] : this->id_conn_) {
    conn.reset();
    conn->GetLoop()->RunInLoop(
        std::bind(&TcpConnection::DestroyConnection, conn));
  }
  delete acceptor_;
}
