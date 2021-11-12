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
#include <tao/pegtl/contrib/json.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <tao/pegtl/contrib/parse_tree_to_dot.hpp>
#include <tao/pegtl/contrib/trace.hpp>

namespace peg = TAO_PEGTL_NAMESPACE;

// The end goal of this is to be able to take a string of text (or a file)
// And create a structured tree of data.
// The data should consist only of specific types and eliminate all other
// unecessary data (e.g. whitespace, etc).

namespace json_ish {

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

struct HEXTAG : peg::seq<peg::one<'0'>, peg::one<'x', 'X'>> {};
struct HEX : peg::seq<HEXTAG, peg::plus<peg::xdigit>> {};

struct sign : peg::one<'+', '-'> {};
struct exp
    : peg::seq<peg::one<'e', 'E'>, peg::opt<sign>, peg::plus<peg::digit>> {};
struct INTEGER
    : peg::seq<peg::opt<sign>,
               peg::sor<peg::one<'0'>, peg::seq<peg::range<'1', '9'>,
                                                peg::star<peg::digit>>>> {};
struct FLOAT
    : peg::seq<INTEGER, peg::one<'.'>, peg::star<peg::digit>, peg::opt<exp>> {};
struct NUMBER : peg::sor<FLOAT, INTEGER> {};
/*
struct NUMBER
    : peg::seq<peg::opt<sign>, peg::plus<peg::digit>,
               peg::opt<peg::seq<peg::one<'.'>, peg::star<peg::digit>>>,
               peg::opt<exp>> {};
*/
struct STRING
    : peg::seq<peg::one<'"'>, peg::plus<peg::not_one<'"'>>, peg::one<'"'>> {};

struct VALUE;
struct LIST : peg::seq<SBo, peg::list<VALUE, COMMA, peg::space>, SBc> {};

struct MAP;
struct VALUE : peg::sor<HEX, LIST, NUMBER, STRING, MAP> {};
struct PAIR : peg::seq<STRING, KVs, VALUE> {};

struct MAP : peg::seq<CBo, peg::list<PAIR, COMMA, peg::space>, CBc> {};

struct grammar : peg::seq<MAP, peg::eolf> {};

template <typename Rule>
using selector = peg::parse_tree::selector<
    Rule, peg::parse_tree::store_content::on<HEX, NUMBER, STRING>,
    peg::parse_tree::fold_one::on<VALUE, MAP, PAIR, LIST>>;

template <typename Rule> struct action : peg::nothing<Rule> {};

/*
template <> struct action<HEX> {
  template <typename ActionInput>
  static void apply(const ActionInput &in, std::string &out) {
    std::cout << "In HEX action: " << in.string() << "\n";
    out += "|H" + in.string();
  }
};
template <> struct action<STRING> {
  template <typename ActionInput>
  static void apply(const ActionInput &in, std::string &out) {
    std::cout << "In STRING action: " << in.string() << "\n";
    out += "|S" + in.string();
  }
};
template <> struct action<NUMBER> {
  template <typename ActionInput>
  static void apply(const ActionInput &in, std::string &out) {
    std::cout << "In NUMBER action: " << in.string() << "\n";
    out += "|N" + in.string();
  }
};
*/
} // namespace json_ish

template <typename GTYPE>
auto runTest(size_t idx, const std::string &test_str, bool pdot = true)
    -> bool {
  std::cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n";
  std::cout << "Parsing example " << idx << ":\n";
  std::cout << test_str << std::endl;
  std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

  const auto source = "example " + std::to_string(idx);
  peg::memory_input in(test_str, source);

  bool ret{false};
  std::string out;
  try {
    if (const auto root =
            peg::parse_tree::parse<GTYPE, json_ish::selector>(in)) {
      if (pdot) {
        peg::parse_tree::print_dot(std::cout, *root);
      }
      std::cout << "  Parse tree success.\n";
    } else {
      std::cout << "  Parse tree failure!\n";
      in.restart();
      peg::standard_trace<GTYPE>(in);
    }
    // Re-use of an input is highly discouraged, but given that we probably
    // won't call both `parse_tree::parse` and `parse` in the same run, we'll
    // re-use the input here for convenience.
    in.restart();
    ret = peg::parse<GTYPE, json_ish::action>(in, out);
    std::cout << "  Parse " << (ret ? "success" : "failure") << std::endl;
    std::cout << "out: " << out << std::endl;
  } catch (const peg::parse_error &e) {
    std::cout << "!!!\n";
    std::cout << "  Parser failure!\n";
    const auto p = e.positions().front();
    std::cout << e.what() << '\n'
              << in.line_at(p) << '\n'
              << std::setw(p.column) << '^' << '\n';
    std::cout << "!!!\n";
  }

  std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n";
  return ret;
}

auto main() -> int {

  const bool pdot = false;

  if (peg::analyze<json_ish::grammar>() != 0) {
    std::cout << "Something in the grammar is broken!" << std::endl;
  }

  size_t test_num = 1;
  {
    std::string content = "{";
    runTest<peg::must<json_ish::CBo, peg::eolf>>(test_num++, content, pdot);
  }
  {
    std::string content = "-1001";
    runTest<peg::must<json_ish::INTEGER, peg::eolf>>(test_num++, content, pdot);
  }
  {
    std::string content = "\"float\": 5.37e+6";
    runTest<peg::must<json_ish::PAIR, peg::eolf>>(test_num++, content, pdot);
  }
  {
    std::string content = "1234.";
    runTest<peg::must<json_ish::NUMBER, peg::eolf>>(test_num++, content, pdot);
  }
  {
    std::string content = "0x0ab0";
    runTest<peg::must<json_ish::VALUE, peg::eolf>>(test_num++, content, pdot);
  }

  std::vector map_strs = {
      "{\"ints\":[1, 2,  -3 ], \"more_ints\": [1, 2, -5]}",
      "{\"ints\"  :   \"test\"     }",
      "{\"id\":\"0001\",\"type\":0.55,\"thing\"  :[1, 2.2 ,5.  ], \n"
      "\"sci_notation_test\": [1.e3, 1.E-5, -4.3e+5] }",
      "{\n\
  \"id\": \"0001\",\n\
  \"type\": \"donut\",\n\
  \"name\": \"Cake\",\n\
  \"ppu\": 0x0deadbeef0,\n\
  \"batters\": [ 0.2, 0.4, 0.6 ] \n\
}",

      "{\n\
  \"id\": \"0001\",\n\
  \"type\": \"donut\",\n\
  \"name\": \"Cake\",\n\
  \"ppu\": [0.55, 1.3, 3, -2.5e2],\n\
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
    // Some of these aren't actually json, so we won't expect all of the json
    // parsing to work. Some of the unsupported features are hexadecimal values
    // and decimal values that end in a decimal without trailing numbers.
    runTest<peg::json::text>(test_num, content, pdot);
    runTest<json_ish::grammar>(test_num++, content, pdot);
  }
  return 0;
}
