#pragma once

#include <iostream>
#include <iterator>
#include <numeric>
#include <span>
#include <string>
#include <vector>

namespace config::utils {
auto trim(std::string s, const std::string& sep = " \n\t\v\r\f") -> std::string {
  std::string str(std::move(s));
  str.erase(0, str.find_first_not_of(sep));
  str.erase(str.find_last_not_of(sep) + 1U);
  return str;
}

auto split(const std::string& s, char delimiter) -> std::vector<std::string> {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream token_stream(s);
  while (std::getline(token_stream, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}

auto join(const std::vector<std::string>& keys, const std::string& delim) -> std::string {
  if (keys.empty()) {
    return std::string();
  }

  return std::accumulate(std::next(std::begin(keys), 1), std::end(keys), keys.front(),
                         [&delim](std::string x, std::string y) { return x + delim + y; });
}
}  // namespace config::utils

template <typename T>
std::ostream& operator<<(std::ostream& o, const std::vector<T>& vec) {
  std::copy(std::begin(vec), std::end(vec), std::ostream_iterator<T>(o, " "));
  return o;
}

template <typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
std::ostream& operator<<(std::ostream& o, const std::span<T>& spn) {
  std::copy(std::begin(spn), std::end(spn), std::ostream_iterator<T>(o, " "));
  return o;
}
