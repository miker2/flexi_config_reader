#pragma once

#include <iostream>
#include <iterator>
#include <numeric>
#include <span>
#include <sstream>
#include <string>
#include <vector>

namespace flexi_cfg::utils {
/// \brief Removes all instances of any char foundi in `sep` from the beginning and end of `s`
/// \param[in] s - Input string
/// \param[in] sep - A string containing all `char`s to trim
/// \return The trimmed string
inline auto trim(std::string s, const std::string& chars = " \n\t\v\r\f") -> std::string {
  std::string str(std::move(s));
  str.erase(0, str.find_first_not_of(chars));
  str.erase(str.find_last_not_of(chars) + 1U);
  return str;
}

inline auto removeSubStr(std::string s, const std::string& sub_str) -> std::string {
  std::string str(std::move(s));
  std::size_t pos = str.find(sub_str);
  if (pos != std::string::npos) {
    str.erase(pos, sub_str.size());
  }
  return str;
}

inline auto split(const std::string& s, char delimiter = '.') -> std::vector<std::string> {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream token_stream(s);
  while (std::getline(token_stream, token, delimiter)) {
    tokens.push_back(token);
  }
  return tokens;
}

/// \brief Splits the string on the first instance of a delimiter
///
/// \param[in] s - The input string
/// \param[in] delimiter - String on which to split [default="."]
///
/// \return A tuple with the two parts of the string
inline auto splitHead(const std::string& s, char delimiter = '.')
    -> std::pair<std::string, std::string> {
  const auto split_pos = s.find(delimiter);

  const auto head = s.substr(0, split_pos);
  const auto tail = split_pos == std::string::npos ? "" : s.substr(split_pos + 1);
  return std::make_pair(head, tail);
}

inline auto splitTail(const std::string& s, char delimiter = '.')
    -> std::pair<std::string, std::string> {
  const auto split_pos = s.rfind(delimiter);

  const auto head = split_pos == std::string::npos ? "" : s.substr(0, split_pos);
  const auto tail = split_pos == std::string::npos ? s : s.substr(split_pos + 1);
  return std::make_pair(head, tail);
}

inline auto getParent(const std::string& s, char delimiter = '.') -> std::string {
  const auto pos = s.rfind(delimiter);
  return pos == std::string::npos ? s : s.substr(0, pos);
}

inline auto join(const std::vector<std::string>& keys, const std::string& delim) -> std::string {
  if (keys.empty()) {
    return {};
  }

  return std::accumulate(std::begin(keys), std::end(keys), std::string(),
                         [&delim](const std::string& x, const std::string& y) {
                           return x.empty() ? y : x + delim + y;
                         });
}

/// \brief Concatenates two names/labels with the appropriate delimiter
inline auto makeName(const std::string& n1, const std::string& n2 = "") -> std::string {
  // Check that at least one argument is valid. We could just return an empty string, but that seems
  // silly.
  if (n1.empty() && n2.empty()) {
    throw std::runtime_error("At least one argument must be non-empty");
  }

  if (n1.empty()) {
    return n2;
  }
  if (n2.empty()) {
    return n1;
  }

  return n1 + "." + n2;
}

// A generic `contains` method that works for any "iterable" type.
template <class C, class T>
auto contains(const C& v, const T& x) -> decltype(end(v), true) {
  constexpr bool has_contains = requires(const C& t, const T& x) { t.contains(x); };

  if constexpr (has_contains) {
    // Special case if the container has a 'contains' method.
    return v.contains(x);
  } else {
    return end(v) != std::find(begin(v), end(v), x);
  }
}

// Substitute all environment variables in the string until there are none left.
inline auto substituteEnvVars(const std::string& s) -> std::string {
  std::string str(s);
  while (true) {
    const auto pos = str.find("${");
    if (pos == std::string::npos) {
      break;
    }

    const auto end_pos = str.find('}', pos);
    if (end_pos == std::string::npos) {
      throw std::runtime_error("Invalid environment variable syntax");
    }

    const auto var_name = str.substr(pos + 2, end_pos - pos - 2);
    const auto var_value = std::getenv(var_name.c_str());
    if (var_value == nullptr) {
      str.replace(pos, end_pos - pos + 1, "");
    }
    else {
      str.replace(pos, end_pos - pos + 1, var_value);
    }    
  }
  return str;
}

// RAII temp variable override (handles multiple exit paths with base_dir)
template<typename T>
class ScopedOverride {
 public:
  explicit ScopedOverride(T& field) : field_(&field) {}
  void override(T value) {
    if (armed_) {
      throw std::runtime_error("Override already in progress");
    }
    orig_value_ = *field_;
    *field_ = value;
    armed_ = true;
  }
  ~ScopedOverride() {
    if (armed_) {
      *field_ = orig_value_;
    }
  }
 private:
  bool armed_{};
  T* field_{};
  T orig_value_;
};

}  // namespace flexi_cfg::utils

template <typename T>
auto operator<<(std::ostream& o, const std::vector<T>& vec) -> std::ostream& {
  std::copy(std::begin(vec), std::end(vec), std::ostream_iterator<T>(o, " "));
  return o;
}

template <typename T, std::size_t N>
auto operator<<(std::ostream& o, const std::array<T, N>& arr) -> std::ostream& {
  std::copy(std::begin(arr), std::end(arr), std::ostream_iterator<T>(o, " "));
  return o;
}

template <typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
auto operator<<(std::ostream& o, const std::span<T>& spn) -> std::ostream& {
  std::copy(std::begin(spn), std::end(spn), std::ostream_iterator<T>(o, " "));
  return o;
}
