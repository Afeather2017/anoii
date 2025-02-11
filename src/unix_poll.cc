#include "unix_poll.h"

#include <poll.h>

#include <cassert>
#include <cerrno>
#include <cstring>
#include <vector>

#include "channel.h"
#include "event_loop.h"
#include "logger.h"
UPoll::UPoll(EventLoop *loop, size_t size_hint) : Poller{loop} {
  // 不能直接构造size。
  plfds_.reserve(size_hint);
}

void UPoll::PollUntil(int timeout_ms,
                      std::vector<Channel *> *actived_channels) {
  loop_->AssertIfOutLoopThread();
  Trace("poll with size={} timeout={}", plfds_.size(), timeout_ms);
  int num_events = ::poll(plfds_.data(), plfds_.size(), timeout_ms);
  Trace("got {} events", num_events);
  if (num_events == 0) {
    // Timeout
    Info("poll timeout");
  } else if (num_events < 0) {
    // Failure
    Error("poll error:{}", strerror(errno));
  } else {
    // 必须要先Fill，再调用回调——如果回调中增加了Channel，此时会非常危险
    FillActivedChannels(num_events, actived_channels);
  }
}

void UPoll::FillActivedChannels(
    int num_events, std::vector<Channel *> *actived_channels) const {
  for (auto &pollfd : plfds_) {
    if (num_events <= 0) {
      break;
    }
    if (pollfd.fd < 0) {
      continue;
    }
    if (pollfd.revents) {
      num_events--;
      const auto ch = fd_channels_.find(pollfd.fd);
      assert(ch != fd_channels_.cend());
      assert(ch->second->GetFd() == pollfd.fd);
      ch->second->SetReceivedEvents(pollfd.revents);
      actived_channels->push_back(ch->second);
    }
  }
}

bool UPoll::HasChannel(Channel *channel) const {
  loop_->AssertIfOutLoopThread();
  auto iter = fd_channels_.find(channel->GetFd());
  assert(channel->GetIndex() < plfds_.size());
  return iter != fd_channels_.cend();
}

void UPoll::UpdateChannel(Channel *channel) {
  loop_->AssertIfOutLoopThread();
  // 新创建的，没有加入到poller中的channel，index必须小于0
  if (channel->GetIndex() < 0) {
    assert(fd_channels_.find(channel->GetFd()) == fd_channels_.cend());
    channel->SetIndex(plfds_.size());
    struct pollfd pfd{};
    pfd.events = channel->GetEvents();
    pfd.fd = channel->GetFd();
    plfds_.push_back(pfd);
    fd_channels_[channel->GetFd()] = channel;
  } else {
    assert(fd_channels_.find(channel->GetFd()) != fd_channels_.cend());
    assert(fd_channels_[channel->GetFd()] == channel);
    assert(channel->GetIndex() >= 0);
    assert(channel->GetIndex() < plfds_.size());
    auto &pfd = plfds_[channel->GetIndex()];
    assert(-(pfd.fd + 1) == channel->GetFd() || channel->GetFd() == pfd.fd);
    pfd.events = channel->GetEvents();
    pfd.revents = 0;  // TODO(afeather):why??
    if (!channel->HasEvents()) {
      // 忽略该fd。参见poll的手册
      pfd.fd = -pfd.fd - 1;
    }
  }
}

bool UPoll::RemoveChannel(Channel *channel) {
  loop_->AssertIfOutLoopThread();
  if (channel->GetIndex() < 0) {
    return false;
  }
  assert(!channel->HasEvents());
  auto iter = fd_channels_.find(channel->GetFd());
  assert(iter != fd_channels_.cend());
  assert(iter->second == channel);
  assert(channel->GetIndex() >= 0);
  assert(channel->GetIndex() < plfds_.size());
  auto &pfd = plfds_[channel->GetIndex()];
  assert(-(pfd.fd + 1) == channel->GetFd() || channel->GetFd() == pfd.fd);
  fd_channels_.erase(iter);
  if (channel->GetIndex() + 1 < plfds_.size()) {
    int end_fd =
        plfds_.back().fd > 0 ? plfds_.back().fd : -(plfds_.back().fd + 1);
    assert(fd_channels_.count(end_fd) > 0);
    fd_channels_[end_fd]->SetIndex(channel->GetIndex());
    std::swap(plfds_.back(), plfds_[channel->GetIndex()]);
  }
  channel->SetIndex(-1);
  plfds_.pop_back();
  return true;
}
