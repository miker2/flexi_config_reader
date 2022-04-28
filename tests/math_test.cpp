#include <gtest/gtest.h>

#include <iostream>
#include <sstream>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <tao/pegtl/contrib/parse_tree_to_dot.hpp>
#include <unordered_map>
#include <utility>
#include <vector>

#include "math_actions.h"
#include "math_grammar.h"

namespace peg = TAO_PEGTL_NAMESPACE;

namespace {
const std::vector<std::pair<std::string, double>> test_strings = {
    {"3.14159 * 1e3", 3141.5899999999997},
    {"0.5 * (0.7 + 1.2)", 0.95},
    {"0.5 + 0.7 * 1.2", 1.3399999999999999},
    {"3*0.27 - 2.3**0.5 - 5*4", -20.70657508881031},
    {"3 ^ 2.4 * 12.2 + 0.1 + 4.3 ", 174.79264401590646},
    {"-4.7 * -(3.72 + -pi)", 2.7185145281279732},
    {"1/3 * -(5 + 4)", -3.0},
    {"3.4 * -(1.9**2 * (1/3.1 - 6) * (2.54- 17.0))", -1007.6399690322581}};
}

TEST(math_expression, grammar1_analyze) {
  namespace math = grammar1;
  ASSERT_EQ(peg::analyze<math::expression>(), 0);

  struct grammar : peg::seq<Mo, math::expression, Mc> {};
  ASSERT_EQ(peg::analyze<grammar>(), 0);
}

TEST(math_expression, grammar2_analyze) {
  namespace math = grammar2;
  ASSERT_EQ(peg::analyze<math::expression>(), 0);

  struct grammar : peg::seq<Mo, math::expression, Mc> {};
  ASSERT_EQ(peg::analyze<grammar>(), 0);
}

TEST(math_expression, grammar1_evaluate) {
  namespace math = grammar1;
  struct grammar : peg::seq<Mo, math::expression, Mc> {};

  auto test_input = [](const std::string& input) -> double {
    peg::memory_input in(input, "from content");
    ActionData out;
    const auto result = peg::parse<grammar, math::action>(in, out);
    return out.res;
  };

  for (const auto& input : test_strings) {
    std::cout << "Input: " << input.first << std::endl;
    double result{0};
    EXPECT_NO_THROW(result = test_input(" {{  " + input.first + "   }}"));
    EXPECT_FLOAT_EQ(result, input.second);
  }
}

TEST(math_expression, grammar2_evaluate) {
  namespace math = grammar2;
  struct grammar : peg::seq<Mo, math::expression, Mc> {};

  auto test_input = [](const std::string& input) -> double {
    peg::memory_input in(input, "from content");
    ActionData out;
    const auto result = peg::parse<grammar, math::action>(in, out);
    return out.res;
  };

  for (const auto& input : test_strings) {
    std::cout << "Input: " << input.first << std::endl;
    double result{0};
    EXPECT_NO_THROW(result = test_input(" {{  " + input.first + "   }}"));
    EXPECT_FLOAT_EQ(result, input.second);
  }
}
