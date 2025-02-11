#include <cstring>
#include <memory>

#include "unix_poll.h"
std::unique_ptr<Poller> Poller::PollerFactory(EventLoop *loop,
                                              const char *poll,
                                              size_t size_hint) {
  return std::make_unique<UPoll>(loop, size_hint);
}
