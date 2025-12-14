#include "tcp_client.h"

#include <functional>
#include <memory>

#include "connector.h"
#include "event_loop.h"
#include "inet_addr.h"
#include "socket.h"
#include "tcp_connection.h"
TcpClient::TcpClient(EventLoop* loop, const InetAddr& addr)
    : conn_cb_{DefaultConnCb}
    , readable_cb_{DefaultReadCb}
    , write_cb_{DefaultWriteCb}
    , watermark_cb_{DefaultHighWatermarkCb} {
  loop_ = loop;
  connector_ = std::make_unique<Connector>(loop, addr);
  connector_->SetNewConnCb(std::bind(&TcpClient::NewConnection,
                                     this,
                                     std::placeholders::_1,
                                     std::placeholders::_2));
  connector_->Start();
  peer_ = addr;
}

long long TcpClient::id_ = 2e9 * 1e9;

void TcpClient::NewConnection(int fd, const InetAddr& addr) {
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
    connector_ = std::make_unique<Connector>(loop_, peer_);
    connector_->SetNewConnCb(std::bind(&TcpClient::NewConnection,
                                       this,
                                       std::placeholders::_1,
                                       std::placeholders::_2));
    connector_->Start();
  }
}

TcpClient::~TcpClient() {
  // std::unique_ptr在析构的时候居然需要知道包含的成员的大小？
  // 本来可以用默认析构的，但是因为这个，只能写一个默认析构了。
}
