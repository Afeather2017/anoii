#pragma once
#ifndef LOGGER_H
#define LOGGER_H
#include <fmt/base.h>
#include <fmt/format.h>

#include "time_helpers.h"
enum class LogLevel {
  DEBUG_LEVEL,
  TRACE_LEVEL,
  INFO_LEVEL,
  WARN_LEVEL,
  ERROR_LEVEL,
  FATAL_LEVEL
};

template <typename... Args>
void LogBase(bool val,
             const char *path,
             const char *fn,
             int line,
             LogLevel level,
             fmt::format_string<Args...> format,
             Args &&...args) {
  if (!val) return;
  std::string s = fmt::format(format, args...);
  const char *p = nullptr;
  switch (level) {
    case LogLevel::DEBUG_LEVEL: p = "DBG "; break;
    case LogLevel::TRACE_LEVEL: p = "TRCE"; break;
    case LogLevel::INFO_LEVEL: p = "INFO"; break;
    case LogLevel::WARN_LEVEL: p = "WARN"; break;
    case LogLevel::ERROR_LEVEL: p = "ERR "; break;
    case LogLevel::FATAL_LEVEL: p = "FAT "; break;
  }
  // fmt::println("{}:{}:{}:{}:{} {}", p, path, fn, line, GetTime(), s);
  fmt::println("{}:{}:{}:{} {}", p, fn, line, GetNowTimeString(), s);
}

template <typename... Args>
void LogFatal(bool val,
              const char *path,
              const char *fn,
              int line,  // NOLINT
              fmt::format_string<Args...> format,
              Args &&...args) {  // NOLINT
  if (val) {
    std::string s = fmt::format(format, args...);
    fmt::println("{}:{}:{}:{} {}", path, fn, line, GetNowTimeString(), s);
    // 安卓似乎没有terminate的实现，所以改为abort了。
    // std::terminate();
    abort();
  }
}

#define Trace(...)               \
  LogBase(true,                  \
          __FILE__,              \
          __PRETTY_FUNCTION__,   \
          __LINE__,              \
          LogLevel::TRACE_LEVEL, \
          __VA_ARGS__)
#define Debug(...)               \
  LogBase(true,                  \
          __FILE__,              \
          __PRETTY_FUNCTION__,   \
          __LINE__,              \
          LogLevel::DEBUG_LEVEL, \
          __VA_ARGS__)
#define Info(...)               \
  LogBase(true,                 \
          __FILE__,             \
          __PRETTY_FUNCTION__,  \
          __LINE__,             \
          LogLevel::INFO_LEVEL, \
          __VA_ARGS__)

#define Warn(...)               \
  LogBase(true,                 \
          __FILE__,             \
          __PRETTY_FUNCTION__,  \
          __LINE__,             \
          LogLevel::WARN_LEVEL, \
          __VA_ARGS__)
#define WarnIf(val, ...)        \
  LogBase(val,                  \
          __FILE__,             \
          __PRETTY_FUNCTION__,  \
          __LINE__,             \
          LogLevel::WARN_LEVEL, \
          __VA_ARGS__)

#define Error(...)               \
  LogBase(true,                  \
          __FILE__,              \
          __PRETTY_FUNCTION__,   \
          __LINE__,              \
          LogLevel::ERROR_LEVEL, \
          __VA_ARGS__)
#define ErrorIf(val, ...)        \
  LogBase(val,                   \
          __FILE__,              \
          __PRETTY_FUNCTION__,   \
          __LINE__,              \
          LogLevel::ERROR_LEVEL, \
          __VA_ARGS__)

#define Fatal(...) \
  LogFatal(true, __FILE__, __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)
#define FatalIf(v, ...) \
  LogFatal((v), __FILE__, __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)
// FIXME:
// 对于FatalIf，ErrorIf，WarnIf这一类函数，
// 由于函数参数的调用顺序不确定，所以需要想方法解决这个问题
// 比如FatalIf((pid = fork()) < 0, "fork: {}", strerror(errno))这个调用，
// 其调用顺序可能是fork(),strerror(errno)，这种情况是符合预期的；
// 但是也可能是strerror(errno),fork()。这样可能会出现意想不到的结果。
#endif  // LOGGER_H
