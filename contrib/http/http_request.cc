#include "http_request.h"

#include <cctype>
#include <sstream>
#include <string>
#include <string_view>

#include "http_utils.h"
#include "logger.h"

// request-target = origin-form
//                / absolute-form
//                / authority-form
//                / asterisk-form
// origin-form    = absolute-path [ "?" query ]
// absolute-form  = absolute-URI
// authority-form = uri-host ":" port
// asterisk-form  = "*"
//
// absolute-path = 1*( "/" segment )
// segment       = *pchar
// query       = *( pchar / "/" / "?" )
//
bool HttpRequest::ParseTarget(std::string_view target) {
  auto spliter = target.find_first_of('?');
  if (spliter == target.npos) {
    spliter = target.size();
  }
  // 处理router。router分为绝对路由和相对路由，比如
  // http://localhost/a.html和/a.html
  std::string_view router = target.substr(0, spliter);
  // 避免通过这个访问到操作系统中的其他文件
  if (router.find("..") != router.npos) return false;
  if (router.size() > 4 && ::memcmp(router.data(), "http", 4) == 0) {
    // 需要改为相对路径
    auto pos = router.find("//");
    if (pos == router.npos) return false;
    router.remove_prefix(pos + 2);
    pos = router.find("/");
    if (pos == router.npos) return false;
    router.remove_prefix(pos);
  }
  router_ = router;

  if (spliter == target.size()) return true;
  std::string_view query{target.data() + spliter + 1,
                         target.size() - spliter - 1};
  while (!query.empty()) {
    size_t amp_pos = query.find('&');
    std::string_view kv =
        (amp_pos != std::string_view::npos) ? query.substr(0, amp_pos) : query;

    size_t eq_pos = kv.find('=');
    std::string_view key, value;

    if (eq_pos != std::string_view::npos) {
      key = kv.substr(0, eq_pos);
      value = kv.substr(eq_pos + 1);
    } else {
      key = kv;
      value = "";
    }
    auto key_opt = ParsePercentEncoding(key);
    auto val_opt = ParsePercentEncoding(value);
    if (key_opt.has_value() && val_opt.has_value())
      arguments_[key_opt.value()] = val_opt.value();
    else
      return false;

    if (amp_pos == std::string_view::npos) break;
    query.remove_prefix(amp_pos + 1);  // 跳过'&'
  }
  return true;
}
// HTTP-message   = start-line CRLF
//                  *( field-line CRLF )
//                  CRLF
//                  [ message-body ]
void HttpRequest::Process(Buffer *buf) {
  for (;;) {
    if (state_ == HttpParseState::kExpectStartLine) {
      // start-line     = request-line / status-line
      // request-line   = method SP request-target SP HTTP-version
      // status-line    = HTTP-version SP status-code SP [ reason-phrase ]
      // SP = Single space ' '
      auto clrf_idx = buf->FirstOf("\r\n", 2);
      if (clrf_idx <= 0) return;
      int start_idx = 0;
      if (buf->StartWith("GET", 3)) {
        type_ = RequestType::kGet;
        buf->Pop(3);
        clrf_idx -= 3;
      } else if (buf->StartWith("POST", 4)) {
        type_ = RequestType::kPost;
        buf->Pop(4);
        clrf_idx -= 4;
      } else {
        state_ = HttpParseState::kRequestIsBad;
        return;
      }
      if (!buf->StartWith(" ", 1)) {
        state_ = HttpParseState::kRequestIsBad;
        return;
      }
      buf->Pop(1);
      clrf_idx--;

      auto target_end_idx = buf->FirstOf(" ", 1);
      if (target_end_idx >= clrf_idx) {
        state_ = HttpParseState::kRequestIsBad;
        return;
      }
      assert(*(buf->begin() + target_end_idx) == ' ');
      std::string_view target{buf->begin(),
                              static_cast<size_t>(target_end_idx)};
      if (!ParseTarget(target)) {
        state_ = HttpParseState::kRequestIsBad;
        return;
      }

      buf->Pop(static_cast<int>(target_end_idx) + 1);
      clrf_idx -= target_end_idx + 1;
      std::string version{buf->begin(), static_cast<size_t>(clrf_idx)};
      assert(0 == memcmp(buf->begin() + clrf_idx, "\r\n", 2));
      buf->Pop(static_cast<int>(clrf_idx) + 2);
      state_ = HttpParseState::kExpectHeaderLine;
    } else if (state_ == HttpParseState::kExpectHeaderLine) {
      // field-line     = field-name ":" OWS field-value OWS
      // field-name     = token
      // OWS            = Optional Spaces or Tabs
      // field-value    = *field-content
      // field-content  = field-vchar
      //                  [ 1*( SP / HTAB / field-vchar ) field-vchar ]
      // field-vchar    = VCHAR / obs-text
      // obs-text       = %x80-FF
      // token          = 1*tchar
      // tchar          = "!" / "#" / "$" / "%" / "&" / "'" / "*"
      //                  / "+" / "-" / "." / "^" / "_" / "`" / "|" / "~"
      //                  / DIGIT / ALPHA
      //                  ; any VCHAR, except delimiters
      auto clrf_idx = buf->FirstOf("\r\n", 2);
      if (clrf_idx < 0) return;
      if (clrf_idx == 0) {
        buf->Pop(2);
        state_ = HttpParseState::kExpectBody;
        continue;
      }
      std::string_view line{buf->begin(), static_cast<size_t>(clrf_idx)};
      auto colon_idx = line.find(":");
      if (colon_idx == line.npos) {
        state_ = HttpParseState::kRequestIsBad;
        return;
      }
      std::string field_name{line.substr(0, colon_idx)};
      tolower(field_name);
      if (!IsToken(field_name)) {
        state_ = HttpParseState::kRequestIsBad;
        return;
      }
      std::string_view field_content{line.substr(colon_idx + 1, line.size())};
      field_content = strip(field_content);
      headers_.emplace(std::move(field_name), field_content);
      buf->Pop(static_cast<int>(clrf_idx) + 2);
    } else if (state_ == HttpParseState::kExpectBody) {
      // message-body = *OCTET
      // RFC文档：
      // The presence of a message body in a request is signaled by a
      // Content-Length or Transfer-Encoding header field.
      // Request message framing is independent of method semantics.
      auto content_length_iter = headers_.find("content-length");
      if (content_length_iter != headers_.end()) {
        if (static_cast<ssize_t>(buf->size()) >=
            std::stoi(content_length_iter->second)) {
          state_ = HttpParseState::kDone;
        }
        return;
      }
      auto transfer_encoding_iter = headers_.find("transfer-encoding");
      if (transfer_encoding_iter != headers_.end()) {
        Info("{}: {}",
             transfer_encoding_iter->first,
             transfer_encoding_iter->second);
        return;
      }
      state_ = HttpParseState::kDone;
      return;
    } else {
      break;
    }
  }
}
void HttpRequest::DbgPrint() {
  std::string_view method;
  switch (type_) {
    case RequestType::kUnknow: method = "UNKNOW"; break;
    case RequestType::kGet: method = "GET"; break;
    case RequestType::kPost: method = "POST"; break;
    default: Fatal("Unknow request type");
  }
  std::stringstream ss;
  ss << "{\n";
  for (auto &[k, v] : headers_) {
    ss << "  " << k << ": " << v << '\n';
  }
  ss << '}';
  std::string args = "{\n";
  for (auto &[k, v] : arguments_) {
    args += "  " + k + '=' + v + '\n';
  }
  args += '}';
  Debug("method={} router={} arguments={} headers={}",
        method,
        router_,
        args,
        ss.str());
}
void HttpRequest::AddHeader(std::string_view key, std::string_view value) {
  headers_.emplace(key, value);
}
