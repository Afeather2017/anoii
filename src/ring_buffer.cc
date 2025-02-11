#include <sys/uio.h>

#include <cassert>

#include "buffer.h"
size_t Buffer::size() {
  if (head_ <= tail_) {
    return tail_ - head_;
  }
  return tail_ + buf_.size() - head_;
}

void Buffer::resize(size_t new_size) {
  // increase only
  auto old = buf_.size();
  new_size++;
  assert(new_size >= old);
  buf_.resize(new_size);
  buf_.resize(buf_.capacity());
  new_size = buf_.size();
  // senario 1:
  // begin       head   tail     old          new
  // or tail == 0, we don't need any modification to head or tail
  if (head_ <= tail_ || tail_ == 0) return;
  char *begin = buf_.data();
  if (new_size - old >= tail_) {
    // senario 2:
    // begin       tail      head     old                           new
    // copy result:
    // begin                 head     old         tail              new
    // copy begin to tail could fit into old to new
    std::copy(begin, begin + tail_, begin + old);
    tail_ += old;
  } else {
    // senario 3:
    // begin                  tail      head     old     new
    // copy result:
    // begin         tail               head     old     new
    // copy begin to tail could not fit into old to new
    std::copy(begin, begin + new_size - old, begin + old);
    std::copy(begin + new_size - old, begin + tail_, begin);
    tail_ = new_size - old;
  }
  if (tail_ >= new_size) tail_ -= new_size;
}

void Buffer::push_back(char *data, size_t dsize) {
  if (size() + dsize + 1 <= buf_.size()) {
    resize(size() + dsize);
  }
  for (long long i = 0; i < dsize; i++) {
    _push_back(data[i]);
  }
}

void Buffer::push_front(char *data, size_t dsize) {
  if (size() + dsize + 1 <= buf_.size()) {
    resize(size() + dsize);
  }
  for (long long i = dsize - 1; i >= 0; i--) {
    _push_front(data[i]);
  }
}

void Buffer::rpush_front(char *data, size_t dsize) {
  if (size() + dsize + 1 <= buf_.size()) {
    resize(size() + dsize);
  }
  for (long long i = 0; i < dsize; i++) {
    _push_front(data[i]);
  }
}

void Buffer::push_back(char data) {
  if (size() + 2 <= buf_.size()) {
    resize(size() + 1);
  }
  _push_back(data);
}

void Buffer::push_front(char data) {
  if (size() + 2 <= buf_.size()) {
    resize(size() + 1);
  }
  _push_front(data);
}

void Buffer::_push_back(char data) {
  assert(buf_.size() - size() >= 2);
  buf_[tail_++] = data;
  if (tail_ >= buf_.size()) tail_ = 0;
}

void Buffer::_push_front(char data) {
  assert(buf_.size() - size() >= 2);
  if (head_ == 0) {
    buf_.back() = data;
    head_ = buf_.size() - 1;
    return;
  }
  buf_[--head_] = data;
}

void Buffer::get_front(char *data, size_t size) {
  for (size_t i = 0; i < size; i++) {
    data[i] = this->operator[](i);
  }
}

char &Buffer::operator[](ssize_t index) {
  assert(index < size());
  if (head_ <= tail_) {
    return buf_[index];
  }
  return buf_[index - (buf_.size() - head_)];
}

void Buffer::pop_front(size_t n) {
  head_ += n;
  if (head_ >= buf_.size()) head_ -= buf_.size();
}

void Buffer::pop_back(size_t n) {
  tail_ -= n;
  if (tail_ < 0) tail_ += buf_.size();
}

ssize_t Buffer::ReadFd(int fd, int *error) {
  char buf[65536];
  ssize_t ret = read(fd, buf, sizeof(buf));
  if (ret < 0) {
    *error = ret;
  }
  push_back(buf, ret);
  return ret;
}
