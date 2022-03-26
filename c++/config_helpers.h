#pragma once

#include <string>

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
}  // namespace config::utils
