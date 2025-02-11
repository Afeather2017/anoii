#include <sys/timerfd.h>
#include <unistd.h>

#include <cstdio>

#include "channel.h"
#include "event_loop.h"
#include "unix_poll.h"
int main() {
  EventLoop el = EventLoop();
  // TFD_NONBLOCK: fctl set O_NONBLOCK
  // TFD_CLOEXEC : fctl set O_CLOEXEC
  int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  struct itimerspec new_tm{};
  new_tm.it_value.tv_sec = 5;
  ::timerfd_settime(timerfd, 0, &new_tm, NULL);
  Channel channel(&el, timerfd);
  channel.SetReadCallback([&] {
    puts("time out!");
    el.Quit();
  });
  channel.EnableRead();
  el.Loop();
  close(timerfd);
}
