#include <unistd.h>

#include "event_loop.h"
#include "logger.h"
#include "poller.h"
EventLoop loop{};
// 安卓编译的时候，报错说times符号重复，所以加了前缀
static int functor_call_times = 0;
void Func() {
  Info("runs {} times", functor_call_times++);
  if (functor_call_times > 5) loop.Quit();
  loop.QueueInLoop(Func);
  loop.WakeUp();
  sleep(1);
}
int main() {
  loop.RunInLoop(Func);
  loop.Loop();
}
