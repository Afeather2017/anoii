#pragma once
#ifndef HTTP_UTILS_H
#define HTTP_UTILS_H
#include <string>
#include <string_view>
#include <optional>
std::string_view strip(std::string_view sv);
bool IsToken(std::string_view sv);
void tolower(std::string &s);
std::optional<std::string> ParsePercentEncoding(std::string_view encoded);
#endif // HTTP_UTILS_H
