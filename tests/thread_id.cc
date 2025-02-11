#include <iostream>
#include <thread>
const std::chrono::duration<int, std::milli> one_ms(1);
void SleepForAWhile() { std::this_thread::sleep_for(100 * one_ms); }
// 某一次执行的结果
// id1: 132936168900288, id2: 132936168900288
// id1 == id2
// 所以这个线程id应该是复用的
// 也正常，毕竟std::thread的底层实现是pthread，gdb里随便打个断点就知道了

int main() {
  std::thread th1{SleepForAWhile};
  auto id1 = th1.get_id();
  th1.join();
  std::thread th2{SleepForAWhile};
  auto id2 = th2.get_id();
  th2.join();
  std::cout << "id1: " << id1 << ", id2: " << id2 << std::endl;
  std::cout << "id1 " << (id1 == id2 ? "==" : "!=") << " id2" << std::endl;
  return 0;
}
