#include <functional>
#include <memory>
#include <string_view>
#include <unordered_map>

#include "event_loop.h"
#include "http_server.h"
#include "http_utils.h"
#include "logger.h"
#include "tcp_client.h"
#include "tcp_connection.h"
#include "time_helpers.h"
class PostGetTest {
 public:
  std::shared_ptr<HttpResponse> Action(const HttpRequest &req) {
    auto id_iter = req.headers_.find("action-id");
    auto resp = std::make_shared<HttpResponse>();
    if (id_iter == req.headers_.end()) {
      resp->status_code_ = StatusCode::kBadRequest;
      resp->buf_.Append("missing action-id");
      return resp;
    }
    resp->status_code_ = StatusCode::kSuccess;
    resp->buf_.Append(GetNowTimeString() + " clicked " + id_iter->second);
    return resp;
  }
  std::shared_ptr<HttpResponse> Pass(const HttpRequest &) {
    test_completed_ = true;
    auto resp = std::make_shared<HttpResponse>();
    resp->status_code_ = StatusCode::kSuccess;
    return resp;
  }
  PostGetTest() {
    Info("Constructed");
    srv_.SetEstablishCallback(
        std::bind(&PostGetTest::OnEstablished, this, std::placeholders::_1));
    srv_.SetDisconnectCallback(
        std::bind(&PostGetTest::OnDisconnect, this, std::placeholders::_1));
    Info("HTML_FILEPATH: {}", HTML_FILEPATH);
    srv_.AddStaticFile(HTML_FILEPATH, "/");
    srv_.AddStaticFile(HTML_FILEPATH);
    srv_.AddRouter(
        "/action",
        std::bind(&PostGetTest::Action, this, std::placeholders::_1));
    srv_.AddRouter("/pass",
                   std::bind(&PostGetTest::Pass, this, std::placeholders::_1));
  }
  void Run() { loop_.Loop(); }

 private:
  void OnEstablished(std::shared_ptr<TcpConnection> conn) {
    if (test_completed_) {
      conn->Shutdown();
      return;
    }
    activing_conn_count_++;
  }
  void OnDisconnect(std::shared_ptr<TcpConnection> conn) {
    activing_conn_count_--;
    if (test_completed_ && activing_conn_count_ == 0) {
      loop_.Quit();
    }
  }
  InetAddr addr_{8888};
  EventLoop loop_;
  HttpServer srv_{&loop_, addr_};
  long activing_conn_count_ = 0;
  bool test_completed_ = false;
};
void HttpParseTest() {
  {
    HttpRequest req;
    Buffer buf{};
    buf.Append("laugh out aloud");
    req.Process(&buf);
    assert(req.state_ == HttpParseState::kExpectStartLine);
    buf.Append("\r\n");
    req.Process(&buf);
    assert(req.state_ == HttpParseState::kRequestIsBad);
  }

  {
    HttpRequest req;
    Buffer buf{};
    std::string_view request{
        "GET /foo HTTP/1.1\r\n"
        "Host:     127.0.0.1:8888     \r\n"
        "\r\n",
    };
    for (auto c : request) {
      buf.Append(&c, 1);
      req.Process(&buf);
      assert(req.state_ != HttpParseState::kRequestIsBad);
    }
    assert(req.type_ == RequestType::kGet);
    assert(req.router_ == "/foo");
    assert(req.headers_.count("host") == 1);
    assert(req.headers_["host"] == "127.0.0.1:8888");
  }

  {
    assert(ParsePercentEncoding("%E4%BD%A0=%E5%A5%BD") == "你=好");
  }

  {
    HttpRequest req;
    Buffer buf{};
    std::unordered_map<std::string, std::string> kv{
        {"key", "value"},
        {"flag1", ""},
        {"flag2", ""},
        {"你", "好"},
    };
    std::string_view request{
        "GET /foo/bar?key=value&flag1&flag2=&%E4%BD%A0=%E5%A5%BD HTTP/1.1\r\n"
        "Host:     127.0.0.1:8888     \r\n"
        "\r\n"};
    buf.Append(request.data(), static_cast<int>(request.size()));
    req.Process(&buf);
    assert(req.router_ == "/foo/bar");
    assert(req.state_ == HttpParseState::kDone);
    assert(req.type_ == RequestType::kGet);
    assert(kv == req.arguments_);
  }

  {
    HttpRequest req;
    Buffer buf{};
    std::unordered_map<std::string, std::string> kv{
        {"key", "value"},
        {"flag1", ""},
        {"flag2", ""},
        {"你", "好"},
    };
    std::string_view request{
        "GET /foo/bar?key=value&flag1&flag2=&%E%B%A0=%E5%A5%BD HTTP/1.1\r\n"
        "Host:     127.0.0.1:8888     \r\n"
        "\r\n"};
    buf.Append(request.data(), static_cast<int>(request.size()));
    req.Process(&buf);
    assert(req.state_ == HttpParseState::kRequestIsBad);
  }
}
// TODO: 当文件不可读取时，是否会出现问题？
int main() {
  HttpParseTest();

  {
    PostGetTest t;
    t.Run();
  }
  return 0;
}
