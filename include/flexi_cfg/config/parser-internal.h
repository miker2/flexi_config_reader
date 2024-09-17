#pragma once

#include <tao/pegtl/config.hpp>

#include "flexi_cfg/config/trace-internal.h"

namespace flexi_cfg::config::internal {
namespace peg = TAO_PEGTL_NAMESPACE;

template <typename Rule, template <typename...> class Action = peg::nothing,
          template <typename...> class Control = peg::normal, typename ParseInput, typename... States>
auto parseCore(ParseInput&& input, States&&... output) -> bool {
#ifdef ENABLE_PARSER_TRACE
  return peg_extensions::complete_trace<Rule, Action, Control>(input, output...);
#else
  return peg::parse<Rule, Action, Control>(input, output... );
#endif
}

template< typename Rule,
          template< typename... > class Action = peg::nothing,
          template< typename... > class Control = peg::normal,
          typename Outer,
          typename ParseInput,
          typename... States >
auto parseNestedCore(const Outer& o, ParseInput&& in, States&&... st )
    -> bool {
#ifdef ENABLE_PARSER_TRACE
  return peg_extensions::complete_trace_nested<Rule, Action, Control>(o, in, st...);
#else
  return peg::parse_nested<Rule, Action, Control>(o, in, st...);
#endif
}
}  // namespace flexi_cfg::config::internal