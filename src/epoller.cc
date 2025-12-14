#include "epoller.h"

#include <sys/epoll.h>
#include <sys/poll.h>
#include <unistd.h>

#include <cassert>
#include <vector>

#include "channel.h"
#include "event_loop.h"
#include "logger.h"

EPoller::EPoller(EventLoop* loop, size_t size_hint)
    : Poller{loop}, actived_events_(16) {
  epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
  if (epoll_fd_ < 0) {
    Fatal("epoll_create1 failed: {}", strerror(errno));
  }
}

void EPoller::UpdateChannel(Channel* channel) {
  // 此处channel的index用于表示是否加入了epoll中
  struct epoll_event ev;
  ev.data.ptr = channel;
  if (channel->GetIndex() < 0) {
    assert(fd_channels_.find(channel->GetFd()) == fd_channels_.end());
    ev.events = channel->GetEvents();
    EPollUpdate(EPOLL_CTL_ADD, channel, &ev);
    channel->SetIndex(1);
    fd_channels_[channel->GetFd()] = channel;
  } else {
    assert(fd_channels_.find(channel->GetFd()) != fd_channels_.end());
    assert(channel->GetIndex() == 1);
    assert(channel == fd_channels_.find(channel->GetFd())->second);
    ev.events = channel->GetEvents();
    EPollUpdate(EPOLL_CTL_MOD, channel, &ev);
  }
}

bool EPoller::RemoveChannel(Channel* channel) {
  auto iter = fd_channels_.find(channel->GetFd());
  if (iter == fd_channels_.end()) {
    return false;
  }
  assert(iter->second == channel);
  assert(!channel->HasEvents());
  assert(channel->GetIndex() == 1);
  struct epoll_event ev{};
  // 如果说某个fd被移除了，那么epoll中该fd也会被移除。
  // Linux kernel < 2.6.9, ev不能为nullptr
  EPollUpdate(EPOLL_CTL_DEL, channel, &ev);
  channel->SetIndex(-1);
  fd_channels_.erase(iter);
  return true;
}

void EPoller::EPollUpdate(int operation, Channel* ch, struct epoll_event* ev) {
  int err = epoll_ctl(epoll_fd_, operation, ch->GetFd(), ev);
  if (err == 0) {
    Trace("epoll_ctl({},{},{})",
          epoll_fd_,
          EPollOperationAsString(operation),
          ch->to_string());
    return;
  }
  assert(err == -1);
  Error("epoll_ctl({},{},{}): {}",
        epoll_fd_,
        EPollOperationAsString(operation),
        ch->to_string(),
        strerror(errno));
}

const char* EPoller::EPollOperationAsString(int operation) {
  switch (operation) {
    case EPOLL_CTL_ADD: return "ADD";
    case EPOLL_CTL_MOD: return "MOD";
    case EPOLL_CTL_DEL: return "DEL";
    default: return "Unknow";
  }
}

bool EPoller::HasChannel(Channel* ch) const {
  auto iter = fd_channels_.find(ch->GetFd());
  if (iter == fd_channels_.end()) {
    assert(ch->GetIndex() < 0);
    return false;
  } else {
    assert(ch->GetIndex() == 1);
    assert(ch == iter->second);
    return true;
  }
}

void EPoller::PollUntil(int timeout_ms, std::vector<Channel*>* channels) {
  int size = static_cast<int>(actived_events_.size());
  int num_events =
      epoll_wait(epoll_fd_, actived_events_.data(), size, timeout_ms);
  if (num_events > 0) {
    Trace("{}: {} events happend", epoll_fd_, num_events);
    FillActivedChannels(std::min(num_events, size), channels);
    if (num_events < size) {
      actived_events_.resize(num_events);
    }
  } else if (num_events == 0) {
    Trace("{}: nothing happend", epoll_fd_);
  } else {
    Error("{}: {}", epoll_fd_, strerror(errno));
  }
}

EPoller::~EPoller() { close(epoll_fd_); }

void EPoller::FillActivedChannels(int num_events,
                                  std::vector<Channel*>* channels) {
  for (int i = 0; i < num_events; i++) {
    struct epoll_event event = actived_events_[i];
    // event.data是一个union，所以不能够同时使用event.data.ptr与event.data.fd
    Channel* ch = static_cast<Channel*>(event.data.ptr);
    channels->push_back(ch);
    assert(fd_channels_.find(ch->GetFd()) != fd_channels_.end());
    assert(fd_channels_.find(ch->GetFd())->second == event.data.ptr);
    ch->SetReceivedEvents(static_cast<short>(event.events));
  }
}

// 如果是以静态库形式发布，那么这几句assert需要放在头文件中。
// 不过作为一个个人使用，每次都从源代码编译的程序，这不是个问题。
static_assert(EPOLLIN == POLLIN);
static_assert(EPOLLOUT == POLLOUT);
static_assert(EPOLLRDHUP == POLLRDHUP);
static_assert(EPOLLPRI == POLLPRI);
static_assert(EPOLLERR == POLLERR);
static_assert(EPOLLHUP == POLLHUP);
