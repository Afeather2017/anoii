#include "tcp_client.h"

#include <functional>

#include "connector.h"
#include "event_loop.h"
#include "inet_addr.h"
#include "socket.h"
#include "tcp_connection.h"
TcpClient::TcpClient(EventLoop *loop, const InetAddr &addr) {
  loop_ = loop;
  connector_ = new Connector{loop, addr};
  connector_->SetNewConnCb(std::bind(&TcpClient::NewConnection,
                                     this,
                                     std::placeholders::_1,
                                     std::placeholders::_2));
  connector_->Start();
  peer_ = addr;
}

long long TcpClient::id_ = 2e9 * 1e9;

void TcpClient::NewConnection(int fd, const InetAddr &addr) {
  loop_->AssertIfOutLoopThread();
  InetAddr local = GetLocalAddr(fd);
  conn_ = std::make_shared<TcpConnection>(loop_, fd, ++id_, local, peer_);
  conn_->SetReadableCallback(readable_cb_);
  conn_->SetWriteCompleteCallback(write_cb_);
  conn_->SetConnectionCallback(conn_cb_);
  conn_->SetCloseCallback(std::bind(&TcpClient::HandleClose, this));
  conn_->OnEstablished();
}

void TcpClient::HandleClose() {
  loop_->AssertIfOutLoopThread();
  if (retry_) {
    if (connector_) delete connector_;
    connector_ = new Connector{loop_, peer_};
    connector_->SetNewConnCb(std::bind(&TcpClient::NewConnection,
                                       this,
                                       std::placeholders::_1,
                                       std::placeholders::_2));
    connector_->Start();
  }
}
