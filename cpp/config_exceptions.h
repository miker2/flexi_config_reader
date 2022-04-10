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

class MismatchKeyException : public std::runtime_error {
 public:
  explicit MismatchKeyException(const std::string& message) : std::runtime_error(message){};
};

class MismatchTypeException : public std::runtime_error {
 public:
  explicit MismatchTypeException(const std::string& message) : std::runtime_error(message){};
};

class UndefinedReferenceVarException : public std::runtime_error {
 public:
  explicit UndefinedReferenceVarException(const std::string& message)
      : std::runtime_error(message){};
};
}  // namespace config
