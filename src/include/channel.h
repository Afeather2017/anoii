#pragma once
#ifndef CHANNEL_H
#define CHANNEL_H
#include <functional>
#include <string>

#include "macros.h"
class EventLoop;
// Channel不应该直接使用，应当是某个以它为成员的类来管理它的生命周期。
// 而且Channel也不拥有fd，所以它不会调用socket、open、close等管理fd的函数。
// 它只做事件分发。它完全不是线程安全的。
// 注意：Channel需要以指针的形式管理，因为以它为成员的类可能允许移动!
class Channel final {
 public:
  DISALLOW_COPY(Channel);
  Channel(EventLoop *loop, int fd) : loop_{loop}, fd_{fd} {}
  Channel(EventLoop *loop) : loop_{loop} {}
  ~Channel();
  void SetFd(int fd) { fd_ = fd; }
  void Reset(EventLoop *loop);
  void Handle();
  EventLoop *GetLoop() { return loop_; }
  void EnableRead() {
    events_ |= static_cast<short>(Channel::kReadEvent);
    Update();
  }
  void DisableRead() {
    events_ &= ~static_cast<short>(Channel::kReadEvent);
    Update();
  }
  void EnableWrite() {
    events_ |= static_cast<short>(Channel::kWriteEvent);
    Update();
  }
  void DisableWrite() {
    events_ &= ~static_cast<short>(Channel::kWriteEvent);
    Update();
  }
  bool IsWriting() { return events_ & Channel::kWriteEvent; };
  short GetEvents() { return events_; }
  void SetWriteCallback(std::function<void()> cb) { write_cb_ = std::move(cb); }
  void SetReadCallback(std::function<void()> cb) { read_cb_ = std::move(cb); }
  void SetCloseCallback(std::function<void()> cb) { close_cb_ = std::move(cb); }
  void SetErrorCallback(std::function<void(long int, int)> cb) {
    error_cb_ = std::move(cb);
  }
  void SetReceivedEvents(short revents) { revents_ = revents; }
  bool HasEvents() { return events_ != kNoEvent; }
  int GetIndex() { return index_in_poller_; }
  void SetIndex(int index) { index_in_poller_ = index; }
  int GetFd() { return fd_; }
  void DisableAll() {
    events_ = 0;
    Update();
  }
  std::string to_string() {
    char buf[1024];
    auto p = static_cast<void *>(this);
    sprintf(buf, "%p{fd=%d,events=%hx}", p, fd_, events_);
    return buf;
  }
  static const int kNoEvent;
  static const int kReadEvent;
  static const int kWriteEvent;

 private:
  DEFAULT_MOVE(Channel);  // 用以实现Reset方法
  void Update();
  std::function<void()> read_cb_{};
  std::function<void()> write_cb_{};
  std::function<void()> close_cb_{};
  std::function<void(long int, int)> error_cb_{};
  EventLoop *loop_ = nullptr;
  int fd_ = -1;
  int index_in_poller_ = -1;  // 如果是新Channel，则poller中没有记录它，设置为-1
  short events_ = 0;
  short revents_ = 0;  // Received events
  bool is_handling_events_ = false;
};
#endif  // CHANNEL_H
