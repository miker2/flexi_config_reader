#pragma once

#include <functional>
#include <string>
#include <vector>

#include "logger.h"
#include "math_grammar.h"

/*
struct PM : peg::sor<Bplus, Bminus> {};
struct MD : peg::sor<Bmult, Bdiv> {};

struct T;
struct E : peg::seq<T, peg::star<pd<PM>, T>> {};
struct F;
struct T : peg::seq<F, peg::star<pd<MD>, F>> {};
struct EXP : peg::seq<Bpow, F> {};
struct P;
struct F : peg::seq<P, peg::opt<EXP>> {};
struct N : peg::seq<Um, T> {};
struct P : peg::sor<pd<v>, peg::seq<Po, E, Pc>, N> {};
*/

namespace ops {

using OpMap = std::map<std::string, std::function<double(double, double)>>;

OpMap map = {{"+", std::plus<double>()},
             {"-", std::minus<double>()},
             {"*", std::multiplies<double>()},
             {"/", std::divides<double>()},
             {"^", [](double x, double e) -> double { return std::pow(x, e); }}};

}  // namespace ops

namespace {
class stack {
 public:
  void push(const std::string& op) {
    ops_.push_back(op);
    logger::trace("Pushing {} onto stack. ops={}, values={}", op, ops_.size(), vs_.size());
  }

  void push(const double& v) {
    vs_.push_back(v);
    logger::trace("Pushing {} onto stack. ops={}, values={}", v, ops_.size(), vs_.size());
  }

  auto finish() -> double {
    reduce();
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
    ss.flush();
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
    const auto rhs = vs_.back();
    vs_.pop_back();
    const auto lhs = vs_.back();
    vs_.pop_back();
    const auto op = ops_.back();
    ops_.pop_back();
    const auto v = ops::map.at(op)(lhs, rhs);
    logger::debug("Reducing: {} {} {} = {}", lhs, op, rhs, v);
    vs_.push_back(v);
    logger::trace("stack: op={}, v={}", ops_.size(), vs_.size());
  }
};

struct stacks {
  stacks() { open(); }

  void open() { s_.emplace_back(); }

  template <typename T>
  void push(const T& t) {
    assert(!s_.empty());
    s_.back().push(t);
  }

  void close() {
    assert(s_.size() > 1);
    const auto r = s_.back().finish();
    s_.pop_back();
    s_.back().push(r);
  }

  double finish() {
    assert(s_.size() == 1);
    return s_.back().finish();
  }

 private:
  std::vector<stack> s_;
};
}  // namespace

struct ActionData {
  stacks s;

  stack P = {};
  stack T = {};
  stack E = {};
  stack F = {};
};

void finalizeStack(std::vector<stack>& stacks_) {
  logger::debug("Finalizing stacks: contains {} stack(s)", stacks_.size());
  const auto r = stacks_.back().finish();
  if (stacks_.size() > 1) {
    stacks_.pop_back();
  }
  logger::trace("stacks size: {}", stacks_.size());
  stacks_.back().push(r);
}

namespace grammar2 {

template <typename Rule>
struct action : peg::nothing<Rule> {};

template <>
struct action<v> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    out.P.push(std::stod(in.string()));
  }
};

template <>
struct action<PM> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    out.E.push(in.string());
  }
};

template <>
struct action<MD> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    out.T.push(in.string());
  }
};

template <>
struct action<Bpow> {
  template <typename ActionInput>
  static void apply(const ActionInput& in, ActionData& out) {
    // Put something here!
    logger::warn("Found {}!", in.string());
    out.F.push(in.string());
  }
};

template <>
struct action<Po> {
  static void apply0(ActionData& out) {
    logger::debug("Open parentheses, creating new stack.");
    out.s.open();
  }
};

template <>
struct action<Pc> {
  static void apply0(ActionData& out) {
    logger::debug("Close parentheses, finalizing stack.");
    out.s.close();
  }
};

template <>
struct action<N> {
  static void apply0(ActionData& out) {
    const auto t = out.T.finish();
    logger::warn("N = {}", t);
    out.P.push(-1 * t);
  }
};

template <>
struct action<E> {
  static void apply0(ActionData& out) {
    const auto e = out.E.finish();
    logger::debug("E = {}", e);
    out.P.push(e);
  }
};

template <>
struct action<T> {
  static void apply0(ActionData& out) {
    const auto t = out.T.finish();
    logger::debug("T = {}", t);
    out.E.push(t);
  }
};

template <>
struct action<F> {
  static void apply0(ActionData& out) {
    const auto f = out.F.finish();
    logger::warn("F = {}", f);
    out.T.push(f);
  };
};

template <>
struct action<P> {
  static void apply0(ActionData& out) {
    const auto p = out.P.finish();
    logger::info("P = {}", p);
    out.F.push(p);
  };
};

}  // namespace grammar2
