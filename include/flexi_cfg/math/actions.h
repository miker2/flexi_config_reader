#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include "flexi_cfg/logger.h"
#include "flexi_cfg/math/grammar.h"
#include "flexi_cfg/math/helpers.h"
#include "flexi_cfg/utils.h"

// NOTE: A large portion of this code was derived from:
//   https://github.com/taocpp/PEGTL/blob/main/src/example/pegtl/calculator.cpp

// The algorithm used to perform the math operations is described here:
//   https://en.wikipedia.org/wiki/Shunting_yard_algorithm

// This article was also extremely useful during development:
//   https://www.engr.mun.ca/~theo/Misc/exp_parsing.htm

// Also useful info:
// See: https://godbolt.org/z/7vdE6aYdh
//      https://godbolt.org/z/9aeh7hvjv
//      https://godbolt.org/z/35c19neYG
// https://marginalhacks.com/Hacks/libExpr.rb/

namespace flexi_cfg::math {

struct ActionData {
  math::Stacks s{};

  // This could be a `config::types::CfgMap` object, but rather than introduce that dependency,
  // we'll make it a simple map from strings to values.
  std::map<std::string, double> var_ref_map{};

  // Track open/close brackets to know when to finalize the result.
  size_t bracket_cnt{0};

  double res{};  // The final result of the computation.
};

/* Actions */
template <typename Rule>
struct action : peg::nothing<Rule> {};

template <>
struct action<config::NUMBER> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    out.s.push(std::stod(in.string()));
  }
};

template <>
struct action<pi> {
  static void apply0(ActionData& out) { out.s.push(M_PI); }
};

template <>
struct action<Um> {
  static void apply0(ActionData& out) {
    // Cheeky trick to support unary minus operator. Sneaky but simple!
    out.s.push(-1);  // This value doesn't matter. It is ignored by the unary-minus operator
    out.s.push(std::string("m"));
  }
};

template <>
struct action<Po> {
  static void apply0(ActionData& out) {
    out.s.open();
    out.bracket_cnt++;
  }
};

template <>
struct action<Pc> {
  static void apply0(ActionData& out) {
    out.s.close();
    out.bracket_cnt--;
  }
};

template <>
struct action<expression> {
  static void apply0(ActionData& out) {
    // The top-most stack is automatically "finished" when leaving a bracketed operation. The
    // `expression` rule is also contained within a bracketed operation, but is also the terminal
    // rule, so we only want to call `finish` when not within brackets.
    logger::debug(" !!! In expression action !!!");
    if (out.bracket_cnt == 0) {
      out.s.dump(logger::Severity::DEBUG);
      logger::debug(" !!!  Finishing  !!!");
      out.res = out.s.finish();
      out.s.dump();
    }
  }
};

template <>
struct action<config::VALUE_LOOKUP> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    // Remove the VALUE_LOOKUP enclosing delimiters from the VALUE_LOOKUP
    const auto var_ref = utils::trim(utils::removeSubStr(in.string(), "$("), ")");

    // Look up the corresponding value from the map.
    if (!out.var_ref_map.contains(var_ref)) {
      throw std::runtime_error("This should never happen! " + var_ref + " not found!");
    }
    logger::debug("Replacing {} with {}.", in.string(), out.var_ref_map[var_ref]);

    out.s.push(out.var_ref_map[var_ref]);
  }
};

/// Everything above here is common to both grammars. Everything below exclusive to this one.

template <>
struct action<Bo> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    out.s.push(in.string());
  }
};

}  // namespace flexi_cfg::math
