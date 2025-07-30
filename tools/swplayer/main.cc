#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "event_loop.h"
#include "fmt/base.h"
#include "fmt/format.h"
#include "http_server.h"
#include "logger.h"
#include "socket.h"
struct MusicInfo {
  std::filesystem::path path_;
  std::string name_;
  double lufs_ = 0.5;
};
class SWPlayerServer final {
 public:
  SWPlayerServer(EventLoop *loop,
                 InetAddr addr,
                 std::string_view search_path,
                 std::string home_page_filepath,
                 std::string js_filepath,
                 std::string icon_filepath)
      : srv_{loop, addr}, search_path_{search_path} {
    srv_.AddRouter(
        "/get_music",
        std::bind(&SWPlayerServer::GetMusic, this, std::placeholders::_1));
    srv_.AddStaticFile(home_page_filepath, "/");
    srv_.AddStaticFile(home_page_filepath, "/index.html");
    srv_.AddStaticFile(js_filepath, "/script.js");
    srv_.AddStaticFile(icon_filepath, "/favicon.ico");
    srv_.AddRouter(
        "/list_music",
        std::bind(&SWPlayerServer::ListMusic, this, std::placeholders::_1));
    srv_.AddRouter(
        "/get_lyirc",
        std::bind(&SWPlayerServer::GetLyric, this, std::placeholders::_1));
    GetAllMusicFile();
    InitPlayList();
    GetAllLufs();
    Info("Music file listed, server is ready to start");
  }

 private:
  std::shared_ptr<HttpResponse> GetLyric(const HttpRequest &req) {
    auto resp = std::make_unique<HttpResponse>();
    if (req.type_ != RequestType::kGet) {
      resp->status_code_ = StatusCode::kBadRequest;
      return resp;
    }
    return resp;
  }
  std::shared_ptr<HttpResponse> ListMusic(const HttpRequest &req) {
    auto resp = std::make_unique<HttpResponse>();
    if (req.type_ != RequestType::kGet) {
      resp->status_code_ = StatusCode::kBadRequest;
      return resp;
    }
    std::stringstream ss;
    ss << '{';
    // 依据歌单列出音乐
    int playlist_count = static_cast<int>(playlists_.size());
    for (auto &[playlist, music] : playlists_) {
      ss << '"' << playlist << "\":[";
      int count = static_cast<int>(music.size());
      for (auto &name : music) {
        count--;
        ss << fmt::format(
            "{{\"lufs\":{},\"name\":\"{}\"}}", music_[name].lufs_, name);
        if (count != 0) ss << ',';
      }
      ss << ']';
      playlist_count--;
      if (playlist_count != 0) ss << ',';
    }
    ss << '}';
    resp->buf_.Append(ss.str());
    Info("returns {} music and {} playlists", music_.size(), playlists_.size());
    resp->status_code_ = StatusCode::kSuccess;
    return resp;
  }
  std::shared_ptr<HttpResponse> GetMusic(const HttpRequest &req) {
    if (req.type_ != RequestType::kGet) {
      return std::make_unique<HttpResponse>(StatusCode::kBadRequest);
    }
    auto music_name_iter = req.arguments_.find("music-name");
    if (music_name_iter == req.arguments_.end()) {
      Info("Missing argument: music-name");
      return std::make_unique<HttpResponse>(StatusCode::kBadRequest,
                                            "Missing argument: music-name");
    }
    const std::string &name = music_name_iter->second;
    if (music_.count(name) == 0) {
      Info("Cannot found file {}", name);
      return std::make_unique<HttpResponse>(StatusCode::kBadRequest,
                                            "Could not found " + name);
    }

    std::string filepath = music_[name].path_;
    auto resp = std::make_unique<FileResponse>(filepath);
    int file_size = static_cast<int>(std::filesystem::file_size(filepath));
    resp->status_code_ = StatusCode::kSuccess;
    resp->headers_["accept-ranges"] =
        fmt::format("bytes 0-{}/{}", file_size - 1, file_size);
    resp->headers_["content-length"] = std::to_string(file_size);
    resp->headers_["content-type"] = "audio/mp3";
    resp->headers_["cache-control"] =
        "no-store, no-cache, must-revalidate, max-age=0";
    resp->headers_["pragma"] = "no-cache";
    resp->headers_["expires"] = "0";
    return resp;
  }
  void GetAllMusicFile() {
    Info("music are:");
    for (std::filesystem::directory_entry entry :
         std::filesystem::recursive_directory_iterator(search_path_)) {
      if (!entry.is_regular_file()) continue;
      auto path = entry.path();
      if (!path.has_extension()) continue;
      if (music_file_extensions_.count(path.extension()) == 0) continue;
      fmt::println("find {}", path.c_str());
      music_[path.filename()].path_ = path;
    }
  }
  void InitPlayList() {
    for (auto &[name, info] : music_) {
      std::string playlist_name = info.path_.parent_path().filename();
      playlists_[playlist_name].push_back(name);
      playlists_["所有音乐"].push_back(name);
    }
    for (auto &[playlist_name, _] : playlists_) {
      fmt::println("playlist {}", playlist_name);
    }
  }
  void GetAllLufs() {
    std::ifstream f{search_path_ / "music.info"};
    if (!f.is_open()) {
      return;
    }

    std::string line;
    std::string current_audio_name;
    std::unordered_map<std::string, std::string> block_data;
    MusicInfo tmp_info{};

    while (std::getline(f, line)) {
      if (line.empty()) {
        if (!block_data.empty() &&
            block_data.find("lufs") != block_data.end()) {
          double lufs_value = std::stod(block_data["lufs"]);
          music_[current_audio_name].lufs_ = lufs_value;
        }
        block_data.clear();
        current_audio_name.clear();
        continue;
      }

      if (current_audio_name.empty()) {
        current_audio_name = line;
      } else {
        std::string_view line_sv = line;
        size_t colon_pos = line_sv.find(':');
        if (colon_pos != std::string::npos) {
          std::string_view key = line_sv.substr(0, colon_pos);
          std::string_view value = line_sv.substr(colon_pos + 1);
          size_t start = value.find_first_not_of(" ");
          size_t end = value.find_last_not_of(" ");
          if (start != std::string::npos && end != std::string::npos) {
            value = value.substr(start, end - start + 1);
          }
          block_data.emplace(key, value);
        }
      }
    }

    if (!block_data.empty() && block_data.find("lufs") != block_data.end() &&
        !current_audio_name.empty()) {
      music_[current_audio_name].lufs_ = std::stod(block_data["lufs"]);
    }
  }
  HttpServer srv_;
  std::filesystem::path search_path_;
  // 从音频名字到音频信息的映射
  std::unordered_map<std::string, MusicInfo> music_;
  // 从歌单名字到音频名字的映射
  std::unordered_map<std::string, std::vector<std::string>> playlists_;
  const std::unordered_set<std::string> music_file_extensions_{
      ".mp3", ".ogg", ".wav", ".aac", ".flac"};
};
int main(int argc, char **argv) {
  if (argc != 5) {
    puts(
        "Usage: swplayer MUSIC_PATH HOME_PAGE_FILEPATH JS_FILEPATH "
        "ICON_FILEPATH");
    puts("  If you have music.info file in MUSIC_PATH,");
    puts("  then volume auto balance is enabled.");
    puts("  And file format example see music.info.example");
    return -1;
  }
  IgnoreSigPipe();
  EventLoop loop;
  InetAddr addr{8889};
  SWPlayerServer srv{&loop, addr, argv[1], argv[2], argv[3], argv[4]};
  loop.Loop();
  return 0;
}
