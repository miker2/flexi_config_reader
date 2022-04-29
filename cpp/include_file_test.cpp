
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/analyze.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <tao/pegtl/contrib/parse_tree_to_dot.hpp>
#include <tao/pegtl/contrib/trace.hpp>
#include <vector>

#include "config_grammar.h"

namespace peg = TAO_PEGTL_NAMESPACE;

namespace filename {

template <typename Rule>
using selector =
    peg::parse_tree::selector<Rule, peg::parse_tree::store_content::on<FILEPART, FILENAME, EXT>,
                              peg::parse_tree::remove_content::on<ALPHAPLUS>>;

}  // namespace filename

namespace include_file {
struct INCLUDE : TAO_PEGTL_KEYWORD("#include") {};

struct grammar : peg::seq<INCLUDE, peg::plus<peg::blank>, filename::FILENAME> {};
}  // namespace include_file

template <typename GTYPE, typename SOURCE>
auto runTest(SOURCE& src, bool pdot = true) -> bool {
  std::cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n";
  std::cout << "Parsing: " << src.source() << "\n----- content -----\n";
  std::cout << std::string_view(src.begin(), src.size()) << std::endl;
  std::cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

  bool ret{false};
  try {
    if (const auto root = peg::parse_tree::parse<GTYPE>(src)) {
      if (pdot) {
        peg::parse_tree::print_dot(std::cout, *root);
      }
      std::cout << "  Parse tree success.\n";
    } else {
      std::cout << "  Parse tree failure!\n";
      src.restart();
      peg::standard_trace<GTYPE>(src);
    }
    // Re-use of an input is highly discouraged, but given that we probably
    // won't call both `parse_tree::parse` and `parse` in the same run, we'll
    // re-use the input here for convenience.
    src.restart();
    ret = peg::parse<GTYPE>(src);
    std::cout << "  Parse " << (ret ? "success" : "failure") << std::endl;
  } catch (const peg::parse_error& e) {
    std::cout << "!!!\n";
    std::cout << "  Parser failure!\n";
    const auto p = e.positions().front();
    std::cout << e.what() << '\n' << src.line_at(p) << '\n' << std::setw(p.column) << '^' << '\n';
    std::cout << "!!!\n";
  }

  std::cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n";
  return ret;
}

template <typename GTYPE>
auto runTest(size_t idx, const std::string& test_str, bool pdot = true) -> bool {
  peg::memory_input in(test_str, "example " + std::to_string(idx));

  return runTest<GTYPE>(in, pdot);
}

auto main() -> int {
  const bool pdot = false;

  if (peg::analyze<filename::grammar>() != 0) {
    std::cout << "Something in the grammar is broken!" << std::endl;
    return 1;
  }

  bool ret{true};
  size_t test_num = 1;

  std::vector<std::string> test_filenames = {"example1.cfg", "../example2.cfg", "foo/bar/baz.cfg"};

  for (const auto& e : test_filenames) {
    ret &= runTest<filename::FILENAME>(test_num++, e);
  }

  runTest<include_file::grammar>(999, "#include ../robot/default.cfg");

  return (ret ? 0 : 10);
}
