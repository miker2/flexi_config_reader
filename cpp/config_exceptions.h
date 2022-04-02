#pragma once

#include <exception>

namespace config {

class InvalidTypeException : public std::runtime_error {
 public:
  explicit InvalidTypeException(const std::string& message) : std::runtime_error(message){};
};

class DuplicateKeyException : public std::runtime_error {
 public:
  explicit DuplicateKeyException(const std::string& message) : std::runtime_error(message){};
};

class InvalidStateException : public std::runtime_error {
 public:
  explicit InvalidStateException(const std::string& message) : std::runtime_error(message){};
};

class InvalidConfigException : public std::runtime_error {
 public:
  explicit InvalidConfigException(const std::string& message) : std::runtime_error(message){};
};

}  // namespace config
