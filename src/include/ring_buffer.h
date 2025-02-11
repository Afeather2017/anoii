#pragma once
#ifndef RING_BUFFER_H
#define RING_BUFFER_H
#include <unistd.h>

#include <vector>
class Buffer final {
 public:
  Buffer(int initial_size = 1024) : buf_(initial_size + 1) {}
  Buffer(const Buffer &buf) = default;
  Buffer &operator=(const Buffer &buf) = default;
  Buffer(Buffer &&buf) = default;
  Buffer &operator=(Buffer &&buf) = default;
  size_t size();
  void resize(size_t new_size);
  bool empty() { return head_ == tail_; }
  char &front() { return buf_.front(); }
  void get_front(char *data, size_t size);
  char &back() { return buf_.back(); }
  void push_back(char *data, size_t size);

  // push "1234", 连续四次的pop_front你会按顺序得到1,2,3,4，而不是4,3,2,1
  void push_front(char *data, size_t size);

  // push "1234", 连续四次的pop_front你会按顺序得到4,3,2,1，而不是1,2,3,4
  void rpush_front(char *data, size_t size);
  void push_back(char data);
  void push_front(char data);
  void pop_front(size_t n = 1);
  void pop_back(size_t n = 1);
  ssize_t ReadFd(int fd, int *error);
  // 队列中第index个元素
  char &operator[](ssize_t index);
  char &at_arr(ssize_t index) { return buf_[index]; }

 private:
  // 为了实现简单，批量操作都是使用下列操作
  void _push_back(char data);
  void _push_front(char data);
  ssize_t head_ = 0, tail_ = 0;
  std::vector<char> buf_;
};
#endif  // RING_BUFFER_H
