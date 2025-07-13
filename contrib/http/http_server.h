#pragma once
#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H
#include <functional>
#include <memory>
#include <unordered_map>
#include "inet_addr.h"
#include "tcp_server.h"
#include "http_request.h"
#include "http_response.h"
struct Context {
  Context() = default;
  static const std::shared_ptr<HttpResponse> default_response_;
  // 这里不用unique_ptr的原因是，Context转为std::any时，
  // std::any会执行一次拷贝，所以无法使用unique_ptr。
  std::shared_ptr<HttpRequest> request_ = std::make_shared<HttpRequest>();
  std::shared_ptr<HttpResponse> response_ = default_response_;
};
class HttpServer final {
 public:
  using Header = std::unordered_map<std::string, std::string>;
  using MessageCallback = std::function<std::shared_ptr<HttpResponse>
                                             (const HttpRequest &)>;
  DISALLOW_COPY(HttpServer);
  DISALLOW_MOVE(HttpServer);
  HttpServer(EventLoop *loop, const InetAddr &addr);
  void AddRouter(std::string router, MessageCallback msg_cb);
  void AddStaticFile(std::string pathname, std::string router = "");
  void SetEstablishCallback(std::function<void(std::shared_ptr<TcpConnection>)> cb) {
    establish_cb_ = std::move(cb);
  }
  void SetDisconnectCallback(std::function<void(std::shared_ptr<TcpConnection>)> cb) {
    disconnect_cb_ = std::move(cb);
  }
 private:
  void ReadCb(std::shared_ptr<TcpConnection> conn, Buffer *buf);
  void ConnCb(std::shared_ptr<TcpConnection> conn);
  void WriteCb(std::shared_ptr<TcpConnection> conn);
  static std::shared_ptr<HttpResponse> ReadFileAsResponse(const std::string &,
                                                          const HttpRequest &);
  EventLoop *loop_;
  InetAddr addr_;
  TcpServer srv_;
  std::unordered_map<std::string, MessageCallback> routers_;
  std::function<void(std::shared_ptr<TcpConnection>)> establish_cb_;
  std::function<void(std::shared_ptr<TcpConnection>)> disconnect_cb_;
};
#endif // HTTP_SERVER_H
