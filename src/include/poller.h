#pragma once
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>
class Channel;
class EventLoop;
class Poller {
 public:
  Poller(EventLoop* loop) : loop_{loop} {}
  virtual ~Poller() = default;
  // 注意，此处必须先找出所有的活动事件，填入channels之后，再handle事件。
  // 在handle事件的过程中，可能会增加channel。
  // 如果边遍历channels边handle事件，那么有可能会无限增加channel的长度！
  virtual void PollUntil(int timeout_ms, std::vector<Channel*>* channels) = 0;
  virtual bool HasChannel(Channel*) const = 0;
  virtual void UpdateChannel(Channel*) = 0;
  // 如果成功删除，返回true。
  // 如果不存在，返回false。
  virtual bool RemoveChannel(Channel*) = 0;
  static std::unique_ptr<Poller> PollerFactory(EventLoop* loop,
                                               const char* poll = "default",
                                               size_t size_hint = 0);

 protected:
  std::unordered_map<int, Channel*> fd_channels_;
  EventLoop* loop_;
};
