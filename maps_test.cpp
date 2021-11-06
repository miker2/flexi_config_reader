// Type your code here, or load an example.
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <typeinfo>
#include <variant>
#include <vector>

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/abnf.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <tao/pegtl/contrib/parse_tree_to_dot.hpp>

namespace peg = TAO_PEGTL_NAMESPACE;

namespace maps {

// Consider using 'pad<R, S, T>' in a few places below.
struct WS_ : peg::star<peg::space> {};
struct COMMA : peg::seq<peg::one<','>, WS_> {};
struct SBo : peg::seq<peg::one<'['>, WS_> {};
struct SBc : peg::seq<peg::one<']'>, WS_> {};
struct CBo : peg::seq<peg::one<'{'>, WS_> {};
struct CBc : peg::seq<peg::one<'}'>, WS_> {};
struct KVs : peg::seq<peg::one<':'>, WS_> {};

struct HEX
    : peg::seq<peg::one<'0'>, peg::one<'x', 'X'>, peg::plus<peg::xdigit>> {};

struct sign : peg::one<'+', '-'> {};
struct exp
    : peg::seq<peg::one<'e', 'E'>, peg::opt<sign>, peg::plus<peg::digit>> {};
struct NUMBER
    : peg::seq<peg::opt<sign>, peg::plus<peg::digit>,
               peg::opt<peg::seq<peg::one<'.'>, peg::star<peg::digit>>>,
               peg::opt<exp>, WS_> {};

struct VALUE;
struct LIST : peg::seq<SBo, VALUE, peg::star<peg::seq<COMMA, VALUE>>, SBc> {};
struct STRING : peg::seq<peg::one<'"'>, peg::plus<peg::not_one<'"'>>,
                         peg::one<'"'>, WS_> {};

struct MAP;
struct VALUE : peg::sor<LIST, NUMBER, STRING, MAP> {};
struct PAIR : peg::seq<STRING, KVs, VALUE> {};

struct MAP : peg::seq<CBo, PAIR, peg::star<peg::seq<COMMA, PAIR>>, CBc> {};

struct grammar : peg::must<MAP, peg::eolf> {};
/*
map     <-  CBo PAIR (COMMA PAIR)* CBc
PAIR    <-  string KVs value
value   <-  list / number / string / map
string  <-  '"' [^"]* '"' _
list    <-  SBo value ("," _ value)* SBc
number  <-  [+-]? [0-9]+ ("." [0-9]*)? ("e" [+-]? [0-9]+)? _
HEX     <-  "0" [xX] [0-9a-fA-F]+
KVs     <-  ":" _
CBo     <-  "{" _
CBc     <-  "}" _
SBo     <-  "[" _
SBc     <-  "]" _
COMMA   <-  "," _
_       <-  [ \t\r\n]*
*/

} // namespace maps

auto main() -> int {

  if (peg::analyze<maps::grammar>() != 0) {
    std::cout << "Something in the grammar is broken!" << std::endl;
  }

  {
    std::string content = "[";
    std::cout << "Parsing example 1" << std::endl;

    peg::string_input in(content, "example 1");

    if (const auto root = peg::parse_tree::parse<maps::SBo>(in)) {
      auto ret = peg::parse<maps::SBo>(in);
      std::cout << "  Parse " << (ret ? "success" : "failure") << std::endl;
    } else {
      std::cout << "  Parse failed." << std::endl;
    }
  }
  {
    std::string content = "\"int\": 5.37e+6";
    std::cout << "Parsing example 2" << std::endl;

    peg::string_input in(content, "example 2");

    if (const auto root = peg::parse_tree::parse<maps::PAIR>(in)) {
      peg::parse_tree::print_dot(std::cout, *root);
      auto ret = peg::parse<maps::PAIR>(in);
      std::cout << "  Parse " << (ret ? "success" : "failure") << std::endl;
    } else {
      std::cout << "  Parse failed." << std::endl;
    }
  }

  {
    std::string content =
        "{\"ints\":[1, 2,  -3 ], \"more_ints\": [0001, 0002, -05]}";
    std::cout << "Parsing example 3" << std::endl;
    peg::string_input in(content, "example 3");

    try {
      auto root = peg::parse_tree::parse<maps::grammar>(in);
      peg::parse_tree::print_dot(std::cout, *root);
      std::cout << "  Parse tree success!\n";
      auto ret = peg::parse<maps::grammar>(in);
    } catch (const peg::parse_error &e) {
      std::cout << "  Something failed!\n";
      const auto p = e.positions().front();
      std::cout << e.what() << '\n'
                << in.line_at(p) << '\n'
                << std::setw(p.column) << '^' << '\n';
    }
  }
  return 0;
}
