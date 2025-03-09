#include "timer_queue.h"

#include <sys/time.h>

#include <cassert>
#include <cstdio>

#include "current_thread.h"
#include "event_loop.h"
#include "logger.h"
#include "time_helpers.h"
class Timer {
  Timer(std::function<mstime_t(mstime_t)> cb, TimerId id)
      : cb_{std::move(cb)}, timer_id_{id} {
    assert(id > 0);
  }
  void Cancel() {
    // 隐藏它的原因:
    // Cancel不具有幂等性，调用一次之后，第二次就不能再调用了！
    assert(timer_id_ > 0);
    // 所有Timer都是在其所属的EventLoop中调用回调cb与Cancel的，
    // 所以Cancel直接设置id无效是可行的，哪怕是某个Timer取消掉
    // 另一个Timer，或者自己取消自己
    // 整数的负数范围略大一点，所以取负数是可以的。
    timer_id_ = -timer_id_;
  }
  mstime_t Call(mstime_t mstime) {
    if (timer_id_ <= 0) return 0;
    return cb_(mstime);
  }
  std::function<mstime_t(mstime_t)> cb_;
  TimerId timer_id_;
  friend TimerQueue;
};

TimerQueue::TimerQueue(EventLoop *loop) : loop_{loop} {
  loop_->AssertIfOutLoopThread();
  next_timer_id_ = 0;
}

void TimerQueue::Cancel(TimerId id) {
  auto iter = in_use_.find(id);
  if (iter == in_use_.end()) {
    Debug("Tries to cancel timer {} that is not existed", std::abs(id));
    return;
  }
  // 在取消之后，Timer id被设置，但是不一定从in_use_中移除了。
  if (iter->second->timer_id_ <= 0) {
    Debug("Tries to cancel timer {} twice", std::abs(id));
    return;
  }
  // FIXME:
  // 如果一个timer定时在十年以后，那么取消后，真正的内存释放将会发生在十年以后
  iter->second->Cancel();
}

void TimerQueue::AddTimerInLoop(mstime_t when, Timer *timer) {
  loop_->AssertIfOutLoopThread();
  Debug("Timer {},{} added", when, fmt::ptr(timer));
  assert(timer->timer_id_ > 0);
  {
    auto ret = timers_.insert({when, timer});
    assert(ret.second == true);
  }
  {
    auto ret = in_use_.insert({timer->timer_id_, timer});
    assert(ret.second == true);
  }
}

TimerId TimerQueue::AddTimer(std::function<mstime_t(mstime_t)> &&cb,
                             mstime_t when) {
  auto id = ++next_timer_id_;
  auto timer = new Timer{std::move(cb), id};

  loop_->RunInLoop(std::bind(&TimerQueue::AddTimerInLoop, this, when, timer));
  return id;
}

TimerQueue::~TimerQueue() {
  for (auto [mstime, timer] : timers_) {
    delete timer;
  }
}

mstime_t TimerQueue::SleepTime() {
  if (timers_.size() == 0) return -1;
  struct timeval tv;
  gettimeofday(&tv, NULL);
  mstime_t temp =
      timers_.begin()->first - (tv.tv_sec * 1000 + tv.tv_usec / 1000);
  if (temp <= 0) {
    return 0;
  }
  return temp;
}

void TimerQueue::DoExpired() {
  loop_->AssertIfOutLoopThread();
  mstime_t current = CurrentTimeMs();
  expired_.clear();
  for (auto i = timers_.begin(); i != timers_.end();) {
    if (i->first > current) {
      break;
    }
    expired_.push_back(*i);
    timers_.erase(i++);
  }
  Trace("Handle {} expired timer", expired_.size());
  for (auto i : expired_) {
    mstime_t next_delay = i.second->Call(current);
    if (next_delay <= 0) {
      in_use_.erase(std::abs(i.second->timer_id_));
      delete i.second;
      continue;
    }
    timers_.insert({i.first + next_delay, i.second});
  }
  expired_.clear();
}
