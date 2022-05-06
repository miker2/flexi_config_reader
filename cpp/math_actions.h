#pragma once

#include <functional>
#include <string>
#include <vector>

#include "logger.h"
#include "math_grammar.h"
#include "utils.h"

// NOTE: A large portion of this code was derived from:
//   https://github.com/taocpp/PEGTL/blob/main/src/example/pegtl/calculator.cpp

// The algorithm used to perform the math operations is described here:
//   https://en.wikipedia.org/wiki/Shunting_yard_algorithm

// This article was also extremely useful during development:
//   https://www.engr.mun.ca/~theo/Misc/exp_parsing.htm

// Also usefule info:
// See: https://godbolt.org/z/7vdE6aYdh
//      https://godbolt.org/z/9aeh7hvjv
//      https://godbolt.org/z/35c19neYG
// https://marginalhacks.com/Hacks/libExpr.rb/

namespace math {

namespace ops {

struct op {
  int p;   // precedence
  bool l;  // left-associative=true, right-associative=false
  std::function<double(double, double)> f;
};

using OpsMap = std::map<std::string, op>;
const OpsMap map = {
    // NOTE: The exact precedence values used here are not important, only the relative precendence
    // as it is used to determine order of operations.
    {"+", {.p = 6, .l = true, .f = std::plus<double>()}},
    {"-", {.p = 6, .l = true, .f = std::minus<double>()}},
    {"*", {.p = 8, .l = true, .f = std::multiplies<double>()}},
    {"/", {.p = 8, .l = true, .f = std::divides<double>()}},
    // Support both traditional '^' and pythonic '**' power operators
    {"^", {.p = 9, .l = false, .f = [](double x, double e) -> double { return std::pow(x, e); }}},
    {"**", {.p = 9, .l = false, .f = [](double x, double e) -> double { return std::pow(x, e); }}},
    // This is a placeholder for the unary minus operator (represented as a binary multiply where
    // the first argument is discarded (generally a `-1`).
    // A high precedence is used here in order to ensure the unary minus happens before other
    // operations
    {"m", {.p = 10, .l = false, .f = [](double unused, double x) -> double { return -x; }}}};

/// \brief Evaluates the operation at the top of the stack.
/// \param[in/out] vals - The stack of operands. The last two operands are popped of the stack and
///                       the result of the operation is pushed onto the stack.
/// \param[in] ops - the stack of operators
void evalBack(std::vector<double>& vals, std::vector<std::string>& ops) {
  assert(vals.size() == ops.size() + 1);
  // Extract the operands
  const auto rhs = vals.back();
  vals.pop_back();
  const auto lhs = vals.back();
  vals.pop_back();

  // Extract the operator
  const auto op = ops.back();
  ops.pop_back();
  // Compute the new value
  const auto v = math::ops::map.at(op).f(lhs, rhs);

  logger::debug("Reducing: {} {} {} = {}", lhs, op, rhs, v);
  // Push the result onto the stack.
  vals.push_back(v);
  logger::trace("stack: op={}, v={}", ops.size(), vals.size());
}
}  // namespace ops

class stack {
 public:
  void push(const std::string& op) {
    // This is the core of the shunting yard algorithm. If the operator stack is not empty, then
    // precedence of the new operator is compared to the operator at the top of the stack
    // (comparison depends on left vs right-associativity). If the operator at the top of the stack
    // has higher precedence, then `evalBack` is called with will use the top two operands on the
    // stack to evaluate the operator. This continues while operators on the stack have higher
    // precedence.
    if (!ops_.empty()) {
      auto o2 = ops_.back();
      while (!ops_.empty() &&
             (math::ops::map.at(o2).p > math::ops::map.at(op).p ||
              (math::ops::map.at(o2).p == math::ops::map.at(op).p && math::ops::map.at(op).l))) {
        logger::info("{} > {}", math::ops::map.at(o2).p, math::ops::map.at(op).p);
        logger::info("{} == {} && {}", math::ops::map.at(o2).p, math::ops::map.at(op).p,
                     math::ops::map.at(op).l);
        logger::debug("Comparing {} and {}", op, o2);
        math::ops::evalBack(vs_, ops_);
        if (!ops_.empty()) {
          // If the stack isn't empty, grab the next operator for comparison
          o2 = ops_.back();
        }
      }
    }
    // Finally, push the new operator onto the stack.
    ops_.push_back(op);
    logger::trace("Pushing {} onto stack. ops={}, values={}", op, ops_.size(), vs_.size());
  }

  void push(const double& v) {
    vs_.push_back(v);
    logger::trace("Pushing {} onto stack. ops={}, values={}", v, ops_.size(), vs_.size());
  }

  auto finish() -> double {
    // Clear out the stacks by evaluating any remaining operators.
    while (!ops_.empty()) {
      evalBack();
    }
    assert(vs_.size() == 1);
    const auto v = vs_.back();
    vs_.clear();
    logger::trace("Finishing stack: {}", v);
    return v;
  }

  void dump(logger::Severity lvl = logger::Severity::DEBUG) const {
    logger::log(lvl, "ops={}, vs={}", ops_.size(), vs_.size());
    std::stringstream ss;
    for (const auto& o : ops_) {
      ss << o << ", ";
    }
    logger::log(lvl, "ops = [{}]", ss.str());
    ss.str(std::string());
    for (const auto& v : vs_) {
      ss << v << ", ";
    }
    logger::log(lvl, "vs = [{}]", ss.str());
  }

 private:
  std::vector<std::string> ops_;
  std::vector<double> vs_;

  void evalBack() {
    logger::debug("In 'evalBack' - ops={}, vs={}", ops_.size(), vs_.size());
    assert(vs_.size() == ops_.size() + 1);
    if (ops_.size() == 0 && vs_.size() == 1) {
      // Nothing to do here
      return;
    }
    math::ops::evalBack(vs_, ops_);
  }
};

// This is a stack of stacks. It makes evaluating bracketed operations simpler. Any time an opening
// bracket is encountered, a new stack is added. When the closing bracket is encountered:
//   1. the stack current stack is "finished", caching the result.
//   2. the current stack is removed from the stack of stacks.
//   3. the result is then pushed onto the top stack.
struct stacks {
  stacks() { open(); }

  void open() {
    logger::debug("Opening stack.");
    s_.emplace_back();
  }

  template <typename T>
  void push(const T& t) {
    assert(!s_.empty());
    s_.back().push(t);
  }

  void close() {
    logger::debug("Closing stack.");
    assert(s_.size() > 1);
    const auto r = s_.back().finish();
    s_.pop_back();
    s_.back().push(r);
  }

  double finish() {
    assert(s_.size() == 1);
    return s_.back().finish();
  }

  void dump(logger::Severity lvl = logger::Severity::DEBUG) {
    for (const auto& s : s_) {
      s.dump(lvl);
    }
  }

 private:
  std::vector<stack> s_;
};

struct ActionData {
  math::stacks s;

  // This could be a `config::types::CfgMap` object, but rather than introduce that dependency,
  // we'll make it a simple map from strings to values.
  std::map<std::string, double> var_ref_map;

  // Track open/close brackets to know when to finalize the result.
  size_t bracket_cnt{0};

  double res;  // The final result of the computation.
};

}  // namespace math

namespace math {

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
    out.s.push("m");
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
struct action<config::VAR_REF> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    // Remove the VAR_REF enclosing delimiters from the VAR_REF
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

}  // namespace math

namespace grammar2 {

template <typename Rule>
struct action : peg::nothing<Rule> {};

template <>
struct action<config::NUMBER> : math::action<config::NUMBER> {};

template <>
struct action<math::pi> : math::action<math::pi> {};

template <>
struct action<math::Um> : math::action<math::Um> {};

template <>
struct action<math::Po> : math::action<math::Po> {};

template <>
struct action<math::Pc> : math::action<math::Pc> {};

template <>
struct action<expression> : math::action<math::expression> {};

template <>
struct action<config::VAR_REF> : math::action<config::VAR_REF> {};

/// Everything above here is common to both grammars. Everything below exclusive to this one.

template <>
struct action<PM> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, math::ActionData& out) {
    out.s.push(in.string());
  }
};

template <>
struct action<MD> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, math::ActionData& out) {
    out.s.push(in.string());
  }
};

template <>
struct action<math::Bpow> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, math::ActionData& out) {
    logger::warn("Found {}!", in.string());
    out.s.push(in.string());
  }
};

// These are no-ops. They're only part of the grammar in order to build the AST, but not actually
// functional in performing the math. We include them here for tracing purposes only.
template <>
struct action<T> {
  static void apply0(math::ActionData& out) { logger::debug(" --- In T action"); }
};

template <>
struct action<F> {
  static void apply0(math::ActionData& out) { logger::debug(" --- In F action"); };
};

template <>
struct action<P> {
  static void apply0(math::ActionData& out) { logger::debug(" --- In P action"); };
};

}  // namespace grammar2
