#pragma once
#ifndef MACROS_H
#define MACROS_H
#define DISALLOW_COPY(class_name)          \
  class_name(const class_name &) = delete; \
  class_name &operator=(const class_name &) = delete;
#define DISALLOW_MOVE(class_name)          \
  class_name(class_name &&) = delete; \
  class_name &operator=(class_name &&) = delete;
#define DEFAULT_COPY(class_name)          \
  class_name(const class_name &) = default; \
  class_name &operator=(const class_name &) = default;
#define DEFAULT_MOVE(class_name)          \
  class_name(class_name &&) = default; \
  class_name &operator=(class_name &&) = default;

#endif  // MACROS_H
