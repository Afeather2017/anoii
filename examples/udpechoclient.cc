#include "event_loop.h"
#include "fmt/base.h"
#include "time_helpers.h"
#include "udp_peer.h"
EventLoop loop{};
InetAddr addr{"127.0.0.1", 9996};
UdpPeer cli{&loop};
void ReadCb(UdpPeer* self, InetAddr& peer, char* data, int size) {
  std::string_view str(data, size);
  fmt::println("{}", str);
}
char echo_client_buffer[1500];
mstime_t Send(mstime_t t) {
  std::string now = GetNowTimeString();
  int n =
      sprintf(echo_client_buffer, "mstime:%lld SendTime:%s", t, now.c_str());
  for (int i = n; i < 1472; i++) {
    echo_client_buffer[i] = static_cast<char>('0' + (i % 10));
  }
  cli.SendTo(echo_client_buffer, 1472, addr);
  return 1000;
}
int main() {
  cli.SetReadableCallback(ReadCb);
  loop.AddTimer(Send, 0);
  loop.Loop();
}
