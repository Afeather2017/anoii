#include "channel.h"

#include <poll.h>

#include <cassert>
#include <functional>

#include "event_loop.h"
#include "logger.h"
const int Channel::kNoEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

void Channel::Handle() {
  is_handling_events_ = true;
  if (revents_ & POLLNVAL) {
    Warn("fd={} is invalid", fd_);
  }
  if (revents_ & (POLLIN | POLLPRI | POLLRDNORM | POLLRDBAND)) {
    if (read_cb_) read_cb_();
  }
  if (revents_ & (POLLOUT | POLLWRNORM | POLLWRBAND)) {
    if (write_cb_) write_cb_();
  }
  if (revents_ & (POLLERR | POLLNVAL)) {
    if (error_cb_) error_cb_(0, errno);
  }
  if (revents_ & POLLHUP) {  // SIGPIPE会触发
    if (close_cb_) close_cb_();
  }
  is_handling_events_ = false;
}
void Channel::Update() { loop_->UpdateChannel(this); }
Channel::~Channel() {
  assert(events_ == 0);
  assert(!is_handling_events_);
}
void Channel::Reset(EventLoop* loop) {
  assert(events_ == 0);
  assert(!is_handling_events_);
  // 如果构造函数确保了成员都初始化了，
  // 那么这种做法可以避免某些成员没有初始化到。
  // 分别设置成员的做法不太合适。
  Channel another{loop};
  *this = std::move(another);
}

std::string Channel::to_string() {
  char buf[1024];
  auto p = static_cast<void*>(this);
  int last = sprintf(buf, "%p{fd=%d,events=", p, fd_);
  if (events_ & kReadEvent) last += sprintf(buf + last, "kReadEvent|");
  if (events_ & kWriteEvent) last += sprintf(buf + last, "kWriteEvent|");
  if (buf[last - 1] == '=') last += sprintf(buf + last, "kNoEvent|");
  buf[last - 1] = '}';
  return buf;
}
