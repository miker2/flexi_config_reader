#pragma once

#include <functional>
#include <string>
#include <vector>

#include "logger.h"
#include "math_grammar.h"

namespace ops {

struct op {
  int p;   // precedence
  bool l;  // left-associative?
  std::function<double(double, double)> f;
};

using OpsMap = std::map<std::string, op>;
OpsMap ops = {
    {"+", {.p = 6, .l = true, .f = std::plus<double>()}},
    {"-", {.p = 6, .l = true, .f = std::minus<double>()}},
    {"*", {.p = 8, .l = true, .f = std::multiplies<double>()}},
    {"/", {.p = 8, .l = true, .f = std::divides<double>()}},
    {"^", {.p = 9, .l = false, .f = [](double x, double e) -> double { return std::pow(x, e); }}},
    // This is a placeholder for the unary minus operator (represented as a binary multiply where
    // the first argument is discarded (generally a `-1`).
    {"m", {.p = 10, .l = false, .f = [](double e, double x) -> double { return -x; }}}};

using OpMap = std::map<std::string, std::function<double(double, double)>>;
OpMap map = {
    {"+", std::plus<double>()},
    {"-", std::minus<double>()},
    {"*", std::multiplies<double>()},
    {"/", std::divides<double>()},
    {"^", [](double x, double e) -> double { return std::pow(x, e); }},
    // This is a placeholder for the unary minus operator (represented as a binary multiply∑
    {"m", [](double x, double e = -1) -> double { return e * x; }}};

void reduce(std::vector<double>& vals, std::vector<std::string>& ops) {
  // Extract the operands
  const auto rhs = vals.back();
  vals.pop_back();
  const auto lhs = vals.back();
  vals.pop_back();

  // Extract the operator
  const auto op = ops.back();
  ops.pop_back();
  // Compute the new value
  const auto v = ops::ops[op].f(lhs, rhs);

  logger::debug("Reducing: {} {} {} = {}", lhs, op, rhs, v);
  vals.push_back(v);
  logger::trace("stack: op={}, v={}", ops.size(), vals.size());
}
}  // namespace ops

namespace {
class stack {
 public:
  void push(const std::string& op) {
    if (!ops_.empty()) {
      auto o2 = ops_.back();
      while (!ops_.empty() && (ops::ops[o2].p > ops::ops[op].p ||
                               (ops::ops[o2].p == ops::ops[op].p && ops::ops[op].l))) {
        logger::info("{} > {}", ops::ops[o2].p, ops::ops[op].p);
        logger::info("{} == {} && {}", ops::ops[o2].p, ops::ops[op].p, ops::ops[op].l);
        logger::debug("Comparing {} and {}", op, o2);
        ops::reduce(vs_, ops_);
        if (!ops_.empty()) {
          o2 = ops_.back();
        }
      }
    }
    ops_.push_back(op);
    logger::trace("Pushing {} onto stack. ops={}, values={}", op, ops_.size(), vs_.size());
  }

  void push(const double& v) {
    vs_.push_back(v);
    logger::trace("Pushing {} onto stack. ops={}, values={}", v, ops_.size(), vs_.size());
  }

  auto finish() -> double {
    while (!ops_.empty()) {
      reduce();
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

  void reduce() {
    logger::debug("In 'reduce' - ops={}, vs={}", ops_.size(), vs_.size());
    assert(vs_.size() == ops_.size() + 1);
    if (ops_.size() == 0 && vs_.size() == 1) {
      // Nothing to do here
      return;
    }
    ops::reduce(vs_, ops_);
  }
};

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
}  // namespace

void finalizeStack(std::vector<stack>& stacks_) {
  logger::debug("Finalizing stacks: contains {} stack(s)", stacks_.size());
  const auto r = stacks_.back().finish();
  if (stacks_.size() > 1) {
    stacks_.pop_back();
  }
  logger::trace("stacks size: {}", stacks_.size());
  stacks_.back().push(r);
}

struct ActionData {
  stacks s;

  std::vector<stack> stacks = {{}};

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
  static void apply0(ActionData& out) { out.s.open(); }
};

template <>
struct action<Pc> {
  static void apply0(ActionData& out) { out.s.close(); }
};

template <>
struct action<E> {
  static void apply0(ActionData& out) { logger::warn("In E action."); }
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
