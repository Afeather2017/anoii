#include <bitset>
#include <optional>
std::string_view strip(std::string_view sv) {
  while (isspace(sv.front())) sv.remove_prefix(1);
  while (isspace(sv.back())) sv.remove_suffix(1);
  return sv;
}
void tolower(std::string &s) {
  for (auto &c : s) c = ::tolower(c);
}
static std::bitset<128> GetTcharBitset() {
  std::bitset<128> tchar_bitset;
  for (auto c : "!#$%&'*+-.^_`|~") {
    tchar_bitset[c] = 1;
  }
  for (int c = 'a'; c <= 'z'; c++) {
    tchar_bitset[c] = 1;
  }
  for (int c = 'A'; c <= 'Z'; c++) {
    tchar_bitset[c] = 1;
  }
  for (int c = '0'; c <= '9'; c++) {
    tchar_bitset[c] = 1;
  }
  return tchar_bitset;
}
bool IsToken(std::string_view sv) {
  static std::bitset<128> tchars = GetTcharBitset();
  for (auto c : sv) {
    if (!tchars[c]) return false;
  }
  return true;
}

static char ParseHexDigit(char ch) {
  ch = std::tolower(ch);
  if ('0' <= ch && ch <= '9')
    return ch - '0';
  else if ('a' <= ch && ch <= 'f')
    return ch - 'a' + 10;
  else
    return -1;
}

std::optional<std::string> ParsePercentEncoding(std::string_view encoded) {
  std::string decoded;
  decoded.reserve(encoded.size());
  for (int i = 0; i < encoded.size();) {
    char ch = encoded[i];
    if (ch == '%') {
      if (encoded.size() >= i + 2) {
        char h = ParseHexDigit(encoded[i + 1]);
        char l = ParseHexDigit(encoded[i + 2]);
        if (h < 0 || l < 0) return {};
        ch = h * 16 + l;
        i += 3;
      } else {
        return {};
      }
    } else {
      i++;
    }
    decoded.push_back(ch);
  }
  return decoded;
}
