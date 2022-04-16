#pragma once

#include <fmt/color.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include <cstdint>
#include <magic_enum.hpp>
#include <string_view>

namespace logger {

enum class Severity : uint8_t { TRACE = 0, DEBUG, INFO, WARN, ERROR, CRITICAL };

class Logger {
 public:
  Logger() = default;

  static auto instance() -> Logger& {
    static Logger instance_s;
    return instance_s;
  }

  void setLevel(Severity level) { log_level_ = level; }

  template <typename... Args>
  void log(Severity level, std::string_view msg_f, Args&&... args) {
    if (level >= log_level_) {
      // NOTE: The clear format sequence shouldn't be necessary, but appears to be.
      fmt::print(fg_color_.at(level), "[{}] {}\x1b[0m\n", level,
                 fmt::vformat(msg_f, fmt::make_format_args(std::forward<Args>(args)...)));
    }
  }

 private:
  Severity log_level_{Severity::TRACE};

  const std::map<Severity, fmt::text_style> fg_color_ = {
      {Severity::TRACE, fmt::fg(fmt::color::magenta)},
      {Severity::DEBUG, fmt::fg(fmt::color::cyan)},
      {Severity::INFO, fmt::fg(fmt::color::green)},
      {Severity::WARN, fmt::fg(fmt::color::gold)},
      {Severity::ERROR, fmt::fg(fmt::color::red)},
      {Severity::CRITICAL,
       fmt::emphasis::bold | fmt::bg(fmt::color::red) | fmt::fg(fmt::color::white)}};

  // TODO: Add an container here for multiple messages for a type of "backtrace" functionality
};

static void setLevel(Severity lvl) { Logger::instance().setLevel(lvl); }

static void log(Severity level, std::string_view msg) { Logger::instance().log(level, msg); }

template <typename... Args>
static void trace(std::string_view msg_f, Args&&... args) {
  Logger::instance().log(Severity::TRACE, msg_f, std::forward<Args>(args)...);
}
template <typename... Args>
static void debug(std::string_view msg_f, Args&&... args) {
  Logger::instance().log(Severity::DEBUG, msg_f, std::forward<Args>(args)...);
}
template <typename... Args>
static void info(std::string_view msg_f, Args&&... args) {
  Logger::instance().log(Severity::INFO, msg_f, std::forward<Args>(args)...);
}
template <typename... Args>
static void warn(std::string_view msg_f, Args&&... args) {
  Logger::instance().log(Severity::WARN, msg_f, std::forward<Args>(args)...);
}
template <typename... Args>
static void error(std::string_view msg_f, Args&&... args) {
  Logger::instance().log(Severity::ERROR, msg_f, std::forward<Args>(args)...);
}
template <typename... Args>
static void critical(std::string_view msg_f, Args&&... args) {
  Logger::instance().log(Severity::CRITICAL, msg_f, std::forward<Args>(args)...);
}

static void trace(std::string_view msg) { Logger::instance().log(Severity::TRACE, msg); }
static void debug(std::string_view msg) { Logger::instance().log(Severity::DEBUG, msg); }
static void info(std::string_view msg) { Logger::instance().log(Severity::INFO, msg); }
static void warn(std::string_view msg) { Logger::instance().log(Severity::WARN, msg); }
static void error(std::string_view msg) { Logger::instance().log(Severity::ERROR, msg); }
static void critical(std::string_view msg) { Logger::instance().log(Severity::CRITICAL, msg); }

}  // namespace logger

template <>
struct fmt::formatter<logger::Severity> : formatter<std::string_view> {
  // parse is inherited from formatter<string_view>
  template <typename FormatContext>
  auto format(const logger::Severity& severity, FormatContext& ctx) {
    const auto severity_s = magic_enum::enum_name<logger::Severity>(severity);
    return formatter<std::string_view>::format(severity_s, ctx);
  }
};
