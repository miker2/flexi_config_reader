#pragma once

#include <fmt/format.h>

#include <exception>

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define THROW_EXCEPTION(E_TYPE, MSG_F, ...) \
  throw E_TYPE(fmt::format("At {}:{} \n" MSG_F, __FILE__, __LINE__, ##__VA_ARGS__));

namespace config {

class InvalidTypeException : public std::runtime_error {
 public:
  explicit InvalidTypeException(const std::string& message) : std::runtime_error(message){};
};

class InvalidStateException : public std::runtime_error {
 public:
  explicit InvalidStateException(const std::string& message) : std::runtime_error(message){};
};

class InvalidConfigException : public std::runtime_error {
 public:
  explicit InvalidConfigException(const std::string& message) : std::runtime_error(message){};
};

class InvalidKeyException : public std::runtime_error {
 public:
  explicit InvalidKeyException(const std::string& message) : std::runtime_error(message){};
};

class DuplicateKeyException : public std::runtime_error {
 public:
  explicit DuplicateKeyException(const std::string& message) : std::runtime_error(message){};
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

class UndefinedProtoException : public std::runtime_error {
 public:
  explicit UndefinedProtoException(const std::string& message) : std::runtime_error(message){};
};

class CyclicReferenceException : public std::runtime_error {
 public:
  explicit CyclicReferenceException(const std::string& message) : std::runtime_error(message){};
};

}  // namespace config
