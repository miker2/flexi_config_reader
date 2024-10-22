#pragma once

#include <fmt/color.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include <cstdint>
#include <deque>
#include <magic_enum.hpp>
#include <map>
#include <string_view>

namespace flexi_cfg::logger {

enum class Severity : uint8_t { TRACE = 0, DEBUG, INFO, WARN, ERROR, CRITICAL };

class Logger {
 public:
  Logger() = default;

  static auto instance() -> Logger& {
    static Logger instance_s;
    return instance_s;
  }

  void setLevel(Severity level) { log_level_ = level; }
  [[nodiscard]] auto logLevel() const -> Severity { return log_level_; }

  template <typename... Args>
  void log(Severity level, std::string_view msg_f, Args&&... args) {
    const auto msg = fmt::vformat(msg_f, fmt::make_format_args(args...));
    if (level >= log_level_) {
      // NOTE: The clear format sequence shouldn't be necessary, but appears to be.
      fmt::print(fg_color_.at(level), "[{}] {}\x1b[0m\n", level, msg);
    }
  }

 private:
  Severity log_level_{Severity::INFO};

  const std::map<Severity, fmt::text_style> fg_color_ = {
      {Severity::TRACE, fmt::fg(fmt::color::magenta)},
      {Severity::DEBUG, fmt::fg(fmt::color::cyan)},
      {Severity::INFO, fmt::fg(fmt::color::green)},
      {Severity::WARN, fmt::fg(fmt::color::gold)},
      {Severity::ERROR, fmt::fg(fmt::color::indian_red)},
      {Severity::CRITICAL, fmt::emphasis::bold | fmt::fg(fmt::color::orange_red)}};

  // This container holds a history of messages to provide a type of "backtrace" functionality
};

static void setLevel(Severity lvl) { Logger::instance().setLevel(lvl); }
static auto logLevel() -> Severity { return Logger::instance().logLevel(); }

template <typename... Args>
static void log(Severity level, std::string_view msg_f, Args&&... args) {
  Logger::instance().log(level, msg_f, std::forward<Args>(args)...);
}

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

}  // namespace flexi_cfg::logger

#define LOG(SEVERITY, MSG_F, ...)  \
  flexi_cfg::logger::log(SEVERITY, \
                         fmt::format("{}:{} - " MSG_F, __FILE__, __LINE__, ##__VA_ARGS__));

#define LOG_T(MSG_F, ...) LOG(flexi_cfg::logger::Severity::TRACE, MSG_F, ##__VA_ARGS__)
#define LOG_D(MSG_F, ...) LOG(flexi_cfg::logger::Severity::DEBUG, MSG_F, ##__VA_ARGS__)
#define LOG_I(MSG_F, ...) LOG(flexi_cfg::logger::Severity::INFO, MSG_F, ##__VA_ARGS__)
#define LOG_W(MSG_F, ...) LOG(flexi_cfg::logger::Severity::WARN, MSG_F, ##__VA_ARGS__)
#define LOG_E(MSG_F, ...) LOG(flexi_cfg::logger::Severity::ERROR, MSG_F, ##__VA_ARGS__)
#define LOG_C(MSG_F, ...) LOG(flexi_cfg::logger::Severity::CRITICAL, MSG_F, ##__VA_ARGS__)

template <>
struct fmt::formatter<flexi_cfg::logger::Severity> : formatter<std::string_view> {
  // parse is inherited from formatter<string_view>
  auto format(const flexi_cfg::logger::Severity& severity, format_context& ctx) {
    const auto severity_s = magic_enum::enum_name<flexi_cfg::logger::Severity>(severity);
    return formatter<std::string_view>::format(severity_s, ctx);
  }
};
