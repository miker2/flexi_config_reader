#include <iostream>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <tao/pegtl/contrib/parse_tree_to_dot.hpp>
#include <unordered_map>
#include <vector>

#include "config_grammar.h"

// This article has been extremely useful:
//   https://www.engr.mun.ca/~theo/Misc/exp_parsing.htm

// See: https://godbolt.org/z/7vdE6aYdh
//      https://godbolt.org/z/9aeh7hvjv
//      https://godbolt.org/z/35c19neYG

namespace peg = TAO_PEGTL_NAMESPACE;

// A rule for padding another rule with blanks on either side
template <typename Rule>
struct pd : peg::pad<Rule, peg::blank> {};

struct Um : peg::one<'-'> {};
struct Up : peg::one<'+'> {};
struct Uo : peg::sor<Um, Up> {};

struct Bplus : peg::one<'+'> {};
struct Bminus : peg::one<'-'> {};
struct Bmult : peg::one<'*'> {};
struct Bdiv : peg::one<'/'> {};
struct Bpow : peg::one<'^'> {};
struct Bo : peg::sor<Bplus, Bminus, Bmult, Bdiv, Bpow> {};

struct Po : pd<peg::one<'('>> {};
struct Pc : pd<peg::one<')'>> {};

struct Mo : pd<TAO_PEGTL_STRING("{{")> {};
struct Mc : pd<TAO_PEGTL_STRING("}}")> {};

struct ignored : peg::space {};

// v includes numbers, variables & var refs
struct v : peg::sor<config::NUMBER, config::VAR, config::VAR_REF> {};

namespace grammar1 {
// Grammar G1:
//    E --> P {B P}
//    P --> v | "(" E ")" | U P
//    B --> "+" | "-" | "*" | "/" | "^"
//    U --> "-"

struct E;
struct BRACKET : peg::seq<Po, E, Pc> {};
struct atom : peg::sor<v, BRACKET> {};
struct P;
struct P : peg::sor<atom /*v, BRACKET*/, peg::seq<Uo, P>> {};
struct E : peg::list<P, Bo, ignored> {};

template <typename Rule>
struct selector : peg::parse_tree::selector<
                      Rule, peg::parse_tree::store_content::on<v, E, Bo, Uo>,
                      peg::parse_tree::remove_content::on</*Um, Bplus, Bminus, Bmult, Bdiv, Bpow*/>,
                      peg::parse_tree::fold_one::on<P, BRACKET>> {};

}  // namespace grammar1

namespace grammar2 {
/*
E --> T {( "+" | "-" ) T}
T --> F {( "*" | "/" ) F}
F --> P ["^" F]
P --> v | "(" E ")" | "-" T
*/

struct PM : peg::sor<Bplus, Bminus> {};
struct MD : peg::sor<Bmult, Bdiv> {};

struct T;
struct E : peg::seq<T, peg::star<pd<PM>, T>> {};
struct F;
struct T : peg::seq<F, peg::star<pd<MD>, F>> {};
struct EXP : peg::seq<Bpow, F> {};
struct P;
struct F : peg::seq<P, peg::opt<EXP>> {};
struct P : peg::sor<pd<v>, peg::seq<Po, E, Pc>, peg::seq<peg::one<'-'>, T>> {};

// template< typename Rule > struct selector : std::true_type {};

template <typename Rule>
struct selector : peg::parse_tree::selector<
                      Rule, peg::parse_tree::store_content::on<v, E, T, PM, MD, Bpow>,
                      peg::parse_tree::remove_content::on</*Um, Bplus, Bminus, Bmult, Bdiv, Bpow*/>,
                      peg::parse_tree::fold_one::on<P, EXP, F>> {};

}  // namespace grammar2

int main() {
  namespace math = grammar2;

  struct grammar : peg::seq<Mo, math::E, Mc> {};
  auto ret = peg::analyze<grammar>();

  std::cout << "Grammar is valid: " << (ret == 0 ? "true" : "false") << std::endl;

  // std::string input = "a*b - c^d - e*f";
  std::string input = " {{  $A ^ $(b.foo) * $C + $D + $(e.bar) }}";
  // std::string input = "-x * -(y + z)";

  peg::memory_input in(input, "from content");
  const auto result = peg::parse<grammar>(in);

  std::cout << "Parse result: " << result << std::endl;

  in.restart();
  if (const auto root = peg::parse_tree::parse<grammar, math::selector>(in)) {
    peg::parse_tree::print_dot(std::cout, *root);
  }

  return 0;
}
