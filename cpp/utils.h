#pragma once

#include <iostream>
#include <iterator>
#include <numeric>
#include <span>
#include <sstream>
#include <string>
#include <vector>

namespace utils {
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

  return std::accumulate(std::begin(keys), std::end(keys), std::string(),
                         [&delim](const std::string& x, const std::string& y) {
                           return x.empty() ? y : x + delim + y;
                         });
}

auto makeName(const std::string& n1, const std::string& n2 = "") -> std::string {
  // Check that at least one argument is valid. We could just return an empty string, but that seems
  // silly.
  if (n1.empty() && n2.empty()) {
    throw std::runtime_error("At least one argument must be non-empty");
  }

  if (n1.empty()) {
    return n2;
  } else if (n2.empty()) {
    return n1;
  }

  return n1 + "." + n2;
}

}  // namespace utils

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
