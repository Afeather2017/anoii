#include <poll.h>

#include <vector>

#include "poller.h"
class Channel;
class UPoll : public Poller {
 public:
  UPoll(EventLoop* loop, size_t size_hint);
  void PollUntil(int timeout_ms, std::vector<Channel*>* channels) override;
  bool HasChannel(Channel*) const override;
  void UpdateChannel(Channel*) override;
  bool RemoveChannel(Channel*) override;
  ~UPoll() override = default;

 private:
  void FillActivedChannels(int num_event,
                           std::vector<Channel*>* actived_channels) const;
  std::vector<struct pollfd> plfds_;
};
