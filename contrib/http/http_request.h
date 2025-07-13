#pragma once
#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H
#include "macros.h"
#include <string>
#include <string_view>
#include <unordered_map>
#include "buffer.h"
// Ref: https://httpwg.org/specs/rfc9112.html
enum class RequestType {
  // 只需要这两个就够了
  kUnknow,
  kGet,
  kPost,
};
enum class HttpParseState {
  kExpectStartLine,
  kExpectHeaderLine,
  kExpectBody,
  kRequestIsBad,
  kDone
};
struct HttpRequest {
  DISALLOW_COPY(HttpRequest);
  HttpRequest() = default;
  void Process(Buffer *buf);
  void DbgPrint();
  void AddHeader(std::string_view key, std::string_view value);
  [[nodiscard]]
  std::string HeadersToString();
  [[nodiscard]]
  bool ParseTarget(std::string_view target);
  std::unordered_map<std::string, std::string> headers_{};
  std::unordered_map<std::string, std::string> arguments_{};
  HttpParseState state_ = HttpParseState::kExpectStartLine;
  RequestType type_ = RequestType::kUnknow;
  std::string router_;
};
#endif // HTTP_REQUEST_H
