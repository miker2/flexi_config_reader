#include "math_grammar.h"

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

// This article has been extremely useful:
//   https://www.engr.mun.ca/~theo/Misc/exp_parsing.htm

// See: https://godbolt.org/z/7vdE6aYdh
//      https://godbolt.org/z/9aeh7hvjv
//      https://godbolt.org/z/35c19neYG

// https://marginalhacks.com/Hacks/libExpr.rb/
// https://en.wikipedia.org/wiki/Shunting_yard_algorithm
// https://en.wikipedia.org/wiki/Shift-reduce_parser

namespace grammar1 {
template <typename Rule>
struct selector : peg::parse_tree::selector<
                      Rule, peg::parse_tree::store_content::on<v, expression, Bo, Uo>,
                      peg::parse_tree::remove_content::on</*Um, Bplus, Bminus, Bmult, Bdiv, Bpow*/>,
                      peg::parse_tree::fold_one::on<P, BRACKET>> {};

}  // namespace grammar1

namespace grammar2 {
// template< typename Rule > struct selector : std::true_type {};

template <typename Rule>
struct selector
    : peg::parse_tree::selector<
          Rule, peg::parse_tree::store_content::on<v, expression, T, F, P, PM, MD, Bpow, N>,
          peg::parse_tree::remove_content::on</*Um, Bplus, Bminus, Bmult, Bdiv, Bpow*/>,
          peg::parse_tree::fold_one::on<EXP>> {};

}  // namespace grammar2

auto nearEq(double res, double exp_res, double tol = 1e-8) { return abs(exp_res - res) < tol; }

int main() {
  namespace math = grammar1;

  struct grammar : peg::seq<Mo, math::expression, Mc> {};
  auto ret = peg::analyze<grammar>();

  std::cout << "Grammar is valid: " << (ret == 0 ? "true" : "false") << std::endl;

  auto test_input = [](const std::string& input) -> double {
    peg::memory_input in(input, "from content");

    if (const auto root = peg::parse_tree::parse<grammar, math::selector>(in)) {
      peg::parse_tree::print_dot(std::cout, *root);
      std::cout << std::endl;
    }

    in.restart();
    ActionData out;
    const auto result = peg::parse<grammar, math::action>(in, out);

    std::cout << "Result: " << out.res << std::endl;

    std::cout << "Parse success: " << (result == 0 ? "false" : "true") << std::endl;
    std::cout << std::endl;

    return out.res;
  };

  std::vector<std::pair<std::string, double>> test_strings = {
      /*"$VAR*$BAR - $C^$D - $E*$FOO",
      "$A ^ $(b.foo) * $C + $D + $(e.bar) ",
      "-$(x) * -($(y) + $(z))",*/
      {"3.14159 * 1e3", 3141.5899999999997},
      {"0.5 * (0.7 + 1.2)", 0.95},
      {"0.5 + 0.7 * 1.2", 1.3399999999999999},
      {"3*0.27 - 2.3**0.5 - 5*4", -20.70657508881031},
      {"3 ^ 2.4 * 12.2 + 0.1 + 4.3 ", 174.79264401590646},
      {"-4.7 * -(3.72 + -pi)", 2.7185145281279732},
      {"1/3 * -(5 + 4)", -3.0},
      {"3.4 * -(1.9**2 * (1/3.1 - 6) * (2.54- 17.0))", -1007.6399690322581}};

  bool success{true};
  for (const auto& input : test_strings) {
    std::cout << "Input: " << input.first << std::endl;
    auto result = test_input(" {{  " + input.first + "   }}");
    std::cout << "Expected result: " << input.second << "\n\n\n";
    success &= nearEq(result, input.second);
  }

  return success ? 0 : 1;
}
