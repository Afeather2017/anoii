#pragma once
#include <vector>
#ifndef EPOLLER_H
#define EPOLLER_H
#include "poller.h"
class EPoller final : public Poller {
 public:
  EPoller(EventLoop *loop, size_t size_hint);
  ~EPoller();
  void PollUntil(int timeout_ms, std::vector<Channel *> *channels) override;
  bool HasChannel(Channel *) const override;
  void UpdateChannel(Channel *) override;
  bool RemoveChannel(Channel *) override;

 private:
  void EPollUpdate(int operation, Channel *ch, struct epoll_event *ev);
  void FillActivedChannels(int num_events, std::vector<Channel *> *channels);
  void UpdateChannelToEPoll(Channel *);
  const char *EPollOperationAsString(int operation);
  int epoll_fd_;
  std::vector<struct epoll_event> actived_events_;
};
#endif  // EPOLLER_H
