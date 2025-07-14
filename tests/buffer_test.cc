#include "buffer.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <string>
void Test1() {
  unsigned prepend = 8;
  Buffer buf(10, static_cast<int>(prepend));
  std::string s{"abcdefg"};
  buf.Append(s);
  auto p = buf.Data();
  for (unsigned i = 0; i < s.size(); i++) {
    assert(p[i + prepend] == s[i]);
  }
  auto q = buf.Data();
  buf.Prepend("12345678", 8);
  s = "12345678abcdefg";
  assert(p == q);
  for (unsigned i = 0; i < s.size(); i++) {
    assert(p[i] == s[i]);
  }
}
int main() { Test1(); }
