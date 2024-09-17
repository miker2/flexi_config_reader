// Copyright (c) 2024 Jo Voordeckers
// Copyright (c) 2020-2022 Dr. Colin Hirsch and Daniel Frey
// Please see LICENSE for license or visit https://github.com/taocpp/PEGTL/
// Adapted from below, quick and dirty hack to make it work
// https://github.com/taocpp/PEGTL/blob/main/include/tao/pegtl/contrib/trace.hpp

#pragma once

#include <cstddef>
#include <functional>
#include <iomanip>
#include <iostream>
#include <stack>
#include <string_view>
#include <tao/pegtl/apply_mode.hpp>
#include <tao/pegtl/config.hpp>
#include <tao/pegtl/contrib/state_control.hpp>
#include <tao/pegtl/demangle.hpp>
#include <tao/pegtl/normal.hpp>
#include <tao/pegtl/nothing.hpp>
#include <tao/pegtl/parse.hpp>
#include <tao/pegtl/position.hpp>
#include <tao/pegtl/rewind_mode.hpp>
#include <tuple>

namespace peg = TAO_PEGTL_NAMESPACE;

namespace flexi_cfg::config::internal::peg_extensions {

using namespace peg;

template <bool HideInternal = true, bool UseColor = true, std::size_t IndentIncrement = 2,
          std::size_t InitialIndent = 8>
struct tracer_traits {
  template <typename Rule>
  static constexpr bool enable = !HideInternal || normal<Rule>::enable;

  static constexpr std::size_t initial_indent = InitialIndent;
  static constexpr std::size_t indent_increment = IndentIncrement;

  //  static constexpr std::string_view ansi_reset = UseColor ? "\033[m" : "";
  static constexpr std::string_view ansi_rule = UseColor ? "\033[38;5;30m" : "";
  static constexpr std::string_view ansi_hide = UseColor ? "\033[48;5;0m\033[38;5;102m" : "";

  static constexpr std::string_view ansi_position = UseColor ? "\033[38;5;32m" : "";
  static constexpr std::string_view ansi_success = UseColor ? "\033[48;5;118m\033[38;5;0m" : "";
  static constexpr std::string_view ansi_failure = UseColor ? "\033[38;5;124m" : "";
  static constexpr std::string_view ansi_raise = UseColor ? "\033[48;5;196m\033[38;5;0m" : "";
  static constexpr std::string_view ansi_unwind = UseColor ? "\033[38;5;17m" : "";
  static constexpr std::string_view ansi_apply = UseColor ? "\033[48;5;129m\033[38;5;0m" : "";
  static constexpr std::string_view ansi_print = UseColor ? "\033[48;5;0m\033[38;5;450m" : "";
};

using standard_tracer_traits = tracer_traits<true>;
using complete_tracer_traits = tracer_traits<false>;

template <typename TracerTraits>
struct tracer {
  const std::ios_base::fmtflags m_flags;
  std::size_t m_count = 0;
  std::vector<std::size_t> m_stack;
  peg::position m_position;
  std::string m_line;
  std::stack<std::function<void()>> update_line;
  std::string ansi_reset = "";

  template <typename Rule>
  static constexpr bool enable = TracerTraits::template enable<Rule>;

  template <typename ParseInput>
  explicit tracer(const ParseInput& in, int nested = 0)
      : m_flags(std::cerr.flags()), m_position(in.position()) {
    ansi_reset = fmt::format("\033[48;5;{}m", 232 + nested * 5);
    std::cerr << std::left;
    update_line.emplace([&]() { m_line = in.line_at(m_position); });
    update_line.top()();
    print_position();
  }

  tracer(const tracer&) = delete;
  tracer(tracer&&) = delete;

  ~tracer() { std::cerr.flags(m_flags); }

  tracer& operator=(const tracer&) = delete;
  tracer& operator=(tracer&&) = delete;

  [[nodiscard]] std::size_t indent() const noexcept {
    return TracerTraits::initial_indent + TracerTraits::indent_increment * m_stack.size();
  }

  void print_position() const {
    std::cerr << std::setw(indent()) << ' ' << TracerTraits::ansi_position << "position" << ' '
              << m_position << '\n';
    std::cerr << std::setw(indent()) << ' ' << TracerTraits::ansi_position << "input    "
              << TracerTraits::ansi_apply << "[" << ansi_reset << TracerTraits::ansi_print << m_line
              << TracerTraits::ansi_apply << "]" << ansi_reset << "\n";
  }

  void update_position(const position& p) {
    if (m_position != p) {
      m_position = p;
      update_line.top()();
      print_position();
    }
  }

  template <typename Rule, typename ParseInput, typename... States>
  void start(const ParseInput& in, States&&... /*unused*/) {
    update_line.emplace([&]() { m_line = in.line_at(m_position); });
    std::cerr << '#' << std::setw(indent() - 1) << ++m_count << TracerTraits::ansi_rule
              << demangle<Rule>() << ansi_reset << '\n';
    m_stack.push_back(m_count);
    update_position(in.position());
  }

  template <typename Rule, typename ParseInput, typename... States>
  void success(const ParseInput& in, States&&... /*unused*/) {
    const auto prev = m_stack.back();
    m_stack.pop_back();
    update_line.pop();
    std::cerr << std::setw(indent()) << ' ' << TracerTraits::ansi_success << "success"
              << ansi_reset;
    if (m_count != prev) {
      std::cerr << " #" << prev << ' ' << TracerTraits::ansi_hide << demangle<Rule>() << ansi_reset;
    }
    std::cerr << '\n';
    update_position(in.position());
  }

  template <typename Rule, typename ParseInput, typename... States>
  void failure(const ParseInput& in, States&&... /*unused*/) {
    const auto prev = m_stack.back();
    m_stack.pop_back();
    update_line.pop();
    std::cerr << std::setw(indent()) << ' ' << TracerTraits::ansi_failure << "failure" << ' '
              << in.position() << ansi_reset;
    if (m_count != prev) {
      std::cerr << " #" << prev << ' ' << TracerTraits::ansi_hide << demangle<Rule>() << ansi_reset;
    }
    std::cerr << '\n';
    update_position(in.position());
  }

  template <typename Rule, typename ParseInput, typename... States>
  void raise(const ParseInput& /*unused*/, States&&... /*unused*/) {
    std::cerr << std::setw(indent()) << ' ' << TracerTraits::ansi_raise << "raise" << ansi_reset
              << ' ' << TracerTraits::ansi_rule << demangle<Rule>() << ansi_reset << '\n';
  }

  template <typename Rule, typename ParseInput, typename... States>
  void unwind(const ParseInput& in, States&&... /*unused*/) {
    const auto prev = m_stack.back();
    m_stack.pop_back();
    update_line.pop();
    std::cerr << std::setw(indent()) << ' ' << TracerTraits::ansi_unwind << "unwind" << ansi_reset;
    if (m_count != prev) {
      std::cerr << " #" << prev << ' ' << TracerTraits::ansi_hide << demangle<Rule>() << ansi_reset;
    }
    std::cerr << '\n';
    update_position(in.position());
  }

  template <typename Rule, typename ParseInput, typename... States>
  void apply(const ParseInput& /*unused*/, States&&... /*unused*/) {
    std::cerr << std::setw(static_cast<int>(indent() - TracerTraits::indent_increment)) << ' '
              << TracerTraits::ansi_apply << "apply " << ansi_reset << TracerTraits::ansi_hide
              << demangle<Rule>() << ansi_reset << '\n';
    print_position();
  }

  template <typename Rule, typename ParseInput, typename... States>
  void apply0(const ParseInput& /*unused*/, States&&... /*unused*/) {
    std::cerr << std::setw(static_cast<int>(indent() - TracerTraits::indent_increment)) << ' '
              << TracerTraits::ansi_apply << "apply0 " << TracerTraits::ansi_hide
              << demangle<Rule>() << ansi_reset << '\n';
    print_position();
  }

  template <typename Rule, template <typename...> class Action = nothing,
            template <typename...> class Control = normal, typename ParseInput, typename... States>
  bool parse(ParseInput&& in, States&&... st) {
    return TAO_PEGTL_NAMESPACE::parse<Rule, Action, state_control<Control>::template type>(
        in, st..., *this);
  }
};

template <typename Rule, template <typename...> class Action = nothing,
          template <typename...> class Control = normal, typename ParseInput, typename... States>
bool standard_trace(ParseInput&& in, States&&... st) {
  tracer<standard_tracer_traits> tr(in);
  return tr.parse<Rule, Action, Control>(in, st...);
}

template <typename Rule, template <typename...> class Action = nothing,
          template <typename...> class Control = normal, typename ParseInput, typename... States>
bool complete_trace(ParseInput&& in, States&&... st) {
  tracer<complete_tracer_traits> tr(in);
  return tr.parse<Rule, Action, Control>(in, st...);
}

template <typename Rule, template <typename...> class Action = nothing,
          template <typename...> class Control = normal, typename Outer, typename ParseInput,
          typename ActionData, typename... States>
auto complete_trace_nested(const Outer& o, ParseInput&& in, ActionData&& out, States&&... st) {
  tracer<complete_tracer_traits> tr(in, out.all_files.size());
  try {
    return tr.parse<Rule, Action, Control>(in, out, st...);
  } catch (parse_error& e) {
    e.add_position(peg::internal::get_position(o));
    throw;
  }
}

template <typename Tracer>
struct trace : maybe_nothing {
  template <typename Rule, apply_mode A, rewind_mode M, template <typename...> class Action,
            template <typename...> class Control, typename ParseInput, typename... States>
  [[nodiscard]] static bool match(ParseInput& in, States&&... st) {
    if constexpr (sizeof...(st) == 0) {
      return TAO_PEGTL_NAMESPACE::match<Rule, A, M, Action, state_control<Control>::template type>(
          in, st..., Tracer(in));
    } else if constexpr (!std::is_same_v<
                             std::tuple_element_t<sizeof...(st) - 1, std::tuple<States...>>,
                             Tracer&>) {
      return TAO_PEGTL_NAMESPACE::match<Rule, A, M, Action, state_control<Control>::template type>(
          in, st..., Tracer(in));
    } else {
      return TAO_PEGTL_NAMESPACE::match<Rule, A, M, Action, Control>(in, st...);
    }
  }
};

using trace_standard = trace<tracer<standard_tracer_traits>>;
using trace_complete = trace<tracer<complete_tracer_traits>>;

}  // namespace flexi_cfg::config::internal::peg_extensions