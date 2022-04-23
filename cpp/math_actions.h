#pragma once

#include <functional>
#include <string>
#include <vector>

#include "logger.h"
#include "math_grammar.h"

// NOTE: A large portion of this code was derived from:
//   https://github.com/taocpp/PEGTL/blob/main/src/example/pegtl/calculator.cpp

// The algorithm used to perform the math operations is described here:
//   https://en.wikipedia.org/wiki/Shunting_yard_algorithm

// This article was also extremely useful during development:
//   https://www.engr.mun.ca/~theo/Misc/exp_parsing.htm

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

  void dump() const {
    logger::info("ops={}, vs={}", ops_.size(), vs_.size());
    std::stringstream ss;
    for (const auto& o : ops_) {
      ss << o << ", ";
    }
    logger::debug("ops = [{}]", ss.str());
    ss.str(std::string());
    for (const auto& v : vs_) {
      ss << v << ", ";
    }
    logger::debug("vs = [{}]", ss.str());
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

  void dump() {
    for (const auto& s : s_) {
      s.dump();
    }
  }

 private:
  std::vector<stack> s_;
};

}  // namespace math

struct ActionData {
  math::stacks s;

  std::vector<math::stack> stacks = {{}};

  size_t bracket_cnt{0};

  double res;
};

namespace grammar1 {

template <typename Rule>
struct action : peg::nothing<Rule> {};

template <>
struct action<v> {
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
    // This is sneaky, but a simple way of making this work!
    out.s.push("m");
    out.s.push(-1);
  }
};

template <>
struct action<Bo> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    out.s.push(in.string());
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
struct action<E> {
  static void apply0(ActionData& out) {
    logger::warn("In E action.");
    if (out.bracket_cnt == 0) {
      out.s.dump();
      logger::info("Finishing!");
      out.res = out.s.finish();
      out.s.dump();
    }
  }
};

}  // namespace grammar1

namespace grammar2 {

template <typename Rule>
struct action : peg::nothing<Rule> {};

template <>
struct action<v> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    out.stacks.back().push(std::stod(in.string()));
  }
};

template <>
struct action<pi> {
  static void apply0(ActionData& out) { out.stacks.back().push(M_PI); }
};

template <>
struct action<PM> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    out.stacks.back().push(in.string());
  }
};

template <>
struct action<MD> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    out.stacks.back().push(in.string());
  }
};

template <>
struct action<Bpow> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    logger::warn("Found {}!", in.string());
    out.stacks.back().push(in.string());
  }
};

template <>
struct action<Po> {
  static void apply0(ActionData& out) {
    logger::debug("Open parentheses, opening stack.");
    out.stacks.emplace_back();
  }
};

template <>
struct action<Pc> {
  static void apply0(ActionData& out) { logger::debug("Close parentheses, closing stack."); }
};

template <>
struct action<Um> {
  static void apply0(ActionData& out) {
    // Cheeky trick to support unary minus operator.
    out.stacks.back().push(-1);
    out.stacks.back().push("m");
  }
};

template <>
struct action<E> {
  static void apply0(ActionData& out) {
    logger::warn("In E action");
    // For every expression we want to finalize the top of the stack, remove it and put the result
    // on the top of the stack.
    const auto r = out.stacks.back().finish();
    out.stacks.pop_back();
    if (out.stacks.empty()) {
      out.res = r;
    } else {
      out.stacks.back().push(r);
    }
  }
};

template <>
struct action<T> {
  static void apply0(ActionData& out) { logger::warn("In T action"); }
};

template <>
struct action<F> {
  static void apply0(ActionData& out) { logger::warn("In F action"); };
};

template <>
struct action<P> {
  static void apply0(ActionData& out) { logger::warn("In P action"); };
};

}  // namespace grammar2
