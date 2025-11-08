#include "udp_peer.h"

#include <cassert>
#include <cstring>
#include <functional>

#include "event_loop.h"
#include "logger.h"
class TestAutoSize final {
 public:
  TestAutoSize() {
    srv_.SetReadableCallback(std::bind(&TestAutoSize::SrvReadCb,
                                       this,
                                       std::placeholders::_1,
                                       std::placeholders::_2,
                                       std::placeholders::_3,
                                       std::placeholders::_4));
    cli_.SetReadableCallback(std::bind(&TestAutoSize::CliReadCb,
                                       this,
                                       std::placeholders::_1,
                                       std::placeholders::_2,
                                       std::placeholders::_3,
                                       std::placeholders::_4));
    loop_.AddTimer(std::bind(&TestAutoSize::Timer, this, std::placeholders::_1),
                   10);
    for (unsigned i = 0; i < sizeof(buffer_); i++) {
      char j = static_cast<char>(i % (10 + 26 + 26));
      if (j < 10) {
        buffer_[i] = '0' + j;
      } else if (j - 10 < 26) {
        buffer_[i] = 'a' + j - 10;
      } else {
        buffer_[i] = 'A' + j - 10 - 26;
      }
    }
  }
  mstime_t Timer(mstime_t) {
    int size = rand() % 65536;
    Info("Send {} data", size);
    cli_.SendTo(buffer_, size, srv_addr_);
    if (send_times_++ == 100) {
      loop_.Quit();
      return 0;
    }
    return 10;
  }
  void Run() { loop_.Loop(); }

 private:
  void SrvReadCb(UdpPeer *, InetAddr &, char *data, int size) {
    assert(memcmp(buffer_, data, static_cast<size_t>(size)) == 0);
  }
  void CliReadCb(UdpPeer *, InetAddr &, char *data, int size) {
    assert(memcmp(buffer_, data, static_cast<size_t>(size)) == 0);
  }
  char buffer_[65536];
  InetAddr srv_addr_{"127.0.0.1", 9996};
  EventLoop loop_{};
  UdpPeer srv_{&loop_, srv_addr_, 0};
  UdpPeer cli_{&loop_, 0};
  int send_times_ = 0;
};
class TestFixxedSize final {
 public:
  TestFixxedSize() {
    srv_.SetReadableCallback(std::bind(&TestFixxedSize::SrvReadCb,
                                       this,
                                       std::placeholders::_1,
                                       std::placeholders::_2,
                                       std::placeholders::_3,
                                       std::placeholders::_4));
    cli_.SetReadableCallback(std::bind(&TestFixxedSize::CliReadCb,
                                       this,
                                       std::placeholders::_1,
                                       std::placeholders::_2,
                                       std::placeholders::_3,
                                       std::placeholders::_4));
    loop_.AddTimer(
        std::bind(&TestFixxedSize::Timer, this, std::placeholders::_1), 10);
    for (unsigned i = 0; i < sizeof(buffer_); i++) {
      char j = static_cast<char>(i % (10 + 26 + 26));
      if (j < 10) {
        buffer_[i] = '0' + j;
      } else if (j - 10 < 26) {
        buffer_[i] = 'a' + j - 10;
      } else {
        buffer_[i] = 'A' + j - 10 - 26;
      }
    }
  }
  mstime_t Timer(mstime_t t) {
    int size = rand() % 8000;
    Info("Send {} data", size);
    cli_.SendTo(buffer_, size, srv_addr_);
    if (send_times_++ == 100) {
      loop_.Quit();
      return 0;
    }
    return 10;
  }
  void Run() { loop_.Loop(); }

 private:
  void SrvReadCb(UdpPeer *, InetAddr &, char *data, int size) {
    assert(size <= 4000);
    assert(memcmp(buffer_, data, static_cast<size_t>(size)) == 0);
  }
  void CliReadCb(UdpPeer *, InetAddr &, char *data, int size) {
    assert(size <= 4000);
    assert(memcmp(buffer_, data, static_cast<size_t>(size)) == 0);
  }
  char buffer_[65536];
  InetAddr srv_addr_{"127.0.0.1", 9996};
  EventLoop loop_{};
  UdpPeer srv_{&loop_, srv_addr_, 4000};
  UdpPeer cli_{&loop_, 4000};
  int send_times_ = 0;
};
int main() {
  srand(static_cast<unsigned>(time(nullptr)));
  {
    TestAutoSize test1;
    test1.Run();
  }
  {
    TestFixxedSize test2;
    test2.Run();
  }
}
