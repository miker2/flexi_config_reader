#pragma once

#include <ostream>

#include "flexi_cfg/render.h"

// The ostream adapter over the core renderer in `render.h`. A thin wrapper, not a second
// implementation: everything here funnels into `flexi_cfg::render` / `flexi_cfg::render_map`.
//
// `std::ostreambuf_iterator<char>` writes straight to the stream's buffer;
// `std::ostream_iterator<char>` would go through formatted insertion for every single character.

namespace flexi_cfg {

/// \brief Renders `dump` into `os`.
template <typename T>
auto operator<<(std::ostream& os, const Dump<T>& dump) -> std::ostream& {
  render_dump(std::ostreambuf_iterator<char>(os), dump);
  return os;
}

}  // namespace flexi_cfg

namespace flexi_cfg::config::types {

/// \brief Backing for the `ConfigBase::stream` shim declared at the top of `classes.h`.
inline void renderToStream(std::ostream& os, const ConfigBase& cfg) {
  const DumpOptions opts{};
  render_detail::render_node(std::ostreambuf_iterator<char>(os), &cfg, opts, opts.base_indent);
}

/// \brief Backward-compatible map streaming. Equivalent to `Dump{data}` with default options.
template <MapLike MapType>
inline auto operator<<(std::ostream& os, const MapType& data) -> std::ostream& {
  const DumpOptions opts{};
  render_map(std::ostreambuf_iterator<char>(os), data, opts, opts.base_indent);
  return os;
}

}  // namespace flexi_cfg::config::types
