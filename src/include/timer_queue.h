#pragma once
#ifndef TIMER_QUEUE_H
#define TIMER_QUEUE_H
#include <functional>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "definitions.h"
#include "macros.h"
class EventLoop;
class Timer;
class TimerQueue {
 public:
  DISALLOW_COPY(TimerQueue);
  TimerQueue(EventLoop* loop);
  // 添加一个timer，在调用后start毫秒后第一次调用。
  // std::function<mstime_t(mstime_t current)> 为 NULL,
  // 或者返回值 <= 0，表示定时器可以马上删除，
  // 大于0则表示多少毫秒后才再次调用Timer。
  // current表示此次开始处理所有激活的Timer的时间
  TimerId AddTimer(std::function<mstime_t(mstime_t)>&& cb, mstime_t start);
  mstime_t SleepTime();
  void DoExpired();
  void Cancel(TimerId id);
  ~TimerQueue();

 private:
  void AddTimerInLoop(mstime_t when, Timer* timer);
  std::set<std::pair<mstime_t, Timer*>> timers_;
  std::unordered_map<TimerId, Timer*> in_use_;
  std::vector<std::pair<mstime_t, Timer*>> expired_;
  EventLoop* loop_;
  TimerId next_timer_id_;
};
#endif  // TIMER_QUEUE_H
