// Type your code here, or load an example.
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/abnf.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <tao/pegtl/contrib/parse_tree_to_dot.hpp>
#include <tao/pegtl/contrib/trace.hpp>
#include <tao/pegtl/contrib/utf32.hpp>
#include <typeinfo>
#include <variant>
#include <vector>

namespace peg = TAO_PEGTL_NAMESPACE;

// The end goal of this is to be able to take a string of text (or a file)
// And create a structured tree of data.
// The data should consist only of specific types and eliminate all other
// unecessary data (e.g. whitespace, etc).

namespace toml {
#if 0
struct COMMENT
    : peg::seq<peg::one<'#'>,
               peg::star<peg::seq<peg::not_at<peg::eol>, peg::any>>> {};
struct WS : peg::star<peg::blank> {};
struct IGNORE : peg::star<peg::sor<COMMENT, peg::blank, peg::eol>> {};
struct LINE_END
    : peg::seq<WS, peg::opt<COMMENT>,
               peg::not_at<peg::seq<peg::not_at<peg::eol>, peg::any>>> {};

struct COMMA : peg::one<','> {};
struct SBo : peg::pad<peg::one<'['>, peg::blank> {};
struct SBc : peg::pad<peg::one<']'>, peg::blank> {};
struct CBo : peg::pad<peg::one<'{'>, peg::blank> {};
struct CBc : peg::pad<peg::one<'}'>, peg::blank> {};
struct KVs : peg::pad<peg::one<':'>, peg::blank> {};

struct ESCAPE_CHAR : peg::one<'0', 't', 'n', 'r', '"', '\\'> {};

struct sign : peg::one<'+', '-'> {};
struct exp
    : peg::seq<peg::one<'e', 'E'>, peg::opt<sign>, peg::plus<peg::digit>> {};

struct INTEGER
    : peg::seq<peg::opt<sign>, peg::range<'1', '9'>, peg::star<peg::digit>> {};
struct FLOAT
    : peg::seq<INTEGER, peg::one<'.'>, peg::plus<peg::digit>, peg::opt<exp>> {};

struct STRING
    : peg::seq<peg::one<'"'>,
               peg::star<peg::sor<peg::seq<peg::one<'\\'>, ESCAPE_CHAR>,
                                  peg::not_one<'"'>>>,
               peg::one<'"'>> {};

struct BOOLEAN : peg::sor<peg::keyword<'t', 'r', 'u', 'e'>,
                          peg::keyword<'f', 'a', 'l', 's', 'e'>> {};

struct YEAR
    : peg::seq<peg::range<'1', '9'>, peg::digit, peg::digit, peg::digit> {};
struct TWODIGIT : peg::seq<peg::digit, peg::digit> {};
struct DATETIME
    : peg::seq<YEAR, peg::one<'-'>, TWODIGIT, peg::one<'-'>, TWODIGIT,
               peg::one<'T'>, TWODIGIT, peg::one<':'>, TWODIGIT, peg::one<':'>,
               TWODIGIT,
               peg::opt<peg::seq<peg::one<'.'>, peg::plus<peg::digit>>>,
               peg::one<'Z'>> {};

struct EMPTY_ARRAY : peg::seq<SBo, SBc> {};
template <typename Key>
struct TYPE_ARRAY : peg::seq<SBo, peg::list<Key, COMMA, peg::blank>, SBc> {};
struct STRING_ARRAY : TYPE_ARRAY<STRING> {};
struct INTEGER_ARRAY : TYPE_ARRAY<INTEGER> {};
struct FLOAT_ARRAY : TYPE_ARRAY<FLOAT> {};
struct BOOLEAN_ARRAY : TYPE_ARRAY<BOOLEAN> {};
struct DATETIME_ARRAY : TYPE_ARRAY<DATETIME> {};
struct ARRAY_ARRAY : TYPE_ARRAY<struct ARRAY> {};

struct ARRAY : peg::sor<EMPTY_ARRAY, STRING_ARRAY, DATETIME_ARRAY, FLOAT_ARRAY,
                        INTEGER_ARRAY, BOOLEAN_ARRAY, ARRAY_ARRAY> {};

struct VALUE : peg::sor<STRING, DATETIME, FLOAT, INTEGER, BOOLEAN, ARRAY> {};

struct NAME : peg::plus<peg::seq<peg::not_at<peg::blank>, peg::any>> {};
struct VALUE_LINE : peg::seq<IGNORE, NAME, KVs, VALUE, LINE_END> {};

struct KEY_SEGMENT : peg::plus<peg::not_one<'[', ']', '.'>> {};
struct KEY_NAME
    : peg::seq<KEY_SEGMENT, peg::star<peg::seq<peg::one<'.'>, KEY_SEGMENT>>> {};
struct HEADER_LINE : peg::seq<IGNORE, SBo, KEY_NAME, SBc, LINE_END> {};
struct KEY_GROUP
    : peg::seq<peg::opt<HEADER_LINE>, peg::plus<VALUE_LINE>, IGNORE> {};

struct grammar : peg::must<peg::star<KEY_GROUP>> {};

#else

#include "toml_grammar.h"

struct grammar : peg::must<toml> {};

#endif

template <typename Rule>
struct selector : std::true_type {};

template <typename Rule>
struct action : peg::nothing<Rule> {};

}  // namespace toml

template <typename GTYPE>
auto runTest(size_t idx, const std::string &infile, bool pdot = true) -> bool {
  std::cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n";
  std::cout << "Parsing example " << idx << " from file " << infile << ".\n";
  std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

  peg::file_input in(infile);

  bool ret{false};
  std::string out;
  try {
    peg::standard_trace<GTYPE>(peg::file_input(infile));
    if (const auto root = peg::parse_tree::parse<GTYPE, toml::selector>(in)) {
      if (pdot) {
        peg::parse_tree::print_dot(std::cout, *root);
      }
      std::cout << "  Parse tree success.\n";
    } else {
      std::cout << "  Parse tree failure!\n";
    }
    // Re-use of an input is highly discouraged, but given that we probably
    // won't call both `parse_tree::parse` and `parse` in the same run, we'll
    // re-use the input here for convenience.
    in.restart();
    ret = peg::parse<GTYPE, toml::action>(in, out);
    std::cout << "  Parse " << (ret ? "success" : "failure") << std::endl;
    std::cout << "out: " << out << std::endl;
  } catch (const peg::parse_error &e) {
    std::cout << "!!!\n";
    std::cout << "  Parser failure!\n";
    const auto p = e.positions().front();
    std::cout << e.what() << '\n' << in.line_at(p) << '\n' << std::setw(p.column) << '^' << '\n';
    std::cout << "!!!\n";
  }

  std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n";
  return ret;
}

auto main(int argc, char *argv[]) -> int {
  const bool pdot = true;

  if (peg::analyze<toml::grammar>(1) != 0) {
    std::cout << "Something in the grammar is broken!" << std::endl;
    return 10;
  }

  bool ret{true};
  if (argc > 1) {
    for (size_t i = 1; i < argc; ++i) {
      ret &= runTest<toml::grammar>(i, std::string(argv[i]), pdot);
    }
  }
  return (ret ? 0 : 1);
}
