#pragma once

#include <fmt/format.h>

#include <exception>

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define THROW_EXCEPTION(E_TYPE, MSG_F, ...) \
  throw E_TYPE(fmt::format("{} at {}:{}\n" MSG_F, #E_TYPE, __FILE__, __LINE__, ##__VA_ARGS__));

namespace flexi_cfg::config {

class Exception : public std::runtime_error {
 public:
  explicit Exception(const std::string& message) : std::runtime_error(message), message_(message){};

  auto what() const noexcept -> const char* override { return message_.c_str(); }

  void append(std::string_view msg) { message_.append(msg); }

  void prepend(std::string_view msg) { message_.insert(0, msg); }

 protected:
  std::string message_;
};

class InvalidTypeException : public Exception {
 public:
  explicit InvalidTypeException(const std::string& message) : Exception(message){};
};

class InvalidStateException : public Exception {
 public:
  explicit InvalidStateException(const std::string& message) : Exception(message){};
};

class InvalidConfigException : public Exception {
 public:
  explicit InvalidConfigException(const std::string& message) : Exception(message){};
};

class InvalidKeyException : public Exception {
 public:
  explicit InvalidKeyException(const std::string& message) : Exception(message){};
};

class DuplicateKeyException : public Exception {
 public:
  explicit DuplicateKeyException(const std::string& message) : Exception(message){};
};

class MismatchKeyException : public Exception {
 public:
  explicit MismatchKeyException(const std::string& message) : Exception(message){};
};

class MismatchTypeException : public Exception {
 public:
  explicit MismatchTypeException(const std::string& message) : Exception(message){};
};

class UndefinedReferenceVarException : public Exception {
 public:
  explicit UndefinedReferenceVarException(const std::string& message) : Exception(message){};
};

class UndefinedProtoException : public Exception {
 public:
  explicit UndefinedProtoException(const std::string& message) : Exception(message){};
};

class CyclicReferenceException : public Exception {
 public:
  explicit CyclicReferenceException(const std::string& message) : Exception(message){};
};

}  // namespace flexi_cfg::config
