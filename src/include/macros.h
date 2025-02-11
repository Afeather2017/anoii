#pragma once
#ifndef MACROS_H
#define MACROS_H
#define DISALLOW_COPY(class_name)          \
  class_name(const class_name &) = delete; \
  class_name &operator=(const class_name &) = delete;

#ifdef NDEBUG
#define ASSERT(val, message)
#else
#include <cassert>
#include <cstdio>
#define ASSERT(val, message) \
  do {                       \
    if (!(val)) {            \
      puts(message);         \
      assert(val);           \
    }                        \
  } while (0)
#endif

#endif  // MACROS_H
