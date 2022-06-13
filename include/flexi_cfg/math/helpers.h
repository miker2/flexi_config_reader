#pragma once

#include <string>
#include <vector>

#include "flexi_cfg/logger.h"

namespace math {

class stack {
 public:
  void push(const std::string& op);

  void push(const double& v);

  auto finish() -> double;

  void dump(logger::Severity lvl = logger::Severity::DEBUG) const;

 private:
  std::vector<std::string> ops_;
  std::vector<double> vs_;

  void evalBack();
};

// This is a stack of stacks. It makes evaluating bracketed operations simpler. Any time an opening
// bracket is encountered, a new stack is added. When the closing bracket is encountered:
//   1. the stack current stack is "finished", caching the result.
//   2. the current stack is removed from the stack of stacks.
//   3. the result is then pushed onto the top stack.
struct stacks {
  stacks();

  void open();

  template <typename T>
  void push(const T& t) {
    // NOLINTNEXTLINE
    assert(!s_.empty());
    s_.back().push(t);
  }

  void close();

  auto finish() -> double;

  void dump(logger::Severity lvl = logger::Severity::DEBUG);

 private:
  std::vector<stack> s_;
};

}  // namespace math
