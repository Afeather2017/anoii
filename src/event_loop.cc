#include "event_loop.h"

#include <poll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "channel.h"
#include "logger.h"
#include "poller.h"
#include "time_helpers.h"
#include "timer_queue.h"
// 每个线程只能有一个 EventLoop，反之亦然。
// 该 thread_local 变量用于帮助实现这一点。
thread_local EventLoop *loop_of_thread = nullptr;
EventLoop::EventLoop(const char *poll_type, size_t size_hint)
    : thread_id_{::gettid()} {
  if (loop_of_thread != nullptr) {
    Fatal("This thread already allocated with an eventloop");
  }
  loop_of_thread = this;
  assert(thread_id_ > 0);
  poller_ = Poller::PollerFactory(this, poll_type, size_hint);
  InitWakeupFd();
  timer_queue_ = std::make_unique<TimerQueue>(this);
}

EventLoop::~EventLoop() {
  loop_of_thread = nullptr;
  close(wakeup_fd);
}

void EventLoop::Loop() {
  assert(!is_looping_);
  is_looping_ = true;
  AssertIfOutLoopThread();
  while (!done_) {
    actived_channels_.clear();
    poller_->PollUntil(timer_queue_->SleepTime(), &actived_channels_);
    for (auto channel : actived_channels_) {
      channel->Handle();
    }
    timer_queue_->DoExpired();
    DoPeddingFunctors();
  }
  is_looping_ = false;
}

bool EventLoop::IsInLoopThread() const { return this == loop_of_thread; }

void EventLoop::UpdateChannel(Channel *channel) {
  poller_->UpdateChannel(channel);
}

// 当前线程，则会立马调用回调
// 其他线程调用，则会唤醒当前线程，待其处理完读写事件之后则会调用回调
void EventLoop::RunInLoop(const std::function<void()> &cb) {
  if (IsInLoopThread()) {
    cb();
  } else {
    QueueInLoop(cb);
  }
}

void EventLoop::QueueInLoop(const std::function<void()> &cb) {
  {
    // 此函数可能被其他线程调用，所以上锁
    std::unique_lock guard{pedding_functors_lock_};
    pedding_functors_.push_back(cb);
  }
  // 其他线程调用，需要唤醒
  // 自己线程调用，且自己正在处理functors，也要唤醒，这样在下一次调用时可以处理该functor
  if (!IsInLoopThread() || calling_pedding_functors_) {
    WakeUp();
  }
}

void EventLoop::DoPeddingFunctors() {
  AssertIfOutLoopThread();
  calling_pedding_functors_ = true;
  {
    // 由于functor调用时，有可能会新增functor
    // 这么做主要是为了规避无限循环与死锁的问题，还顺便减少临界区
    std::lock_guard guard{pedding_functors_lock_};
    calling_functors_.swap(pedding_functors_);
  }
  for (auto &func : calling_functors_) {
    func();
  }
  // 两个vector可以复用
  calling_functors_.clear();
  calling_pedding_functors_ = false;
}

void EventLoop::WakeUp() {
  uint64_t t;
  int n = ::write(wakeup_fd, &t, sizeof(t));
  if (n != sizeof(t)) {
    Error("eventfd write returns {}, expect {}", n, sizeof(t));
  }
}

void EventLoop::InitWakeupFd() {
  wakeup_fd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
  if (wakeup_fd < 0) {
    Fatal("eventfd() create failed!");
  }
  wakeup_channel_ = std::make_unique<Channel>(this, wakeup_fd);
  wakeup_channel_->SetReadCallback(
      std::bind(&EventLoop::HandleWakeUpRead, this));
  wakeup_channel_->EnableRead();
}

void EventLoop::HandleWakeUpRead() {
  uint64_t t = 1;
  int n = ::read(wakeup_fd, &t, sizeof(t));
  if (n != sizeof(t)) {
    Error("eventfd read returns {}, expect {}", n, sizeof(t));
  }
}

void EventLoop::Quit() {
  done_ = true;
  if (!IsInLoopThread()) {
    WakeUp();
  }
}

TimerId EventLoop::AddTimer(std::function<mstime_t(mstime_t)> cb,
                            mstime_t start) {
  start += CurrentTimeMs();
  return timer_queue_->AddTimer(std::move(cb), start);
}

void EventLoop::CancelTimer(TimerId id) { timer_queue_->Cancel(id); }

void EventLoop::RemoveChannel(Channel *ch) {
  AssertIfOutLoopThread();
  poller_->RemoveChannel(ch);
}
