#include <fmt/base.h>

#include <filesystem>
#include <string>

#include "event_loop.h"
#include "http_server.h"
#include "logger.h"
#include "socket.h"
class StaticServer final {
 public:
  StaticServer(EventLoop* loop, InetAddr addr, std::string path)
      : srv_{loop, addr} {
    std::filesystem::path base_path(path);
    if (!std::filesystem::is_directory(path)) {
      Fatal("path {} shall be a directory", path);
    }
    if (base_path.is_relative()) {
      base_path = std::filesystem::absolute(base_path);
    }

    try {
      for (const auto& entry :
           std::filesystem::recursive_directory_iterator(base_path)) {
        if (entry.is_regular_file()) {
          std::string absolute_path = entry.path().string();

          std::string relative_path =
              std::filesystem::relative(entry.path(), base_path).string();

          // 确保相对路径以正斜杠开头
          if (!relative_path.empty() && relative_path[0] != '/') {
            relative_path = "/" + relative_path;
          }

          // 添加到静态文件服务器
          srv_.AddStaticFile(absolute_path, relative_path);

          Info("Added static file: {} -> {}", absolute_path, relative_path);
        }
      }
    } catch (const std::filesystem::filesystem_error& ex) {
      Fatal("Filesystem error: {}", ex.what());
    }
  }

 private:
  HttpServer srv_;
};
int main(int argc, char** argv) {
  if (argc != 2) {
    puts("Usage: static_server PATH");
    return -1;
  }
  IgnoreSigPipe();
  EventLoop loop;
  InetAddr addr{9080};
  StaticServer srv{&loop, addr, argv[1]};
  loop.Loop();
  return 0;
}
