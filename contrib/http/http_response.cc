#include "http_response.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "fmt/format.h"

void HttpResponse::AddHeader(std::string_view key, std::string_view value) {
  headers_.emplace(key, value);
}

std::string HttpResponse::StartAndFieldToString() {
  std::stringstream ss;
  // status-line = HTTP-version SP status-code SP [ reason-phrase ]
  ss << "HTTP/1.1 " << static_cast<int>(status_code_) << " \r\n";
  for (auto &[k, v] : headers_) {
    ss << k << ": " << v << "\r\n";
  }
  ss << "\r\n";
  return ss.str();
}

FileResponse::FileResponse(const std::string &filepath) {
  // ifstream居然可以打开文件夹？？
  // 打开文件夹后.is_open() == true, .good() == true，
  // 但是读取不出来任何内容。
  if (!std::filesystem::is_regular_file(filepath)) return;
  size_t size = std::filesystem::file_size(filepath);
  // WTF？这句话能过编译？
  // 调用的是std::string::operator=(char)，可以通过Wall来解决
  // headers_["content-length"] = size;
  // 但是如果headers_["content-length"] = (char)size就不行了
  headers_["content-length"] = std::to_string(size);
  file_.open(filepath);
  if (!file_.is_open()) buf_.Append(fmt::format("Cannot open {}", filepath));
}

bool FileResponse::HasMoreDataToLoad() {
  if (!file_.is_open()) return false;
  return !file_.eof();
}

void FileResponse::LoadData() {
  if (!HasMoreDataToLoad()) return;
  char tmp[65536];
  auto readed = file_.read(tmp, sizeof(tmp)).gcount();
  if (readed > 0) buf_.Append(tmp, static_cast<int>(readed));
}
