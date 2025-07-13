#pragma once
#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H
#include "buffer.h"
#include "macros.h"
#include <string_view>
#include <string>
#include <unordered_map>
#include <fstream>
enum class StatusCode {
  kSuccess = 200,
  kBadRequest = 400,
  kNotFound = 400,
  kInternalError = 500,
};
struct HttpResponse {
  DISALLOW_COPY(HttpResponse);
  HttpResponse(StatusCode status_code, std::string_view msg = ""):
    status_code_{status_code},
    buf_{static_cast<int>(msg.size()), 0} {
    buf_.Append(msg);
  }
  HttpResponse() = default;
  void AddHeader(std::string_view key, std::string_view value);
  [[nodiscard]]
  std::string StartAndFieldToString();
  // begin和size应当一起使用，表示的是缓冲区中的数据量
  const char *begin() { return buf_.begin(); }
  int size() { return buf_.size(); }
  // 从缓冲区中读取出来多少数据，就需要Pop多少数据
  void Pop(size_t size) { buf_.Pop(size); }
  // 如果缓冲区之外还有更多数据，返回true
  virtual bool HasMoreDataToLoad() { return false; }
  // 如果缓冲区之外还有更多数据，就尝试填充缓冲区。否则什么也不做
  virtual void LoadData() {}
  std::unordered_map<std::string, std::string> headers_;
  StatusCode status_code_ = StatusCode::kInternalError;
  Buffer buf_{1024, 0};
};
struct FileResponse : public HttpResponse {
  FileResponse(const std::string &filepath);
  bool HasMoreDataToLoad() override;
  void LoadData() override;
  std::ifstream file_;
};
#endif // HTTP_RESPONSE_H
