#pragma once
#ifndef BUFFER_H
#define BUFFER_H
#include <unistd.h>

#include <cassert>
#include <string_view>
#include <vector>
// 这个类的不是一个通用性的buffer，而是一个网络中用于接受与发送数据的buffer，
// 所以实现不会很复杂，但是却够用。
//
// 因为一个待发送二进制的网络包的头往往会包含校验、包大小两个字段，
// 所以就允许预留prepend_size字节。而基于字符的，可以设置prepend_size为0。
//
// 对于接收，基于字符串协议，比如http协议，往往需要找\r\n，如果是ring buffer
// 这种不连续的内存，查找比较难写对。据说lighttpd踩过这个坑。
// 而基于二进制的包，可以设置prepend_size为0，且如果遇上分割符号，此时
// 找分割符号也会很方便。
class Buffer {
 public:
  Buffer(int initial_size = 1024, int prepend_size = 8)
      : head_{prepend_size}
      , tail_{prepend_size}
      , prepend_{prepend_size}
      , data_(initial_size) {
    assert(initial_size >= prepend_size);
  }
  ssize_t ReadFd(int fd, int *err);
  void Shrink();
  void Append(const char *data, int size);
  void Append(std::string_view sv) { Append(sv.data(), sv.size()); }
  void Prepend(const char *data, int size);
  void Prepend(std::string_view sv) { Prepend(sv.data(), sv.size()); }
  void Pop(int size) {
    assert(tail_ - head_ >= size);
    head_ += size;
  }
  bool StartWith(const char *data, int size);
  ssize_t FirstOf(const char *data, int size);
  bool Contains(char ch);
  // 使得还可以连续追加写入size个字节
  void EnsureWritableSpace(int size);
  char *begin() { return data_.data() + head_; }
  int WriteableBytes() {
    return static_cast<int>(data_.size()) - (tail_ - head_);
  }
  int ReadableBytes() { return tail_ - head_; }
  bool Empty() { return head_ == tail_; }
  char *Data() { return data_.data(); }
  size_t size() { return tail_ - head_; }

 private:
  // 将区间[head, tail)的内容移动到index处，没有处理空间不足的情况
  void MoveTo(int index);
  int head_{};
  int tail_{};
  int prepend_{};
  std::vector<char> data_;
};
#endif  // BUFFER_H
