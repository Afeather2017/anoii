#include <cstring>
#include <memory>

#include "epoller.h"
#include "logger.h"
#include "unix_poll.h"
std::unique_ptr<Poller> Poller::PollerFactory(EventLoop *loop,
                                              const char *poll,
                                              size_t size_hint) {
  if (strcmp(poll, "default") == 0) {
    const char *anoii_poll = std::getenv("ANOII_POLL");
    if (anoii_poll == nullptr)
      return std::make_unique<EPoller>(loop, size_hint);
    poll = anoii_poll;
  }
  if (strcmp(poll, "epoll") == 0)
    return std::make_unique<EPoller>(loop, size_hint);
  if (strcmp(poll, "poll") == 0)
    return std::make_unique<UPoll>(loop, size_hint);
  Fatal("Cannot create poll");
  return nullptr;
}
