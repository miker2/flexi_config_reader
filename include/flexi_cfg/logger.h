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

  void setMaxHistory(std::size_t max_history) { max_history_ = max_history; }
  void clearHistory() { log_history_.clear(); }

  template <typename... Args>
  void log(Severity level, std::string_view msg_f, Args&&... args) {
    const auto msg = fmt::vformat(msg_f, fmt::make_format_args(std::forward<Args>(args)...));
    log_history_.emplace_back(level, msg);
    if (log_history_.size() > max_history_) {
      log_history_.pop_front();
    }
    if (level >= log_level_) {
      // NOTE: The clear format sequence shouldn't be necessary, but appears to be.
      fmt::print(fg_color_.at(level), "[{}] {}\x1b[0m\n", level, msg);
    }
  }

  /// \brief Prints the history of messages (ignoring log level)
  /// \param[in] clear - Flag to indicate if history should be cleared after printing
  void backtrace(bool clear = true) {
    for (const auto& msg : log_history_) {
      fmt::print(fg_color_.at(msg.first), "[{}] {}\x1b[0m\n", msg.first, msg.second);
    }
    if (clear) {
      clearHistory();
    }
  }

 private:
  Severity log_level_{Severity::INFO};

  const std::map<Severity, fmt::text_style> fg_color_ = {
      {Severity::TRACE, fmt::fg(fmt::color::magenta)},
      {Severity::DEBUG, fmt::fg(fmt::color::cyan)},
      {Severity::INFO, fmt::fg(fmt::color::green)},
      {Severity::WARN, fmt::fg(fmt::color::gold)},
      {Severity::ERROR, fmt::fg(fmt::color::red)},
      {Severity::CRITICAL,
       fmt::emphasis::bold | fmt::bg(fmt::color::red) | fmt::fg(fmt::color::white)}};

  // This container holds a history of messages to provide a type of "backtrace" functionality
  std::size_t max_history_{15};  // NOLINT
  std::deque<std::pair<Severity, std::string>> log_history_;
};

static void setLevel(Severity lvl) { Logger::instance().setLevel(lvl); }
static auto logLevel() -> Severity { return Logger::instance().logLevel(); }

static void backtrace(bool clear = true) { Logger::instance().backtrace(clear); }

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

template <>
struct fmt::formatter<flexi_cfg::logger::Severity> : formatter<std::string_view> {
  // parse is inherited from formatter<string_view>
  template <typename FormatContext>
  auto format(const flexi_cfg::logger::Severity& severity, FormatContext& ctx) {
    const auto severity_s = magic_enum::enum_name<flexi_cfg::logger::Severity>(severity);
    return formatter<std::string_view>::format(severity_s, ctx);
  }
};
