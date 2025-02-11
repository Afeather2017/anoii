#include "channel.h"

#include <poll.h>

#include "event_loop.h"
#include "logger.h"
const int Channel::kNoEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

void Channel::Handle() {
  is_handling_events_ = true;
  if (revents_ & POLLNVAL) {
    Warn("fd=%d is invalid", fd_);
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
  FatalIf(is_handling_events_,
          "fd={} channel dtor called during event handling",
          fd_);
}
