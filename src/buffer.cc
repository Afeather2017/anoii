#include "buffer.h"

#include <sys/uio.h>

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstring>

#include "logger.h"
ssize_t Buffer::ReadFd(int fd, int *err) {
  char extra[65535];
  struct iovec vec[2];
  vec[0].iov_base = data_.data() + tail_;
  vec[0].iov_len = data_.size() - tail_;
  vec[1].iov_base = extra;
  vec[1].iov_len = sizeof(extra);
  auto ret = ::readv(fd, vec, 2);
  if (ret < 0) {
    *err = errno;
  } else if (ret <= vec[0].iov_len) {
    tail_ += ret;
  } else {
    tail_ += vec[0].iov_len;
    Append(extra, ret - vec[0].iov_len);
  }
  Trace("readed {} bytes from fd={}, now head={} tail={} err={} data.size={}",
        ret,
        fd,
        head_,
        tail_,
        *err,
        data_.size());
  return ret;
}
void Buffer::Shrink() {
  MoveTo(prepend_);
  data_.resize(tail_);
  data_.shrink_to_fit();
}
void Buffer::Append(const char *data, int size) {
  EnsureWritableSpace(size);
  std::copy(data, data + size, data_.data() + tail_);
  tail_ += size;
}
void Buffer::EnsureWritableSpace(int size) {
  if (WriteableBytes() - prepend_ < size) {
    // 整体空间不够，仅仅只扩容，此处不做内存移动
    data_.resize(size + tail_ + data_.size());
    data_.resize(data_.capacity());
  } else {
    // 整体空间够，移动
    MoveTo(prepend_);
  }
}
void Buffer::MoveTo(int index) {
  assert(index + tail_ - head_ <= static_cast<int>(data_.size()));
  if (head_ == index) {
    return;
  }
  ::memmove(data_.data() + index, data_.data() + head_, tail_ - head_);
  tail_ = index + tail_ - head_;
  head_ = index;
}
void Buffer::Prepend(const char *data, int size) {
  assert(size <= head_);
  head_ -= size;
  std::copy(data, data + size, data_.data() - head_);
}
