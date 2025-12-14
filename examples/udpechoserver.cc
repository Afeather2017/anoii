#include "event_loop.h"
#include "udp_peer.h"
EventLoop loop{};
InetAddr addr{9996};
void ReadCb(UdpPeer* self, InetAddr& peer, char* data, int size) {
  self->SendTo(data, size, peer);
}
int main() {
  UdpPeer srv{&loop, addr};
  srv.SetReadableCallback(ReadCb);
  loop.Loop();
}
