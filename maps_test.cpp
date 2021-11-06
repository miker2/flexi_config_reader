// Type your code here, or load an example.
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
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

struct WS_ : peg::star<peg::space> {};
struct COMMA : peg::one<','> {};
struct SBo : peg::pad<peg::one<'['>, peg::space> {};
struct SBc : peg::pad<peg::one<']'>, peg::space> {};
struct CBo : peg::pad<peg::one<'{'>, peg::space> {};
struct CBc : peg::pad<peg::one<'}'>, peg::space> {};
struct KVs : peg::pad<peg::one<':'>, peg::space> {};

struct HEX
    : peg::seq<peg::one<'0'>, peg::one<'x', 'X'>, peg::plus<peg::xdigit>> {};

struct sign : peg::one<'+', '-'> {};
struct exp
    : peg::seq<peg::one<'e', 'E'>, peg::opt<sign>, peg::plus<peg::digit>> {};
struct NUMBER
    : peg::seq<peg::opt<sign>, peg::plus<peg::digit>,
               peg::opt<peg::seq<peg::one<'.'>, peg::star<peg::digit>>>,
               peg::opt<exp>> {};

struct VALUE;
struct LIST : peg::seq<SBo, peg::list<VALUE, COMMA, peg::space>, SBc> {};
struct STRING
    : peg::seq<peg::one<'"'>, peg::plus<peg::not_one<'"'>>, peg::one<'"'>> {};

struct MAP;
struct VALUE : peg::sor<LIST, NUMBER, STRING, MAP> {};
struct PAIR : peg::seq<STRING, KVs, VALUE> {};

struct MAP : peg::seq<CBo, peg::list<PAIR, COMMA, peg::space>, CBc> {};

struct grammar : peg::must<MAP, peg::eolf> {};

} // namespace maps

template <typename GTYPE>
auto runTest(size_t idx, const std::string &test_str, bool pdot = true)
    -> bool {
  std::cout << "Parsing example " << idx << "\n";
  peg::string_input in(test_str, "example " + std::to_string(idx));

  try {
    if (const auto root = peg::parse_tree::parse<GTYPE>(in)) {
      if (pdot) {
        peg::parse_tree::print_dot(std::cout, *root);
      }
      std::cout << "  Parse tree success.\n";
    } else {
      std::cout << "  Parse tree failure!\n";
    }
    auto ret = peg::parse<GTYPE>(in);
    std::cout << "  Parse " << (ret ? "success" : "failure") << std::endl;
    return ret;
  } catch (const peg::parse_error &e) {
    std::cout << "!!!\n";
    std::cout << "  Parser failure!\n";
    const auto p = e.positions().front();
    std::cout << e.what() << '\n'
              << in.line_at(p) << '\n'
              << std::setw(p.column) << '^' << '\n';
    std::cout << "!!!\n";
  }

  return false;
}

auto main() -> int {

  const bool pdot = false;

  if (peg::analyze<maps::grammar>() != 0) {
    std::cout << "Something in the grammar is broken!" << std::endl;
  }

  size_t test_num = 1;
  {
    std::string content = "{";
    runTest<peg::must<maps::CBo, peg::eolf>>(test_num++, content, pdot);
  }
  {
    std::string content = "\"int\": 5.37e+6";
    runTest<peg::must<maps::PAIR, peg::eolf>>(test_num++, content, pdot);
  }

  std::vector map_strs = {
      "{\"ints\":[1, 2,  -3 ], \"more_ints\": [0001, 0002, -05]}",
      "{\"ints\"  :   \"test\"     }",
      "{\"id\":\"0001\",\"type\":0.55,\"thing\"  :[1, 2.2 ,5.  ], "
      "\"sci_not_test\": [1e3, 1.e-5, -4.3e+5] }",
      "{\n\
  \"id\": \"0001\",\n\
  \"type\": \"donut\",\n\
  \"name\": \"Cake\",\n\
  \"ppu\": 0.55,\n\
  \"batters\": [ 0.2, 0.4, 0.6 ] \n\
}",

      "{\n\
  \"id\": \"0001\",\n\
  \"type\": \"donut\",\n\
  \"name\": \"Cake\",\n\
  \"ppu\": 0.55,\n\
  \"batters\":\n\
    {\n\
      \"batter\":\n\
        [\n\
          { \"id\": \"1001\", \"type\": \"Regular\" },\n\
          { \"id\": \"1002\", \"type\": \"Chocolate\" },\n\
          { \"id\": \"1003\", \"type\": \"Blueberry\" },\n\
          { \"id\": \"1004\", \"type\": \"Devil's Food\" }\n\
        ]\n\
    },\n\
  \"topping\":\n\
    [\n\
      { \"id\": \"5001\", \"type\": \"None\" },\n\
      { \"id\": \"5002\", \"type\": \"Glazed\" },\n\
      { \"id\": \"5005\", \"type\": \"Sugar\" },\n\
      { \"id\": \"5007\", \"type\": \"Powdered Sugar\" },\n\
      { \"id\": \"5006\", \"type\": \"Chocolate with Sprinkles\" },\n\
      { \"id\": \"5003\", \"type\": \"Chocolate\" },\n\
      { \"id\": \"5004\", \"type\": \"Maple\" }\n\
    ]\n\
}"};

  for (const auto &content : map_strs) {
    std::cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n";
    std::cout << "Testing:\n" << content << std::endl;
    std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";
    runTest<maps::grammar>(test_num++, content, pdot);
    std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n";
  }
  return 0;
}
