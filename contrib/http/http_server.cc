#include "http_server.h"

#include <functional>
#include <memory>
#include <string>

#include "buffer.h"
#include "event_loop.h"
#include "fmt/base.h"
#include "fmt/format.h"
#include "http_response.h"
#include "logger.h"
#include "tcp_connection.h"

const std::shared_ptr<HttpResponse> Context::default_response_ =
    std::make_shared<HttpResponse>(StatusCode::kInternalError,
                                   "Response was not initialized");
HttpServer::HttpServer(EventLoop *loop, const InetAddr &addr)
    : loop_{loop}, addr_{addr}, srv_{loop_, addr} {
  srv_.SetReadableCallback(std::bind(
      &HttpServer::ReadCb, this, std::placeholders::_1, std::placeholders::_2));
  srv_.SetConnectionCallback(
      std::bind(&HttpServer::ConnCb, this, std::placeholders::_1));
  srv_.SetWriteCompleteCallback(
      std::bind(&HttpServer::WriteCb, this, std::placeholders::_1));
}
void HttpServer::AddRouter(std::string router,
                           HttpServer::MessageCallback msg_cb) {
  routers_[router] = msg_cb;
}
void HttpServer::AddStaticFile(std::string pathname, std::string router) {
  if (router != "") {
    routers_[router] =
        std::bind(ReadFileAsResponse, pathname, std::placeholders::_1);
  } else {
    routers_['/' + pathname] =
        std::bind(ReadFileAsResponse, pathname, std::placeholders::_1);
  }
}
static std::string MakeResponseString(StatusCode code) {
  return fmt::format("HTTP/1.1 {}", static_cast<int>(code));
}
static const auto kBadRequestString =
    MakeResponseString(StatusCode::kBadRequest);
static const auto kNotFoundString = MakeResponseString(StatusCode::kNotFound);

void HttpServer::ReadCb(std::shared_ptr<TcpConnection> conn, Buffer *buf) {
  Debug("Read from {}", conn->GetPeer().GetAddr());
  // 由于conn->Send的线程安全性不足，所以必须检查调用它的线程。
  loop_->AssertIfOutLoopThread();
  Context &context = conn->GetContext<Context>();
  // Http1.1中，长链接复用是通过对头阻塞实现的，所以这里直接重新分配
  if (context.request_->state_ == HttpParseState::kDone) {
    context.request_ = std::make_shared<HttpRequest>();
  }
  auto &request = context.request_;
  request->Process(buf);
  switch (request->state_) {
    case HttpParseState::kExpectStartLine: break;
    case HttpParseState::kExpectHeaderLine:
      // 此处就可以检查是否有路由了，但是没必要
      break;
    case HttpParseState::kExpectBody: break;
    case HttpParseState::kRequestIsBad:
      conn->Send(kBadRequestString);
      conn->Shutdown();
      break;
    case HttpParseState::kDone:
      request->DbgPrint();
      auto iter = routers_.find(request->router_);
      if (iter == routers_.end()) {
        Info("No such router {}", request->router_);
        conn->Send(kNotFoundString);
        conn->Shutdown();
        break;
      }
      context.response_ = iter->second(*request);
      auto &response = context.response_;
      conn->Send(response->StartAndFieldToString());
      if (response->size()) {
        conn->Send(response->begin(), static_cast<size_t>(response->size()));
        // 因为conn->Send会确保所有数据都能够被发送，所以
        // 这里不需要继续检查实际发送的数据量。
        response->Pop(static_cast<size_t>(response->size()));
      }
      if (!response->HasMoreDataToLoad()) {
        Debug("{} send complete", conn->GetId());
      }
      break;
  }
}
void HttpServer::ConnCb(std::shared_ptr<TcpConnection> conn) {
  switch (conn->GetState()) {
    case TcpConnection::kConnecting: break;
    case TcpConnection::kEstablished:
      conn->SetContext(Context{});
      Info("Accept from {}", conn->GetPeer().GetAddr());
      if (establish_cb_) establish_cb_(conn);
      break;
    case TcpConnection::kHalfShutdown: break;
    case TcpConnection::kDisconnected:
      Info("Disconnect {}", conn->GetPeer().GetAddr());
      if (disconnect_cb_) disconnect_cb_(conn);
      break;
  }
}
void HttpServer::WriteCb(std::shared_ptr<TcpConnection> conn) {
  Context &context = conn->GetContext<Context>();
  auto &response = context.response_;
  if (response->HasMoreDataToLoad()) {
    response->LoadData();
    conn->Send(response->begin(), static_cast<size_t>(response->size()));
    // 因为conn->Send会确保所有数据都能够被发送，所以
    // 这里不需要继续检查实际发送的数据量。
    Debug("Write {}B data to {}", response->size(), conn->GetId());
    response->Pop(static_cast<size_t>(response->size()));
  } else {
    Debug("{} send complete", conn->GetId());
  }
}
std::shared_ptr<HttpResponse> HttpServer::ReadFileAsResponse(
    const std::string &path, const HttpRequest &req) {
  Info("Get {}", path);
  if (req.type_ != RequestType::kGet) {
    return std::make_shared<HttpResponse>(StatusCode::kBadRequest);
  }
  std::shared_ptr<HttpResponse> resp = std::make_shared<FileResponse>(path);
  resp->status_code_ = StatusCode::kSuccess;
  return resp;
}
