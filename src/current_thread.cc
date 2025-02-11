#include "current_thread.h"
// thread_local __attribute__((aligned(64))) pid_t cached_tid = 0;
__attribute__((aligned(64))) pid_t cached_tid = 0;
pid_t CurrentThread::GetTid() {
  // 使用gettid的原因
  // 1. pid_t 不会特别大
  //    虽然sizeof(pid_t) == 4，
  //    但是 /proc/sys/kernel/pid_max 里给的值不是很大
  //    它放在日志里输出不是很影响
  // 2. pid 在 linux 中是递增轮回的
  //    这意味着除非短期内能够创建非常多的线程，
  //    否则短期内不会出现重复的tid。
  //    递增轮回：简单理解是，alloc_pidmap维护一个bitset，
  //              找到上一个分配出去的pid后的那个空位(调用的是find_next_offset)。
  //              如果满了对会对bitset扩容。
  // 3. pid_t可以轻易地在/proc中找到，top里也容易看到
  // 4. 0是非法值，因为第一个进程init的pid是1
  if (cached_tid == 0) {
    cached_tid = gettid();
  }
  return cached_tid;
}
