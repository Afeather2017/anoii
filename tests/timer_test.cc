#include <unistd.h>

#include "event_loop.h"
#include "logger.h"
#include "poller.h"
EventLoop loop{};
// 安卓编译的时候，报错说times符号重复，所以加了前缀
static int timer_call_times = 0;
mstime_t Print(mstime_t time) {
  Info("Print triggered {}, and now is {}", timer_call_times++, time);
  auto ret = timer_call_times < 10;
  if (!ret) {
    loop.Quit();
    return 0;
  }
  return 100;
}

TimerId timer1;
TimerId timer2;
TimerId timer3;
mstime_t SelfCancel(mstime_t time) {
  Info("SelfCancel triggered at {}", time);
  loop.CancelTimer(timer1);
  return 100;
}

mstime_t Cancel1(mstime_t time) {
  Info("Cancel1 triggered at {}", time);
  return 100;
}
mstime_t Cancel2(mstime_t time) {
  Info("Cancel2 triggered at {}", time);
  loop.CancelTimer(timer2);
  return 0;
}
mstime_t RepeatedCancel(mstime_t time) {
  Info("RepeatedCancel triggered at {}", time);
  loop.CancelTimer(timer1);
  loop.CancelTimer(timer2);
  loop.CancelTimer(timer3);
  return 100;
}

int main() {
  timer1 = loop.AddTimer(SelfCancel, 100);
  timer2 = loop.AddTimer(Cancel1, 1000);
  timer3 = loop.AddTimer(Cancel2, 1500);
  loop.AddTimer(Print, 2000);
  loop.AddTimer(RepeatedCancel, 2000);
  loop.Loop();
}
