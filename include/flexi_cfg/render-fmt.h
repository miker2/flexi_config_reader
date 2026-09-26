#pragma once

#include <fmt/format.h>

#include "flexi_cfg/render.h"

// The fmt adapter over the core renderer in `render.h`. `ctx.out()` yields a `fmt::appender`,
// which already satisfies the output-iterator requirement, so the renderer writes directly into
// fmt's buffer -- no `std::stringstream`, no per-value `std::string` copy.

template <typename T>
struct fmt::formatter<flexi_cfg::Dump<T>> {
  static constexpr auto parse(format_parse_context& ctx) {
    auto it = ctx.begin();
    if (it != ctx.end() && *it != '}') {
      report_error("invalid format spec for flexi_cfg::Dump");
    }
    return it;
  }

  template <typename FormatContext>
  auto format(const flexi_cfg::Dump<T>& dump, FormatContext& ctx) const {
    return flexi_cfg::render_dump(ctx.out(), dump);
  }
};

// Formatter for all types that inherit from `config::types::ConfigBase`.
template <typename T>
struct fmt::formatter<
    T, std::enable_if_t<
           std::is_convertible_v<T, std::shared_ptr<flexi_cfg::config::types::ConfigBase>>, char>>
    : formatter<std::string_view> {
  // parse is inherited from formatter<string_view>
  //
  // Writes straight into fmt's output buffer. Kept as a shim over `flexi_cfg::render` for the
  // dozens of existing `logger::trace`/`fmt::join` call sites; `flexi_cfg::Dump` is what you reach
  // for when the options need to be anything other than the defaults.
  auto format(const std::shared_ptr<flexi_cfg::config::types::ConfigBase>& cfg,
              format_context& ctx) const {
    return flexi_cfg::render(ctx.out(), cfg, flexi_cfg::DumpOptions{}, 0);
  }
};
