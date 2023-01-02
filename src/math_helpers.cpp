
#include <cmath>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "flexi_cfg/logger.h"
#include "flexi_cfg/math/helpers.h"

namespace ops {

struct Operator {
  int p{-1};     // precedence
  bool l{true};  // left-associative=true, right-associative=false
  std::function<double(double, double)> f;
};

auto get(const std::string& op_) -> const Operator& {
  static const std::map<std::string, Operator> op_map = {
      // NOTE: The exact precedence values used here are not important, only the relative
      // precendence
      // as it is used to determine order of operations.
      {"+", {.p = 6, .l = true, .f = std::plus<>()}},
      {"-", {.p = 6, .l = true, .f = std::minus<>()}},
      {"*", {.p = 8, .l = true, .f = std::multiplies<>()}},
      {"/", {.p = 8, .l = true, .f = std::divides<>()}},
      // Support both traditional '^' and pythonic '**' power operators
      {"^", {.p = 9, .l = false, .f = [](double x, double e) -> double { return std::pow(x, e); }}},
      {"**",
       {.p = 9, .l = false, .f = [](double x, double e) -> double { return std::pow(x, e); }}},
      // This is a placeholder for the unary minus operator (represented as a binary multiply where
      // the first argument is discarded (generally a `-1`).
      // A high precedence is used here in order to ensure the unary minus happens before other
      // operations
      {"m", {.p = 10, .l = false, .f = [](double unused, double x) -> double { return -x; }}}};

  return op_map.at(op_);
}

/// \brief Evaluates the operation at the top of the stack.
/// \param[in/out] vals - The stack of operands. The last two operands are popped of the stack and
///                       the result of the operation is pushed onto the stack.
/// \param[in] ops - the stack of operators
void evalBack(std::vector<double>& vals, std::vector<std::string>& ops) {
  // NOLINTNEXTLINE
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
  const auto v = ops::get(op).f(lhs, rhs);

  flexi_cfg::logger::debug("Reducing: {} {} {} = {}", lhs, op, rhs, v);
  // Push the result onto the stack.
  vals.push_back(v);
  flexi_cfg::logger::trace("stack: op={}, v={}", ops.size(), vals.size());
}
}  // namespace ops

namespace flexi_cfg::math {

void Stack::push(const std::string& op) {
  // This is the core of the shunting yard algorithm. If the operator stack is not empty, then
  // precedence of the new operator is compared to the operator at the top of the stack
  // (comparison depends on left vs right-associativity). If the operator at the top of the stack
  // has higher precedence, then `evalBack` is called with will use the top two operands on the
  // stack to evaluate the operator. This continues while operators on the stack have higher
  // precedence.
  if (!ops_.empty()) {
    auto o2 = ops_.back();
    while (!ops_.empty() && (ops::get(o2).p > ops::get(op).p ||
                             (ops::get(o2).p == ops::get(op).p && ops::get(op).l))) {
      logger::trace("{} > {}", ops::get(o2).p, ops::get(op).p);
      logger::trace("{} == {} && {}", ops::get(o2).p, ops::get(op).p, ops::get(op).l);
      logger::debug("Comparing {} and {}", op, o2);
      ops::evalBack(vs_, ops_);
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

void Stack::push(const double& v) {
  vs_.push_back(v);
  logger::trace("Pushing {} onto stack. ops={}, values={}", v, ops_.size(), vs_.size());
}

auto Stack::finish() -> double {
  // Clear out the stacks by evaluating any remaining operators.
  while (!ops_.empty()) {
    evalBack();
  }
  // NOLINTNEXTLINE
  assert(vs_.size() == 1);
  const auto v = vs_.back();
  vs_.clear();
  logger::trace("Finishing stack: {}", v);
  return v;
}

void Stack::dump(logger::Severity lvl) const {
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

void Stack::evalBack() {
  logger::debug("In 'evalBack' - ops={}, vs={}", ops_.size(), vs_.size());
  // NOLINTNEXTLINE
  assert(vs_.size() == ops_.size() + 1);
  if (ops_.empty() && vs_.size() == 1) {
    // Nothing to do here
    return;
  }
  ops::evalBack(vs_, ops_);
}

Stacks::Stacks() { open(); }

void Stacks::open() {
  logger::debug("Opening stack.");
  s_.emplace_back();
}

void Stacks::close() {
  logger::debug("Closing stack.");
  // NOLINTNEXTLINE
  assert(s_.size() > 1);
  const auto r = s_.back().finish();
  s_.pop_back();
  s_.back().push(r);
}

auto Stacks::finish() -> double {
  // NOLINTNEXTLINE
  assert(s_.size() == 1);
  return s_.back().finish();
}

void Stacks::dump(logger::Severity lvl) {
  for (const auto& s : s_) {
    s.dump(lvl);
  }
}

}  // namespace math
