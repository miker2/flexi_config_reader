#pragma once

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <iterator>
#include <string_view>

#include "flexi_cfg/config/classes.h"

// NOTE: this header names no fmt type and includes no ostream header. `config/classes.h` includes
// it right back from its last line, so the two form a deliberate cycle: whichever of the two a
// translation unit reaches first, the node types end up complete before the renderer below is
// compiled, and the adapters at the bottom see the renderer.
//
// Keeping fmt out of here is the seam a future effort to make fmt optional would cut on: the
// renderer only ever writes through an output iterator, so `fmt::appender`, an
// `std::ostreambuf_iterator` and a `back_insert_iterator` are all equally valid sinks.

namespace flexi_cfg {

/// \brief Caller-supplied knobs for rendering a config tree.
struct DumpOptions {
  /// Append `  # <source>:<line>` to each value. Reproduces the old PRINT_SRC=1 behaviour.
  bool show_source{true};
  /// The indentation level the top of the rendered tree sits at. The node's own position in the
  /// tree it was parsed from has no say in this.
  std::size_t base_indent{0};
};

/// \brief Wraps a config object (or a map of them) together with the options to render it with.
template <typename T>
struct Dump {
  const T& value;
  DumpOptions opts{};
};

template <typename T>
Dump(const T&) -> Dump<T>;
template <typename T>
Dump(const T&, DumpOptions) -> Dump<T>;

namespace render_detail {

template <typename OutputIt>
auto put(OutputIt out, std::string_view str) -> OutputIt {
  return std::copy(str.begin(), str.end(), out);
}

template <typename OutputIt>
auto put(OutputIt out, char chr) -> OutputIt {
  *out++ = chr;
  return out;
}

/// \brief Writes `levels` worth of indentation. No allocation, unlike `std::string(n, ' ')`.
template <typename OutputIt>
auto indent(OutputIt out, std::size_t levels) -> OutputIt {
  return std::fill_n(out, levels * config::types::tw, ' ');
}

template <typename OutputIt>
auto put_number(OutputIt out, std::size_t value) -> OutputIt {
  std::array<char, 24> buf{};
  const auto res = std::to_chars(buf.data(), buf.data() + buf.size(), value);
  return put(out, std::string_view(buf.data(), static_cast<std::size_t>(res.ptr - buf.data())));
}

[[nodiscard]] inline auto same_loc(const config::types::ConfigBase& lhs,
                                   const config::types::ConfigBase& rhs) -> bool {
  return lhs.line == rhs.line && lhs.source == rhs.source;
}

/// \brief Writes the output of `ConfigBase::loc()` without allocating: `<source>:<line>`,
/// followed by ` (from <a> <- <b>)` for the node's origin chain.
///
/// Mirrors `loc()`, including which origins it leaves out: any that points back at the node's own
/// location, and consecutive repeats of the same location, neither of which add information. Kept
/// in step with that function -- the two have to agree, since `loc()` still backs the location
/// text in exception and log messages.
template <typename OutputIt>
auto put_loc(OutputIt out, const config::types::ConfigBase& cfg) -> OutputIt {
  out = put(out, cfg.source);
  out = put(out, ':');
  out = put_number(out, cfg.line);

  const config::types::ConfigBase* last = nullptr;
  for (const auto& origin : cfg.origins) {
    if (same_loc(*origin, cfg) || (last != nullptr && same_loc(*origin, *last))) {
      continue;
    }
    out = put(out, last == nullptr ? " (from " : " <- ");
    out = put(out, origin->source);
    out = put(out, ':');
    out = put_number(out, origin->line);
    last = origin.get();
  }
  if (last != nullptr) {
    out = put(out, ')');
  }
  return out;
}

[[nodiscard]] inline auto is_struct_like(const config::types::ConfigBase& cfg) -> bool {
  using config::types::Type;
  return cfg.type == Type::kStruct || cfg.type == Type::kStructInProto ||
         cfg.type == Type::kProto || cfg.type == Type::kReference;
}

template <typename OutputIt>
auto render_node(OutputIt out, const config::types::ConfigBase* cfg, const DumpOptions& opts,
                 std::size_t depth) -> OutputIt;

}  // namespace render_detail

/// \brief Renders every entry of a map of config nodes, one per line.
///
/// Struct-like entries render themselves (including their own indentation and trailing `}`); every
/// other entry is written as `<indent><key> = <value>`, optionally followed by a source comment.
template <typename OutputIt, typename MapType>
auto render_map(OutputIt out, const MapType& data, const DumpOptions& opts, std::size_t depth)
    -> OutputIt {
  for (const auto& kv : data) {
    const config::types::ConfigBase* value = kv.second.get();
    if (value != nullptr && render_detail::is_struct_like(*value)) {
      // Don't add extra whitespace, as this is handled entirely by the struct-like objects.
      out = render_detail::render_node(out, value, opts, depth);
    } else {
      out = render_detail::indent(out, depth);
      out = render_detail::put(out, kv.first);
      out = render_detail::put(out, " = ");
      out = render_detail::render_node(out, value, opts, depth);
      // `loc()` only exists on the base pointer; a map of some more derived pointer type never
      // carried a source comment, so the compile-time half of this test has to stay.
      if constexpr (std::is_same_v<typename MapType::mapped_type, config::types::BasePtr>) {
        if (opts.show_source && value != nullptr) {
          out = render_detail::put(out, "  # ");
          out = render_detail::put_loc(out, *value);
        }
      }
    }
    out = render_detail::put(out, '\n');
  }
  return out;
}

/// \brief The single rendering implementation. Dispatches on `ConfigBase::type`.
template <typename OutputIt>
auto render(OutputIt out, const config::types::BasePtr& cfg, const DumpOptions& opts,
            std::size_t depth) -> OutputIt {
  return render_detail::render_node(out, cfg.get(), opts, depth);
}

namespace render_detail {

template <typename OutputIt>
auto render_node(OutputIt out, const config::types::ConfigBase* cfg, const DumpOptions& opts,
                 std::size_t depth) -> OutputIt {
  using namespace config::types;  // NOLINT(google-build-using-namespace)

  if (cfg == nullptr) {
    return put(out, "NULL");
  }

  // Every node type is constructed with exactly one `Type`, so a switch plus a static_cast is
  // sound here and saves the `dynamic_pointer_cast` the old render loop did per entry.
  switch (cfg->type) {
    case Type::kValue:
    case Type::kString:
    case Type::kNumber:
    case Type::kBoolean:
    case Type::kExpression: {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
      return put(out, static_cast<const ConfigValue*>(cfg)->value);
    }
    case Type::kList: {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
      const auto& list = *static_cast<const ConfigList*>(cfg);
      out = put(out, '[');
      for (auto it = list.data.begin(); it != list.data.end(); it = std::next(it)) {
        out = render_node(out, it->get(), opts, depth);
        if (std::next(it) != list.data.end()) {
          out = put(out, ", ");
        }
      }
      return put(out, ']');
    }
    case Type::kValueLookup: {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
      const auto& lookup = *static_cast<const ConfigValueLookup*>(cfg);
      out = put(out, "$(");
      for (auto it = lookup.keys.begin(); it != lookup.keys.end(); it = std::next(it)) {
        if (it != lookup.keys.begin()) {
          out = put(out, '.');
        }
        out = put(out, *it);
      }
      return put(out, ')');
    }
    case Type::kVar: {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
      return put(out, static_cast<const ConfigVar*>(cfg)->name);
    }
    case Type::kStruct:
    case Type::kStructInProto: {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
      const auto& node = *static_cast<const ConfigStruct*>(cfg);
      out = indent(out, depth);
      out = put(out, "struct ");
      out = put(out, node.name);
      out = put(out, " {\n");
#if DEBUG_CLASSES
      out = indent(out, depth);
      out = put(out, "-- ");
      out = put_number(out, node.data.size());
      out = put(out, " k/v pairs\n");
#endif
      out = flexi_cfg::render_map(out, node.data, opts, depth + 1);
      out = indent(out, depth);
      return put(out, '}');
    }
    case Type::kProto: {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
      const auto& node = *static_cast<const ConfigProto*>(cfg);
      out = indent(out, depth);
      out = put(out, "proto ");
      out = put(out, node.name);
      out = put(out, " {\n");
#if DEBUG_CLASSES
      out = indent(out, depth);
      out = put(out, "-- ");
      out = put_number(out, node.data.size());
      out = put(out, " k/v pairs\n");
#endif
      out = flexi_cfg::render_map(out, node.data, opts, depth + 1);
      out = indent(out, depth);
      return put(out, '}');
    }
    case Type::kReference: {
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
      const auto& node = *static_cast<const ConfigReference*>(cfg);
      out = indent(out, depth);
      out = put(out, "reference ");
      out = put(out, node.proto);
      out = put(out, " as ");
      out = put(out, node.name);
      out = put(out, " {\n");
#if DEBUG_CLASSES
      out = indent(out, depth);
      out = put(out, "-- ");
      out = put_number(out, node.ref_vars.size());
      out = put(out, " ref vars\n");
      out = indent(out, depth);
      out = put(out, "-- ");
      out = put_number(out, node.data.size());
      out = put(out, " k/v pairs\n");
#endif
      out = flexi_cfg::render_map(out, node.ref_vars, opts, depth + 1);
      out = flexi_cfg::render_map(out, node.data, opts, depth + 1);
      out = indent(out, depth);
      return put(out, '}');
    }
    case Type::kUnknown:
      break;
  }
  // `kUnknown` is only ever a list's element type, never a node's own type, so there is nothing
  // sensible to render here.
  return out;
}

}  // namespace render_detail

/// \brief Renders a `Dump<T>`, picking the map or single-node entry point based on `T`.
template <typename OutputIt, typename T>
auto render_dump(OutputIt out, const Dump<T>& dump) -> OutputIt {
  if constexpr (config::types::MapLike<T>) {
    return render_map(out, dump.value, dump.opts, dump.opts.base_indent);
  } else {
    return render_detail::render_node(out, dump.value.get(), dump.opts, dump.opts.base_indent);
  }
}

}  // namespace flexi_cfg

// The adapters go last, for the same reason `classes.h` includes this header last.
#include "flexi_cfg/render-fmt.h"      // NOLINT(misc-include-cleaner,llvm-include-order)
#include "flexi_cfg/render-ostream.h"  // NOLINT(misc-include-cleaner,llvm-include-order)
