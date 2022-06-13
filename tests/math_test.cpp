#include <gtest/gtest.h>

#include <iostream>
#include <sstream>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <tao/pegtl/contrib/parse_tree_to_dot.hpp>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "flexi_cfg/math/actions.h"
#include "flexi_cfg/math/grammar.h"

namespace peg = TAO_PEGTL_NAMESPACE;

namespace {
using RefMap = std::map<std::string, double>;
}

class MathExpressionTest : public ::testing::Test {
 protected:
  const std::vector<std::pair<std::string, double>> test_strings = {
      // Inject some whitespace to test robustness to it.
      {" 3.14159 * 1e3", 3141.5899999999997},
      {"0.5 *  (0.7 + 1.2 ) ", 0.95},
      {"0.5 + 0.7 * 1.2     ", 1.3399999999999999},
      {"3*0.27 - 2.3**0.5 - 5 * 4", -20.70657508881031},
      {"  3 ^ 2.4 * 12.2 + 0.1 + 4.3 ", 174.79264401590646},
      {"-4.7 * -(3.72 + -pi  ) ", 2.7185145281279732},
      {"  1/3 * -( 5 + 4 )  ", -3.0},
      {"\t3.4 * -(1.9**2 * (1/3.1 - 6) * (2.54- 17.0)\t)", -1007.6399690322581}};

  const std::vector<std::tuple<std::string, double, RefMap>> test_w_var_ref = {
      {"0.5 * ($(test1.key) - 0.234)", 0.503, {{"test1.key", 1.24}}},
      {"3*$(var_ref1) - 2.3**$(exponent) - 5 * 4",
       -20.70657508881031,
       {{"var_ref1", 0.27}, {"exponent", 0.5}}},
      {"  $(its.a.three) ^ 2.4 * $(another.var) + $(one.more.value) + 4.3 ",
       174.79264401590646,
       {{"its.a.three", 3}, {"another.var", 12.2}, {"one.more.value", 0.1}}}};
};

// NOLINTNEXTLINE
TEST(MathExpressionGrammar, analyze) { ASSERT_EQ(peg::analyze<math::expression>(), 0); }

// NOLINTNEXTLINE
TEST_F(MathExpressionTest, evaluate) {
  auto test_input = [](const std::string& input) -> double {
    peg::memory_input in(input, "from content");
    math::ActionData out;
    const auto result = peg::parse<math::expression, math::action>(in, out);
    return out.res;
  };

  for (const auto& input : test_strings) {
    std::cout << "Input: " << input.first << std::endl;
    double result{0};
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
    EXPECT_NO_THROW(result = test_input(input.first));
    EXPECT_FLOAT_EQ(result, input.second);
  }

  // We'll intentionally omit the var_ref_map here to ensure a failure occurs
  for (const auto& input : test_w_var_ref) {
    std::cout << "Input: " << std::get<0>(input) << std::endl;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
    EXPECT_THROW(test_input(std::get<0>(input)), std::runtime_error);
  }
}

// NOLINTNEXTLINE
TEST_F(MathExpressionTest, evaluate_var_ref) {
  auto test_input = [](const std::string& input, const RefMap& ref_map) -> double {
    peg::memory_input in(input, "from content");
    math::ActionData out;
    out.var_ref_map = ref_map;
    const auto result = peg::parse<math::expression, math::action>(in, out);
    return out.res;
  };

  for (const auto& input : test_w_var_ref) {
    std::cout << "Input: " << std::get<0>(input) << std::endl;
    double result{0};
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
    EXPECT_NO_THROW(result = test_input(std::get<0>(input), std::get<2>(input)));
    EXPECT_FLOAT_EQ(result, std::get<1>(input));
  }
}

// NOLINTNEXTLINE
TEST(MathExpressionGrammar, g2_analyze) {
  namespace math = grammar2;
  ASSERT_EQ(peg::analyze<math::expression>(), 0);
}

// NOLINTNEXTLINE
TEST_F(MathExpressionTest, g2_evaluate) {
  namespace math = grammar2;

  auto test_input = [](const std::string& input) -> double {
    peg::memory_input in(input, "from content");
    ::math::ActionData out;
    const auto result = peg::parse<math::expression, math::action>(in, out);
    return out.res;
  };

  for (const auto& input : test_strings) {
    std::cout << "Input: " << input.first << std::endl;
    double result{0};
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
    EXPECT_NO_THROW(result = test_input(input.first));
    EXPECT_FLOAT_EQ(result, input.second);
  }

  // We'll intentionally omit the var_ref_map here to ensure a failure occurs
  for (const auto& input : test_w_var_ref) {
    std::cout << "Input: " << std::get<0>(input) << std::endl;
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
    EXPECT_THROW(test_input(std::get<0>(input)), std::runtime_error);
  }
}

// NOLINTNEXTLINE
TEST_F(MathExpressionTest, g2_evaluate_var_ref) {
  namespace math = grammar2;

  auto test_input = [](const std::string& input, const RefMap& ref_map) -> double {
    peg::memory_input in(input, "from content");
    ::math::ActionData out;
    out.var_ref_map = ref_map;
    const auto result = peg::parse<math::expression, math::action>(in, out);
    return out.res;
  };

  for (const auto& input : test_w_var_ref) {
    std::cout << "Input: " << std::get<0>(input) << std::endl;
    double result{0};
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-goto)
    EXPECT_NO_THROW(result = test_input(std::get<0>(input), std::get<2>(input)));
    EXPECT_FLOAT_EQ(result, std::get<1>(input));
  }
}
