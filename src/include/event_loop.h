#pragma once
#include <sys/types.h>

#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "definitions.h"
#include "macros.h"
class Poller;
class Channel;
class TimerQueue;
using TimerId = long long;
class EventLoop {
 public:
  DISALLOW_COPY(EventLoop);
  EventLoop(const char *poll_type = "default", size_t size_hint = 0);
  ~EventLoop();
  void Loop();
  bool IsInLoopThread() const;
  inline void AssertIfOutLoopThread() {
    if (!IsInLoopThread()) {
      std::terminate();
    }
  }
  void UpdateChannel(Channel *);
  void Quit();
  void RunInLoop(const std::function<void()> &cb);
  void QueueInLoop(const std::function<void()> &cb);
  void WakeUp();
  // 添加一个timer，在调用后start毫秒后第一次调用。
  // std::function<mstime_t(mstime_t current)> 返回值 <=
  // 0，表示定时器可以马上删除， 大于0则表示多少毫秒后才再次调用Timer。
  // current表示此次开始处理所有激活的Timer的时间
  TimerId AddTimer(std::function<mstime_t(mstime_t current)>, mstime_t start);
  void CancelTimer(TimerId id);
  void RemoveChannel(Channel *);

 private:
  void InitWakeupFd();
  void HandleWakeUpRead();
  void DoPeddingFunctors();
  std::unique_ptr<Poller> poller_;
  std::vector<Channel *> actived_channels_;
  std::unique_ptr<TimerQueue> timer_queue_;

  // 供其他线程访问，所以需要上锁
  std::mutex pedding_functors_lock_;
  std::vector<std::function<void()>> pedding_functors_;
  std::vector<std::function<void()>> calling_functors_;
  bool calling_pedding_functors_ = false;

  // Event fd，用于线程唤醒
  std::unique_ptr<Channel> wakeup_channel_;
  int wakeup_fd = -1;

  const pid_t thread_id_ = -1;
  bool done_ = false;
  bool is_looping_ = false;
};
